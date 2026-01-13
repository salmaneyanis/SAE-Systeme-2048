/*
 * display_process.c
 * 
 * Ce fichier implémente le processus d'affichage du jeu 2048.
 * Il reçoit l'état du jeu depuis le processus game2048 via un pipe anonyme
 * et l'affiche dans le terminal avec un formatage visuel.
 */

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

// Variable globale pour contrôler la boucle principale
static bool running = true;

/**
 * Handler pour le signal de mise à jour (SIG_UPDATE_DISPLAY)
 * 
 * Ce signal est envoyé par le processus game2048 pour réveiller ce processus
 * et lui indiquer qu'une nouvelle mise à jour est disponible.
 * Le signal lui-même ne fait rien, il sert juste à interrompre read().
 */
void update_handler(int sig) {
    // Le signal sert juste à réveiller le processus (interrompre read())
    (void)sig;  // Éviter l'avertissement du compilateur
}

/**
 * Handler pour le signal de fin de jeu (SIG_GAME_OVER)
 * 
 * Ce signal indique que la partie est terminée et que le processus
 * doit se terminer proprement.
 */
void game_over_handler(int sig) {
    (void)sig;  // Éviter l'avertissement du compilateur
    running = false;  // Arrêter la boucle principale
}

/**
 * Fonction principale du processus d'affichage
 * 
 * Ce processus :
 * 1. Configure les handlers de signaux
 * 2. Lit les messages depuis stdin (pipe anonyme)
 * 3. Affiche la grille et l'état du jeu
 * 4. Gère les messages de fin de partie
 */
int main() {
    // ============================================
    // ÉTAPE 1 : Configuration des handlers de signaux
    // ============================================
    // Utiliser sigaction() plutôt que signal() pour plus de fiabilité.
    struct sigaction sa;
    
    // Handler pour les mises à jour d'affichage
    sa.sa_handler = update_handler;
    sigemptyset(&sa.sa_mask);  // Pas de masque de signaux
    sa.sa_flags = 0;           // Pas de flags spéciaux
    sigaction(SIG_UPDATE_DISPLAY, &sa, NULL);
    
    // Handler pour la fin de partie (utilise le même handler)
    sa.sa_handler = game_over_handler;
    sigaction(SIG_GAME_OVER, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);  // Aussi pour SIGTERM (arrêt propre)
    
    // Structure pour recevoir les messages du processus game2048
    DisplayMessage msg;
    
    // ============================================
    // ÉTAPE 2 : Boucle principale de lecture et affichage
    // ============================================
    while (running) {
        // Lire un message depuis stdin
        // Note : stdin a été redirigé vers le pipe anonyme par le processus parent.
        // read() est bloquant : il attend des données ou est interrompu par un signal.
        ssize_t bytes_read = read(STDIN_FILENO, &msg, sizeof(DisplayMessage));
        
        // Gestion des erreurs de lecture
        if (bytes_read == -1) {
            // Si l'erreur est EINTR, c'est une interruption par signal (normal).
            // On continue simplement la boucle pour relancer read().
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
        if (bytes_read != sizeof(DisplayMessage)) {
            continue;
        }
        
        // ============================================
        // ÉTAPE 3 : Affichage de l'état du jeu
        // ============================================
        
        // Effacer l'écran avec des codes ANSI
        // \033[2J : efface tout l'écran
        // \033[H  : repositionne le curseur en haut à gauche
        printf("\033[2J\033[H");
        
        // Afficher le titre et le score
        printf("=== 2048 ===\n");
        printf("Score: %d\n\n", msg.state.score);
        
        // Mettre à jour la grille formatée avec les nouvelles valeurs
        update_grid(msg.state.grid);
        
        // Afficher la grille formatée
        print_grid(get_formatted_grid());
        
        // ============================================
        // ÉTAPE 4 : Gestion de la fin de partie
        // ============================================
        // Vérifier le statut de la partie et afficher les messages appropriés
        if (msg.state.status == GAME_WON) {
            // Victoire : une tuile 2048 a été atteinte
            printf("\n=== VICTOIRE ! Vous avez atteint 2048 ! ===\n");
            update_game(true);  // Notifier le module d'affichage
            running = false;     // Arrêter la boucle
        } else if (msg.state.status == GAME_LOST) {
            // Défaite : plus aucun mouvement possible
            printf("\n=== DÉFAITE ! Plus aucun mouvement possible. ===\n");
            update_game(true);  // Notifier le module d'affichage
            running = false;     // Arrêter la boucle
        }
        
        // Forcer l'affichage immédiat (important pour l'affichage en temps réel)
        fflush(stdout);
    }
    
    return 0;
}
