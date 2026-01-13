/*
 * game_2048.c
 * 
 * Ce fichier implémente le processus principal du jeu 2048.
 * Il coordonne la logique du jeu avec une architecture multithread :
 * - Thread Move&Score : Gère les mouvements et le calcul du score
 * - Thread Goal : Vérifie les conditions de victoire/défaite et envoie les mises à jour
 * - Thread principal : Lit les commandes et coordonne l'ensemble
 */

#define _POSIX_C_SOURCE 200809L  // Pour les fonctions POSIX (nanosleep, etc.)
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
static MoveDirection command_to_direction(CommandType cmd) {
    switch (cmd) {
        case CMD_MOVE_UP: return MOVE_UP;
        case CMD_MOVE_DOWN: return MOVE_DOWN;
        case CMD_MOVE_LEFT: return MOVE_LEFT;
        case CMD_MOVE_RIGHT: return MOVE_RIGHT;
        default: return MOVE_UP;  // Par défaut (ne devrait jamais arriver)
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
void* move_score_thread(void* arg) {
    (void)arg;  // Éviter l'avertissement du compilateur
    
    // Indiquer que le thread est prêt
    move_thread_ready = true;
    
    // Boucle principale du thread
    while (game_running) {
        // Verrouiller le mutex pour accéder aux variables partagées
        pthread_mutex_lock(&game_mutex);
        
        // Attendre qu'une commande soit disponible
        // pthread_cond_wait() déverrouille automatiquement le mutex et attend le signal
        while (!command_ready && game_running) {
            pthread_cond_wait(&move_cond, &game_mutex);
        }
        
        // Vérifier si on doit s'arrêter
        if (!game_running) {
            pthread_mutex_unlock(&game_mutex);
            break;
        }
        
        // Récupérer la commande et réinitialiser le flag
        CommandType cmd = pending_command;
        command_ready = false;
        
        // Déverrouiller le mutex avant le traitement (pour ne pas bloquer les autres threads)
        pthread_mutex_unlock(&game_mutex);
        
        // Gérer la commande de sortie
        if (cmd == CMD_QUIT) {
            pthread_mutex_lock(&game_mutex);
            game_running = false;  // Arrêter tous les threads
            pthread_cond_signal(&goal_cond);  // Réveiller le thread Goal pour qu'il se termine
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
        if (moved) {
            game_add_random_tile(&shared_game_state);
        }
        
        // Réveiller le thread Goal pour qu'il vérifie l'état du jeu et envoie la mise à jour
        pthread_cond_signal(&goal_cond);
        
        // Déverrouiller le mutex
        pthread_mutex_unlock(&game_mutex);
    }
    
    return NULL;
}

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
void* goal_thread(void* arg) {
    (void)arg;  // Éviter l'avertissement du compilateur
    
    // Indiquer que le thread est prêt
    goal_thread_ready = true;
    
    // Boucle principale du thread
    while (game_running) {
        // Verrouiller le mutex pour accéder à l'état du jeu
        pthread_mutex_lock(&game_mutex);
        
        // Attendre qu'un mouvement soit effectué
        // Le thread Move&Score enverra un signal via goal_cond après chaque mouvement
        pthread_cond_wait(&goal_cond, &game_mutex);
        
        // Vérifier si on doit s'arrêter
        if (!game_running) {
            pthread_mutex_unlock(&game_mutex);
            break;
        }
        
        // ============================================
        // VÉRIFICATION DES CONDITIONS DE FIN DE PARTIE
        // ============================================
        // Vérifier l'état du jeu seulement si on est encore en cours
        if (shared_game_state.status == GAME_PLAYING) {
            // Vérifier si la partie est gagnée (une tuile 2048 est présente)
            if (game_is_won(&shared_game_state)) {
                shared_game_state.status = GAME_WON;
            }
            // Sinon, vérifier si la partie est perdue (plus de mouvements possibles)
            else if (game_is_lost(&shared_game_state)) {
                shared_game_state.status = GAME_LOST;
            }
        }
        
        // ============================================
        // PRÉPARATION DU MESSAGE POUR L'AFFICHAGE
        // ============================================
        // Créer un message contenant l'état complet du jeu
        DisplayMessage display_msg;
        game_copy_state(&display_msg.state, &shared_game_state);
        display_msg.game_over = (shared_game_state.status != GAME_PLAYING);
        
        // Déverrouiller le mutex avant l'écriture dans le pipe (opération bloquante)
        pthread_mutex_unlock(&game_mutex);
        
        // ============================================
        // ENVOI AU PROCESSUS D'AFFICHAGE
        // ============================================
        // Écrire l'état du jeu dans le pipe anonyme
        // Le processus d'affichage lit depuis l'autre extrémité (stdin redirigé)
        ssize_t written = write(display_pipe[1], &display_msg, sizeof(DisplayMessage));
        if (written == -1) {
            perror("write to display pipe");
        }
        
        // Envoyer un signal au processus d'affichage pour le réveiller
        // Le processus d'affichage est bloqué sur read() et ce signal l'interrompt
        if (display_pid > 0) {
            kill(display_pid, SIG_UPDATE_DISPLAY);
        }
        
        // Si la partie est terminée, arrêter le système
        if (display_msg.game_over) {
            game_running = false;  // Arrêter tous les threads
            
            // Envoyer le signal de fin de partie au processus d'affichage
            if (display_pid > 0) {
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
void cleanup_handler(int sig) {
    (void)sig;  // Éviter l'avertissement du compilateur
    game_running = false;  // Arrêter tous les threads
    
    // Envoyer SIGTERM au processus d'affichage pour qu'il se termine proprement
    if (display_pid > 0) {
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
int main() {
    // ============================================
    // ÉTAPE 1 : INITIALISATION DU GÉNÉRATEUR ALÉATOIRE
    // ============================================
    // Le générateur aléatoire est utilisé pour placer les nouvelles tuiles
    srand(time(NULL));
    
    // ============================================
    // ÉTAPE 2 : CRÉATION DU PIPE ANONYME
    // ============================================
    // Le pipe anonyme permet la communication avec le processus d'affichage
    // display_pipe[0] = extrémité de lecture (pour le processus enfant)
    // display_pipe[1] = extrémité d'écriture (pour ce processus)
    if (pipe(display_pipe) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    // ============================================
    // ÉTAPE 3 : CRÉATION DU PROCESSUS D'AFFICHAGE
    // ============================================
    // Utiliser fork() pour créer un processus enfant
    display_pid = fork();
    if (display_pid == -1) {
        perror("fork display");
        exit(EXIT_FAILURE);
    }
    
    if (display_pid == 0) {
        // ============================================
        // PROCESSUS ENFANT : AFFICHAGE
        // ============================================
        // Fermer l'extrémité d'écriture (on ne fait que lire)
        close(display_pipe[1]);
        
        // Rediriger stdin vers le pipe
        // Le processus d'affichage lira depuis stdin (qui sera le pipe)
        dup2(display_pipe[0], STDIN_FILENO);
        close(display_pipe[0]);  // On peut fermer l'ancien descripteur
        
        // Remplacer ce processus par le programme d'affichage
        // execl() remplace l'image mémoire du processus actuel
        char display_path[] = "./bin/display";
        execl(display_path, "display", NULL);
        
        // Si on arrive ici, execl() a échoué
        perror("execl display");
        exit(EXIT_FAILURE);
    }
    
    // ============================================
    // PROCESSUS PARENT : CONTINUER
    // ============================================
    // Fermer l'extrémité de lecture (on ne fait qu'écrire)
    close(display_pipe[0]);
    
    // ============================================
    // ÉTAPE 4 : INSTALLATION DES HANDLERS DE SIGNAL
    // ============================================
    // Permettre un arrêt propre avec Ctrl+C ou SIGTERM
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);
    
    // ============================================
    // ÉTAPE 5 : INITIALISATION DE L'ÉTAT DU JEU
    // ============================================
    // Verrouiller le mutex pour initialiser l'état du jeu de manière thread-safe
    pthread_mutex_lock(&game_mutex);
    game_init(&shared_game_state);  // Initialise la grille et ajoute 2 tuiles
    
    // Préparer le message initial pour le processus d'affichage
    DisplayMessage initial_msg;
    game_copy_state(&initial_msg.state, &shared_game_state);
    initial_msg.game_over = false;
    pthread_mutex_unlock(&game_mutex);
    
    // ============================================
    // ÉTAPE 6 : ENVOI DE L'ÉTAT INITIAL
    // ============================================
    // Attendre un peu pour que le processus display soit prêt à recevoir
    {
        struct timespec ts = {0, 50000000};  // 50ms en nanoseconde
        nanosleep(&ts, NULL);
    }
    
    // Envoyer l'état initial au processus d'affichage
    if (write(display_pipe[1], &initial_msg, sizeof(DisplayMessage)) == -1) {
        perror("write initial state");
    }
    
    // Réveiller le processus d'affichage pour qu'il affiche l'état initial
    kill(display_pid, SIG_UPDATE_DISPLAY);
    
    // ============================================
    // ÉTAPE 7 : CRÉATION DES THREADS
    // ============================================
    pthread_t move_thread, goal_thread_id;
    
    // Créer le thread Move&Score
    // Ce thread traite les mouvements et calcule le score
    if (pthread_create(&move_thread, NULL, move_score_thread, NULL) != 0) {
        perror("pthread_create move");
        exit(EXIT_FAILURE);
    }
    
    // Créer le thread Goal
    // Ce thread vérifie les conditions de fin de partie et envoie les mises à jour
    if (pthread_create(&goal_thread_id, NULL, goal_thread, NULL) != 0) {
        perror("pthread_create goal");
        exit(EXIT_FAILURE);
    }
    
    // Attendre que les threads soient prêts (ils ont initialisé leurs flags)
    // Cette synchronisation garantit que les threads sont opérationnels avant de continuer
    while (!move_thread_ready || !goal_thread_ready) {
        struct timespec ts = {0, 10000000};  // 10ms en nanoseconde
        nanosleep(&ts, NULL);
    }
    
    // ============================================
    // ÉTAPE 8 : CRÉATION DU PIPE NOMmé
    // ============================================
    // Créer le pipe nommé pour recevoir les commandes du processus main
    // Le processus main l'ouvrira en écriture, ce processus en lecture
    if (mkfifo(NAMED_PIPE_MAIN_TO_2048, 0666) == -1) {
        // Si le pipe existe déjà, c'est normal (redémarrage)
        if (errno != EEXIST) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }
    
    // Ouvrir le pipe nommé en lecture (bloquant jusqu'à ce qu'un écrivain se connecte)
    int named_pipe_fd = open(NAMED_PIPE_MAIN_TO_2048, O_RDONLY);
    if (named_pipe_fd == -1) {
        perror("open named pipe");
        exit(EXIT_FAILURE);
    }
    
    // ============================================
    // ÉTAPE 9 : BOUCLE PRINCIPALE DE LECTURE DES COMMANDES
    // ============================================
    // Cette boucle lit les commandes depuis le processus main et les transmet aux threads
    CommandMessage cmd_msg;
    while (game_running) {
        // Lire une commande depuis le pipe nommé
        // read() est bloquant : il attend des données ou est interrompu par un signal
        ssize_t bytes_read = read(named_pipe_fd, &cmd_msg, sizeof(CommandMessage));
        
        // Gestion des erreurs de lecture
        if (bytes_read == -1) {
            // Si l'erreur est EINTR, c'est une interruption par signal (normal)
            if (errno == EINTR) {
                continue;  // Relancer read() après interruption par signal
            }
            // Sinon, c'est une vraie erreur
            perror("read from pipe");
            break;
        }
        
        // Si bytes_read == 0, le pipe a été fermé (fin de communication)
        if (bytes_read == 0) {
            break;
        }
        
        // Si on n'a pas lu un message complet, ignorer et continuer
        if (bytes_read != sizeof(CommandMessage)) {
            continue;
        }
        
        // ============================================
        // TRAITEMENT DE LA COMMANDE
        // ============================================
        // Gérer la commande de sortie
        if (cmd_msg.command == CMD_QUIT) {
            game_running = false;  // Arrêter tous les threads
            
            // Transmettre la commande au thread Move&Score pour qu'il se termine proprement
            pthread_mutex_lock(&game_mutex);
            pending_command = CMD_QUIT;
            command_ready = true;
            pthread_cond_signal(&move_cond);  // Réveiller le thread
            pthread_mutex_unlock(&game_mutex);
            break;
        }
        
        // Transmettre la commande au thread Move&Score
        // Le thread attendra sur move_cond et traitera la commande
        pthread_mutex_lock(&game_mutex);
        pending_command = cmd_msg.command;
        command_ready = true;
        pthread_cond_signal(&move_cond);  // Réveiller le thread Move&Score
        pthread_mutex_unlock(&game_mutex);
    }
    
    // ============================================
    // ÉTAPE 10 : NETTOYAGE ET ARRÊT PROPRE
    // ============================================
    // Attendre que les threads se terminent proprement
    // pthread_join() est bloquant : il attend la fin du thread
    pthread_join(move_thread, NULL);
    pthread_join(goal_thread_id, NULL);
    
    // Fermer les descripteurs de fichiers
    close(named_pipe_fd);      // Pipe nommé (communication avec main)
    close(display_pipe[1]);    // Pipe anonyme (communication avec display)
    
    // Attendre que le processus d'affichage se termine
    // waitpid() est bloquant : il attend la fin du processus enfant
    waitpid(display_pid, NULL, 0);
    
    return 0;
}
