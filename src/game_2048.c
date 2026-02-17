/*
 * game_2048.c
 *
 * Ce fichier implémente le processus principal du jeu 2048.
 * Il coordonne la logique du jeu avec une architecture multithread :
 * - Thread Move&Score : Gère les mouvements et le calcul du score
 * - Thread Goal : Vérifie les conditions de victoire/défaite et envoie les mises à jour
 * - Thread principal : Lit les commandes et coordonne l'ensemble
 */

#define _POSIX_C_SOURCE 200809L // Pour les fonctions POSIX (nanosleep, etc.)
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

// ============================================
// VARIABLES GLOBALES PARTAGÉES ENTRE THREADS
// ============================================

/*
// État du jeu partagé entre tous les threads
static GameState shared_game_state;

// Mutex pour protéger l'accès à shared_game_state et aux variables de synchronisation
static pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

// Variable conditionnelle pour réveiller le thread Move&Score lorsqu'une commande arrive
static pthread_cond_t move_cond = PTHREAD_COND_INITIALIZER;

// Variable conditionnelle pour réveiller le thread Goal après un mouvement
static pthread_cond_t goal_cond = PTHREAD_COND_INITIALIZER;

// Commande en attente de traitement par le thread Move&Score
static CommandType pending_command = CMD_QUIT;  // Initialisé à QUIT pour arrêt propre

// Indicateur qu'une commande est prête à être traitée
static bool command_ready = false;

// Contrôle la boucle principale et les threads (false = arrêt)
static bool game_running = true;

// Indicateurs que les threads sont prêts (pour la synchronisation initiale)
static bool move_thread_ready = false;
static bool goal_thread_ready = false;
*/
// ============================================
// VARIABLES POUR LA COMMUNICATION INTER-PROCESSUS
// ============================================

// Descripteur du pipe anonyme vers le processus affichage
// display_pipe[0] = lecture, display_pipe[1] = écriture
static int display_pipe[2];

// PID du processus d'affichage (pour envoyer des signaux)
static pid_t display_pid = 0;

// ============================================
// FONCTIONS UTILITAIRES
// ============================================

/**
 * Convertit un CommandType (depuis le pipe) en MoveDirection (pour la logique du jeu)
 *
 * @param cmd : Commande reçue depuis le processus main
 * @return : Direction de mouvement correspondante
 */
static MoveDirection command_to_direction(CommandType cmd)
{
    switch (cmd)
    {
    case CMD_MOVE_UP:
        return MOVE_UP;
    case CMD_MOVE_DOWN:
        return MOVE_DOWN;
    case CMD_MOVE_LEFT:
        return MOVE_LEFT;
    case CMD_MOVE_RIGHT:
        return MOVE_RIGHT;
    default:
        return MOVE_UP; // Par défaut (ne devrait jamais arriver)
    }
}

// ============================================
// THREAD MOVE&SCORE
// ============================================

/**
 * Thread responsable du traitement des mouvements et du calcul du score
 *
 * Ce thread :
 * 1. Attend une commande depuis le thread principal
 * 2. Convertit la commande en direction de mouvement
 * 3. Exécute le mouvement avec game_move()
 * 4. Ajoute une nouvelle tuile si un mouvement a été effectué
 * 5. Réveille le thread Goal pour vérifier l'état du jeu
 *
 * @param arg : Paramètre non utilisé (NULL)
 * @return : Toujours NULL
 */
void *move_score_thread(void *arg)
{
    (void)arg; // Éviter l'avertissement du compilateur

    int fds = (int *)arg;
    int cmd_fd = fds[0];
    int state_fd = fds[1];

    GameState local_state;
    game_init(&local_state);

    CommandMessage cmd;

    while (1)
    {
        // Attente d'une commande (bloquant)
        ssize_t n = read(cmd_fd, &cmd, sizeof(cmd));

        if (n == 0)
        {
            // pipe fermé -> fin propre
            break;
        }

        if (n == -1)
        {
            perror("read cmd_fd");
            break;
        }

        if (n != sizeof(cmd))
        {
            // Message incomplet -> ignorer
            continue;
        }

        if (cmd.command == CMD_QUIT)
        {
            break;
        }

        // ============================================
        // TRAITEMENT DU MOUVEMENT
        // ============================================

        // Convertir la commande en direction de mouvement
        MoveDirection dir = command_to_direction(cmd);

        // Effectuer le mouvement dans la direction demandée
        // game_move() retourne true si au moins une tuile a été déplacée
        bool moved = game_move(&shared_game_state, dir);

        // Si un mouvement a été effectué, ajouter une nouvelle tuile aléatoire
        // C'est une règle du jeu 2048 : après chaque mouvement réussi, une nouvelle tuile apparaît
        if (moved)
        {
            game_add_random_tile(&shared_game_state);
        }

        StateMessage msg;
        msg.state = local_state;

        // Envoyer l'état mis à jour
        if (write(state_fd, &msg, sizeof(msg)) == -1)
        {
            perror("write state_fd");
            break;
        }
    }

    return NULL;
}

