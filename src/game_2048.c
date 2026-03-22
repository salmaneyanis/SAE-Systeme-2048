/*
 * game_2048.c
 *
 * Processus principal du jeu 2048.
 *
 * Architecture :
 *   - Thread principal   : lit le pipe nommé, transmet les commandes à Move&Score
 *   - Thread Move&Score  : attend SIGUSR1, déplace les tuiles, réveille Goal
 *   - Thread Goal        : attend SIGUSR1, vérifie fin de partie, envoie au display
 *
 * Synchronisation inter-threads : SIGNAUX uniquement (pthread_kill + sigwait)
 * Communication inter-processus : pipe nommé (main->2048) et pipe anonyme (2048->display)
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "game_state.h"
#include "game_logic.h"
#include "ipc.h"

/* Signaux de synchronisation inter-threads */
#define SIG_MOVE SIGUSR1   /* principal -> Move&Score */
#define SIG_GOAL SIGUSR2   /* Move&Score -> Goal      */

/* Etat du jeu partage entre Move&Score et Goal. */
static GameState shared_game_state;

/* Direction du prochain mouvement, ecrite par le principal, lue par Move&Score. */
static volatile MoveDirection pending_direction;

/* Controle d'arret global. */
static volatile sig_atomic_t game_running = 1;

/* TIDs pour pouvoir envoyer des signaux aux threads depuis d'autres threads. */
static pthread_t move_tid;
static pthread_t goal_tid;

/* Pipe anonyme vers le processus d'affichage. */
static int display_pipe[2];

/* PID du processus d'affichage. */
static pid_t display_pid = 0;

/* ============================================================
 * UTILITAIRES
 * ============================================================ */

static MoveDirection command_to_direction(CommandType cmd)
{
    switch (cmd) {
        case CMD_MOVE_UP:    return MOVE_UP;
        case CMD_MOVE_DOWN:  return MOVE_DOWN;
        case CMD_MOVE_LEFT:  return MOVE_LEFT;
        case CMD_MOVE_RIGHT: return MOVE_RIGHT;
        default:             return MOVE_UP;
    }
}

/* ============================================================
 * THREAD MOVE & SCORE
 *
 * Synchronisation : attend SIG_MOVE via sigwait().
 * Le thread principal ecrit pending_direction PUIS envoie SIG_MOVE.
 * Ce thread lit la direction, execute le mouvement, puis reveille Goal.
 * ============================================================ */
void *move_score_thread(void *arg)
{
    (void)arg;

    sigset_t wait_set;
    sigemptyset(&wait_set);
    sigaddset(&wait_set, SIG_MOVE);

    while (game_running) {
        int sig;
        if (sigwait(&wait_set, &sig) != 0) {
            perror("sigwait move");
            break;
        }

        if (!game_running)
            break;

        MoveDirection dir = pending_direction;
        bool moved = game_move(&shared_game_state, dir);

        if (moved)
            game_add_random_tile(&shared_game_state);

        /* Reveiller Goal : le mouvement est termine, il peut verifier l'etat. */
        pthread_kill(goal_tid, SIG_GOAL);
    }

    return NULL;
}

/* ============================================================
 * THREAD GOAL
 *
 * Synchronisation : attend SIG_GOAL via sigwait().
 * Move&Score envoie SIG_GOAL une fois le mouvement effectue.
 * Ce thread verifie victoire/defaite et envoie l'etat au display.
 * ============================================================ */
void *goal_thread(void *arg)
{
    (void)arg;

    sigset_t wait_set;
    sigemptyset(&wait_set);
    sigaddset(&wait_set, SIG_GOAL);

    while (game_running) {
        int sig;
        if (sigwait(&wait_set, &sig) != 0) {
            perror("sigwait goal");
            break;
        }

        if (!game_running)
            break;

        /* Verification des conditions de fin de partie. */
        if (shared_game_state.status == GAME_PLAYING) {
            if (game_is_won(&shared_game_state))
                shared_game_state.status = GAME_WON;
            else if (game_is_lost(&shared_game_state))
                shared_game_state.status = GAME_LOST;
        }

        /* Construction et envoi du message au display via pipe anonyme. */
        DisplayMessage display_msg;
        game_copy_state(&display_msg.state, &shared_game_state);
        display_msg.game_over = (shared_game_state.status != GAME_PLAYING);

        if (write(display_pipe[1], &display_msg, sizeof(DisplayMessage)) == -1)
            perror("write display_pipe");

        /* Reveil du processus display par signal inter-processus. */
        if (display_pid > 0)
            kill(display_pid, SIG_UPDATE_DISPLAY);

        /* Fin de partie : arreter tous les processus. */
        if (display_msg.game_over) {
            game_running = 0;
            if (display_pid > 0)
                kill(display_pid, SIG_GAME_OVER);
            break;
        }
    }

    return NULL;
}

/* ============================================================
 * HANDLER SIGINT / SIGTERM
 * ============================================================ */
