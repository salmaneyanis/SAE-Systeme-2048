#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "common.h"
#include "game_state.h"
#include "display_grid.h"
#include "ipc.h"

static bool running = true;

// Handler pour le signal de mise à jour
void update_handler(int sig) {
    // Le signal sert juste à réveiller le processus
    (void)sig;
}

// Handler pour le signal de fin de jeu
void game_over_handler(int sig) {
    (void)sig;
    running = false;
}

int main() {
    // Installer les handlers de signaux avec sigaction pour plus de fiabilité
    struct sigaction sa;
    sa.sa_handler = update_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIG_UPDATE_DISPLAY, &sa, NULL);
    
    sa.sa_handler = game_over_handler;
    sigaction(SIG_GAME_OVER, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    
    DisplayMessage msg;
    
    while (running) {
        // Lire depuis stdin (qui est connecté au pipe anonyme)
        ssize_t bytes_read = read(STDIN_FILENO, &msg, sizeof(DisplayMessage));
        
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
        
        if (bytes_read != sizeof(DisplayMessage)) {
            continue;
        }
        
        // Effacer l'écran (code ANSI)
        printf("\033[2J\033[H");
        printf("=== 2048 ===\n");
        printf("Score: %d\n\n", msg.state.score);
        
        // Mettre à jour et afficher la grille
        update_grid(msg.state.grid);
        print_grid(get_formatted_grid());
        
        // Afficher l'état de la partie
        if (msg.state.status == GAME_WON) {
            printf("\n=== VICTOIRE ! Vous avez atteint 2048 ! ===\n");
            update_game(true);
            running = false;
        } else if (msg.state.status == GAME_LOST) {
            printf("\n=== DÉFAITE ! Plus aucun mouvement possible. ===\n");
            update_game(true);
            running = false;
        }
        
        fflush(stdout);
    }
    
    return 0;
}