/*

// Indiquer que le thread est prêt
move_thread_ready = true;

// Boucle principale du thread
while (read(cmd_pipe_fd, &cmd, sizeof(cmd)) > 0)
{
    // Verrouiller le mutex pour accéder aux variables partagées
    pthread_mutex_lock(&game_mutex);
    pthread_cond_wait(&game)

        // Attendre qu'une commande soit disponible
        // pthread_cond_wait() déverrouille automatiquement le mutex et attend le signal
        while (!command_ready && game_running)
    {
        pthread_cond_wait(&move_cond, &game_mutex);
    }

    // Vérifier si on doit s'arrêter
    if (!game_running)
    {
        pthread_mutex_unlock(&game_mutex);
        break;
    }

    // Récupérer la commande et réinitialiser le flag
    CommandType cmd = pending_command;
    command_ready = false;

    // Déverrouiller le mutex avant le traitement (pour ne pas bloquer les autres threads)
    pthread_mutex_unlock(&game_mutex);

    // Gérer la commande de sortie
    if (cmd == CMD_QUIT)
    {
        pthread_mutex_lock(&game_mutex);
        game_running = false;            // Arrêter tous les threads
        pthread_cond_signal(&goal_cond); // Réveiller le thread Goal pour qu'il se termine
        pthread_mutex_unlock(&game_mutex);
        break;
    }

    // ============================================
    // TRAITEMENT DU MOUVEMENT
    // ============================================
    // Verrouiller à nouveau pour modifier l'état du jeu
    pthread_mutex_lock(&game_mutex);

    // Convertir la commande en direction de mouvement
    MoveDirection dir = command_to_direction(cmd);

    // Effectuer le mouvement dans la direction demandée
    // game_move() retourne true si au moins une tuile a été déplacée
    bool moved = game_move(&shared_game_state, dir);

    // Si un mouvement a été effectué, ajouter une nouvelle tuile aléatoire
    // C'est une règle du jeu 2048 : après chaque mouvement réussi, une nouvelle tuile apparaît
    if (moved)
    {
        game_add_random_tile(&shared_game_state);
    }

    // Réveiller le thread Goal pour qu'il vérifie l'état du jeu et envoie la mise à jour
    pthread_cond_signal(&goal_cond);

    // Déverrouiller le mutex
    pthread_mutex_unlock(&game_mutex);
}

return NULL;
}
*/

// ============================================
// THREAD GOAL
// ============================================

/**
 * Thread responsable de la vérification des conditions de fin de partie
 * et de l'envoi des mises à jour au processus d'affichage
 *
 * Ce thread :
 * 1. Attend qu'un mouvement soit effectué (signal du thread Move&Score)
 * 2. Vérifie si la partie est gagnée (tuile 2048 atteinte)
 * 3. Vérifie si la partie est perdue (plus de mouvements possibles)
 * 4. Envoie l'état du jeu au processus d'affichage via le pipe anonyme
 * 5. Envoie un signal au processus d'affichage pour le réveiller
 *
 * @param arg : Paramètre non utilisé (NULL)
 * @return : Toujours NULL
 */
void *goal_thread(void *arg)
{
    (void)arg;

    GameState received_state;

    goal_thread_ready = true;

    while (game_running)
    {
        // 1️ Attendre un nouvel état depuis le Move thread
        ssize_t n = read(move_goal_pipe[0], &received_state, sizeof(GameState));

        if (n <= 0)
        {
            perror("read from move_goal_pipe");
            break;
        }

        // 2️ Vérification des conditions de fin
        if (received_state.status == GAME_PLAYING)
        {
            if (game_is_won(&received_state))
            {
                received_state.status = GAME_WON;
            }
            else if (game_is_lost(&received_state))
            {
                received_state.status = GAME_LOST;
            }
        }

        // 3️ Préparer le message pour le display
        DisplayMessage display_msg;
        display_msg.state = received_state;
        display_msg.game_over = (received_state.status != GAME_PLAYING);

        // 4️ Envoyer au display
        ssize_t written = write(display_pipe[1], &display_msg, sizeof(DisplayMessage));
        if (written == -1)
        {
            perror("write to display pipe");
        }

        // 5️ Réveiller le processus display
        if (display_pid > 0)
        {
            kill(display_pid, SIG_UPDATE_DISPLAY);
        }

        // 6️ Si game over → arrêt
        if (display_msg.game_over)
        {
            game_running = false;

            if (display_pid > 0)
            {
                kill(display_pid, SIG_GAME_OVER);
            }

            break;
        }
    }

    return NULL;
}