void cleanup_handler(int sig)
{
    (void)sig;
    game_running = 0;
    /* Debloquer les threads bloques dans sigwait(). */
    pthread_kill(move_tid, SIG_MOVE);
    pthread_kill(goal_tid, SIG_GOAL);
    if (display_pid > 0)
        kill(display_pid, SIGTERM);
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main()
{
    srand(time(NULL));

    /*
     * 1. Bloquer SIG_MOVE et SIG_GOAL dans le thread principal.
     *
     *    Ces signaux doivent etre traites UNIQUEMENT via sigwait()
     *    dans les threads Move&Score et Goal. En les bloquant ici,
     *    AVANT la creation des threads, tous les threads heritent
     *    de ce masque, ce qui garantit que sigwait() est le seul
     *    point de delivrance de ces signaux.
     */
    sigset_t thread_sigmask;
    sigemptyset(&thread_sigmask);
    sigaddset(&thread_sigmask, SIG_MOVE);
    sigaddset(&thread_sigmask, SIG_GOAL);
    if (pthread_sigmask(SIG_BLOCK, &thread_sigmask, NULL) != 0) {
        perror("pthread_sigmask");
        exit(EXIT_FAILURE);
    }

    /* 2. Pipe anonyme pour envoyer l'etat du jeu au processus display. */
    if (pipe(display_pipe) == -1) {
        perror("pipe display");
        exit(EXIT_FAILURE);
    }

    /* 3. Fork du processus d'affichage. */
    display_pid = fork();
    if (display_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (display_pid == 0) {
        /* Fils (display) : lit depuis stdin (redirige vers le pipe). */
        close(display_pipe[1]);
        if (dup2(display_pipe[0], STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(display_pipe[0]);
        execl("./bin/display", "display", NULL);
        perror("execl display");
        exit(EXIT_FAILURE);
    }

    close(display_pipe[0]); /* parent : cote ecriture uniquement. */

    /* 4. Initialisation de l'etat du jeu. */
    game_init(&shared_game_state);

    /* 5. Handlers pour arret propre (SIGINT / SIGTERM). */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = cleanup_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /*
     * 6. Creation des threads.
     *
     *    Les threads heritent du masque de signaux => SIG_MOVE et SIG_GOAL
     *    sont bloques en eux, prets a etre recus via sigwait().
     */
    if (pthread_create(&move_tid, NULL, move_score_thread, NULL) != 0) {
        perror("pthread_create move_score");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&goal_tid, NULL, goal_thread, NULL) != 0) {
        perror("pthread_create goal");
        exit(EXIT_FAILURE);
    }

    /* 7. Pipe nomme (processus main -> processus 2048). */
    if (mkfifo(NAMED_PIPE_MAIN_TO_2048, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
    int named_pipe_fd = open(NAMED_PIPE_MAIN_TO_2048, O_RDONLY);
    if (named_pipe_fd == -1) {
        perror("open named pipe");
        exit(EXIT_FAILURE);
    }

    /* 8. Envoi de l'etat initial au display (grille de depart). */
    {
        DisplayMessage init_msg;
        game_copy_state(&init_msg.state, &shared_game_state);
        init_msg.game_over = false;
        if (write(display_pipe[1], &init_msg, sizeof(DisplayMessage)) == -1)
            perror("write initial display");
        if (display_pid > 0)
            kill(display_pid, SIG_UPDATE_DISPLAY);
    }

    /*
     * 9. Boucle principale : lecture des commandes depuis le pipe nomme.
     *
     *    - Mouvement : stocker la direction dans pending_direction,
     *      puis envoyer SIG_MOVE a Move&Score via pthread_kill().
     *    - CMD_QUIT  : arreter tout proprement.
     */
    CommandMessage cmd_msg;

    while (game_running) {
        ssize_t n = read(named_pipe_fd, &cmd_msg, sizeof(CommandMessage));

        if (n == 0)
            break; /* Pipe nomme ferme (processus main termine). */

        if (n == -1) {
            if (errno == EINTR)
                continue;
            perror("read named pipe");
            break;
        }

        if (n != sizeof(CommandMessage))
            continue;

        if (cmd_msg.command == CMD_QUIT) {
            game_running = 0;
            pthread_kill(move_tid, SIG_MOVE);
            pthread_kill(goal_tid, SIG_GOAL);
            if (display_pid > 0)
                kill(display_pid, SIGTERM);
            break;
        }

        /* Transmettre la direction et reveiller Move&Score par signal. */
        pending_direction = command_to_direction(cmd_msg.command);
        pthread_kill(move_tid, SIG_MOVE);
    }

    /* 10. Nettoyage. */
    close(named_pipe_fd);
    unlink(NAMED_PIPE_MAIN_TO_2048);

    pthread_join(move_tid, NULL);
    pthread_join(goal_tid, NULL);

    close(display_pipe[1]); /* EOF pour le display. */
    if (display_pid > 0)
        waitpid(display_pid, NULL, 0);

    return 0;
}