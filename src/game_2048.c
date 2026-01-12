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

// Variables globales partagées entre threads
static GameState shared_game_state;
static pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t move_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t goal_cond = PTHREAD_COND_INITIALIZER;

static CommandType pending_command = CMD_QUIT;  // Initialisé à QUIT pour arrêt propre
static bool command_ready = false;
static bool game_running = true;
static bool move_thread_ready = false;
static bool goal_thread_ready = false;

// Descripteur du pipe anonyme vers le processus affichage
static int display_pipe[2];
static pid_t display_pid = 0;

// Convertir CommandType en MoveDirection
static MoveDirection command_to_direction(CommandType cmd) {
    switch (cmd) {
        case CMD_MOVE_UP: return MOVE_UP;
        case CMD_MOVE_DOWN: return MOVE_DOWN;
        case CMD_MOVE_LEFT: return MOVE_LEFT;
        case CMD_MOVE_RIGHT: return MOVE_RIGHT;
        default: return MOVE_UP;
    }
}

// Thread Move&Score
void* move_score_thread(void* arg) {
    (void)arg;
    move_thread_ready = true;
    
    while (game_running) {
        pthread_mutex_lock(&game_mutex);
        
        // Attendre une commande
        while (!command_ready && game_running) {
            pthread_cond_wait(&move_cond, &game_mutex);
        }
        
        if (!game_running) {
            pthread_mutex_unlock(&game_mutex);
            break;
        }
        
        CommandType cmd = pending_command;
        command_ready = false;
        
        pthread_mutex_unlock(&game_mutex);
        
        if (cmd == CMD_QUIT) {
            pthread_mutex_lock(&game_mutex);
            game_running = false;
            pthread_cond_signal(&goal_cond);
            pthread_mutex_unlock(&game_mutex);
            break;
        }
        
        // Effectuer le mouvement
        pthread_mutex_lock(&game_mutex);
        MoveDirection dir = command_to_direction(cmd);
        bool moved = game_move(&shared_game_state, dir);
        
        if (moved) {
            // Ajouter une nouvelle tuile
            game_add_random_tile(&shared_game_state);
        }
        
        // Réveiller le thread Goal
        pthread_cond_signal(&goal_cond);
        pthread_mutex_unlock(&game_mutex);
    }
    
    return NULL;
}

// Thread Goal
void* goal_thread(void* arg) {
    (void)arg;
    goal_thread_ready = true;
    
    while (game_running) {
        pthread_mutex_lock(&game_mutex);
        
        // Attendre qu'un mouvement soit effectué
        pthread_cond_wait(&goal_cond, &game_mutex);
        
        if (!game_running) {
            pthread_mutex_unlock(&game_mutex);
            break;
        }
        
        // Vérifier l'état du jeu seulement si on est encore en cours
        if (shared_game_state.status == GAME_PLAYING) {
            if (game_is_won(&shared_game_state)) {
                shared_game_state.status = GAME_WON;
            } else if (game_is_lost(&shared_game_state)) {
                shared_game_state.status = GAME_LOST;
            }
        }
        
        // Envoyer l'état au processus affichage
        DisplayMessage display_msg;
        game_copy_state(&display_msg.state, &shared_game_state);
        display_msg.game_over = (shared_game_state.status != GAME_PLAYING);
        
        pthread_mutex_unlock(&game_mutex);
        
        // Écrire dans le pipe anonyme
        ssize_t written = write(display_pipe[1], &display_msg, sizeof(DisplayMessage));
        if (written == -1) {
            perror("write to display pipe");
        }
        
        // Envoyer un signal au processus affichage pour le réveiller
        if (display_pid > 0) {
            kill(display_pid, SIG_UPDATE_DISPLAY);
        }
        
        if (display_msg.game_over) {
            game_running = false;
            if (display_pid > 0) {
                kill(display_pid, SIG_GAME_OVER);
            }
            break;
        }
    }
    
    return NULL;
}

// Handler de signal pour l'arrêt propre
void cleanup_handler(int sig) {
    (void)sig;
    game_running = false;
    if (display_pid > 0) {
        kill(display_pid, SIGTERM);
    }
}