// ============================================
// HANDLER DE SIGNAL
// ============================================

/**
 * Handler de signal pour l'arrêt propre du système
 *
 * Appelé lors de la réception de SIGINT (Ctrl+C) ou SIGTERM.
 * Arrête proprement tous les threads et le processus d'affichage.
 *
 * @param sig : Numéro du signal reçu (non utilisé)
 */
void cleanup_handler(int sig)
{
    (void)sig;            // Éviter l'avertissement du compilateur
    game_running = false; // Arrêter tous les threads

    // Envoyer SIGTERM au processus d'affichage pour qu'il se termine proprement
    if (display_pid > 0)
    {
        kill(display_pid, SIGTERM);
    }
}

/**
 * Fonction principale du processus game2048
 *
 * Cette fonction :
 * 1. Initialise le système (générateur aléatoire, pipes, processus)
 * 2. Crée le processus d'affichage (fork + execl)
 * 3. Initialise l'état du jeu
 * 4. Crée les threads Move&Score et Goal
 * 5. Lit les commandes depuis le pipe nommé et les transmet aux threads
 * 6. Gère l'arrêt propre du système
 */
int main()
{
    srand(time(NULL));

    // =============================
    // PIPE MOVE → GOAL
    // =============================
    if (pipe(move_goal_pipe) == -1)
    {
        perror("pipe move_goal");
        exit(EXIT_FAILURE);
    }

    // =============================
    // PIPE GOAL → DISPLAY
    // =============================
    if (pipe(display_pipe) == -1)
    {
        perror("pipe display");
        exit(EXIT_FAILURE);
    }

    // =============================
    // FORK DISPLAY
    // =============================
    display_pid = fork();
    if (display_pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (display_pid == 0)
    {
        close(display_pipe[1]);
        dup2(display_pipe[0], STDIN_FILENO);
        close(display_pipe[0]);

        execl("./bin/display", "display", NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    }

    close(display_pipe[0]); // parent écrit seulement

    // =============================
    // PIPE MAIN → MOVE
    // =============================
    if (pipe(cmd_pipe) == -1)
    {
        perror("pipe cmd");
        exit(EXIT_FAILURE);
    }

    // =============================
    // HANDLERS
    // =============================
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);

    // =============================
    // THREADS
    // =============================
    pthread_t move_thread, goal_thread_id;

    if (pthread_create(&move_thread, NULL, move_score_thread, NULL) != 0)
    {
        perror("pthread_create move");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&goal_thread_id, NULL, goal_thread, NULL) != 0)
    {
        perror("pthread_create goal");
        exit(EXIT_FAILURE);
    }

    // =============================
    // PIPE NOMMÉ (main → 2048)
    // =============================
    if (mkfifo(NAMED_PIPE_MAIN_TO_2048, 0666) == -1)
    {
        if (errno != EEXIST)
        {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }

    int named_pipe_fd = open(NAMED_PIPE_MAIN_TO_2048, O_RDONLY);
    if (named_pipe_fd == -1)
    {
        perror("open named pipe");
        exit(EXIT_FAILURE);
    }

    // =============================
    // BOUCLE DE COMMANDES
    // =============================
    CommandMessage cmd_msg;

    while (game_running)
    {
        ssize_t bytes_read =
            read(named_pipe_fd, &cmd_msg, sizeof(CommandMessage));

        if (bytes_read <= 0)
            break;

        if (bytes_read != sizeof(CommandMessage))
            continue;

        // Si quit → envoyer au move thread
        write(cmd_pipe[1], &cmd_msg.command, sizeof(cmd_msg.command));

        if (cmd_msg.command == CMD_QUIT)
            break;
    }

    // =============================
    // CLEAN EXIT
    // =============================
    close(cmd_pipe[1]);       // débloque move
    close(move_goal_pipe[1]); // débloque goal
    close(display_pipe[1]);

    pthread_join(move_thread, NULL);
    pthread_join(goal_thread_id, NULL);

    close(named_pipe_fd);

    waitpid(display_pid, NULL, 0);

    return 0;
}