int main() {
    // Initialiser le générateur aléatoire
    srand(time(NULL));
    
    // Créer le pipe anonyme vers le processus affichage
    if (pipe(display_pipe) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    // Créer le processus affichage
    display_pid = fork();
    if (display_pid == -1) {
        perror("fork display");
        exit(EXIT_FAILURE);
    }
    
    if (display_pid == 0) {
        // Processus enfant : affichage
        close(display_pipe[1]);  // Fermer l'extrémité d'écriture
        
        // Remplacer stdin par le pipe
        dup2(display_pipe[0], STDIN_FILENO);
        close(display_pipe[0]);
        
        // Exécuter le programme d'affichage
        char display_path[] = "./bin/display";
        execl(display_path, "display", NULL);
        perror("execl display");
        exit(EXIT_FAILURE);
    }
    
    // Processus parent : continuer
    close(display_pipe[0]);  // Fermer l'extrémité de lecture
    
    // Installer le handler de signal
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);
    
    // Initialiser l'état du jeu
    pthread_mutex_lock(&game_mutex);
    game_init(&shared_game_state);
    
    // Envoyer l'état initial au processus affichage
    DisplayMessage initial_msg;
    game_copy_state(&initial_msg.state, &shared_game_state);
    initial_msg.game_over = false;
    pthread_mutex_unlock(&game_mutex);
    
    // Attendre un peu pour que le processus display soit prêt
    {
        struct timespec ts = {0, 50000000};  // 50ms en nanoseconde
        nanosleep(&ts, NULL);
    }
    
    if (write(display_pipe[1], &initial_msg, sizeof(DisplayMessage)) == -1) {
        perror("write initial state");
    }
    kill(display_pid, SIG_UPDATE_DISPLAY);
    
    // Créer les threads
    pthread_t move_thread, goal_thread_id;
    
    if (pthread_create(&move_thread, NULL, move_score_thread, NULL) != 0) {
        perror("pthread_create move");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_create(&goal_thread_id, NULL, goal_thread, NULL) != 0) {
        perror("pthread_create goal");
        exit(EXIT_FAILURE);
    }
    
    // Attendre que les threads soient prêts
    while (!move_thread_ready || !goal_thread_ready) {
        struct timespec ts = {0, 10000000};  // 10ms en nanoseconde
        nanosleep(&ts, NULL);
    }
    
    // Créer le pipe nommé s'il n'existe pas
    if (mkfifo(NAMED_PIPE_MAIN_TO_2048, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }
    
    // Ouvrir le pipe nommé en lecture
    int named_pipe_fd = open(NAMED_PIPE_MAIN_TO_2048, O_RDONLY);
    if (named_pipe_fd == -1) {
        perror("open named pipe");
        exit(EXIT_FAILURE);
    }
    
    // Boucle principale : lire les commandes depuis le pipe nommé
    CommandMessage cmd_msg;
    while (game_running) {
        ssize_t bytes_read = read(named_pipe_fd, &cmd_msg, sizeof(CommandMessage));
        
        if (bytes_read == -1) {
            // Gérer l'interruption par signal (EINTR)
            if (errno == EINTR) {
                continue;  // Relancer read() après interruption par signal
            }
            perror("read from pipe");
            break;
        }
        
        if (bytes_read == 0) {
            // Pipe fermé
            break;
        }
        
        if (bytes_read != sizeof(CommandMessage)) {
            continue;
        }
        
        // Traiter la commande
        if (cmd_msg.command == CMD_QUIT) {
            game_running = false;
            pthread_mutex_lock(&game_mutex);
            pending_command = CMD_QUIT;
            command_ready = true;
            pthread_cond_signal(&move_cond);
            pthread_mutex_unlock(&game_mutex);
            break;
        }
        
        // Transmettre la commande au thread Move&Score
        pthread_mutex_lock(&game_mutex);
        pending_command = cmd_msg.command;
        command_ready = true;
        pthread_cond_signal(&move_cond);
        pthread_mutex_unlock(&game_mutex);
    }
    
    // Attendre la fin des threads
    pthread_join(move_thread, NULL);
    pthread_join(goal_thread_id, NULL);
    
    // Nettoyer
    close(named_pipe_fd);
    close(display_pipe[1]);
    
    // Attendre le processus affichage
    waitpid(display_pid, NULL, 0);
    
    return 0;
}
