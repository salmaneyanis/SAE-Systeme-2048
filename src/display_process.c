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
void update_handler(int sig)
{
    // Le signal sert juste à réveiller le processus (interrompre read())
    (void)sig; // Éviter l'avertissement du compilateur
}

/**
 * Handler pour le signal de fin de jeu (SIG_GAME_OVER)
 *
 * Ce signal indique que la partie est terminée et que le processus
 * doit se terminer proprement.
 */
void game_over_handler(int sig)
{
    (void)sig;       // Éviter l'avertissement du compilateur
    running = false; // Arrêter la boucle principale
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
int main()
{

    struct sigaction sa;

    // =============================
    // Handler UPDATE
    // =============================
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = update_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIG_UPDATE_DISPLAY, &sa, NULL) == -1)
    {
        perror("sigaction UPDATE");
        exit(EXIT_FAILURE);
    }

    // =============================
    // Handler GAME OVER
    // =============================
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = game_over_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIG_GAME_OVER, &sa, NULL) == -1)
    {
        perror("sigaction GAME_OVER");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        perror("sigaction SIGTERM");
        exit(EXIT_FAILURE);
    }

    DisplayMessage msg;

    while (running)
    {

        ssize_t bytes_read = read(STDIN_FILENO, &msg, sizeof(DisplayMessage));

        if (bytes_read == -1)
        {
            if (errno == EINTR)
                continue;

            perror("read from pipe");
            break;
        }

        if (bytes_read == 0)
            break;

        if (bytes_read != sizeof(DisplayMessage))
            continue;

        printf("\033[2J\033[H");
        printf("=== 2048 ===\n");
        printf("Score: %d\n\n", msg.state.score);

        update_grid(msg.state.grid);
        print_grid(get_formatted_grid());

        if (msg.state.status == GAME_WON)
        {
            printf("\n=== VICTOIRE ! Vous avez atteint 2048 ! ===\n");
            update_game(true);
            running = false;
        }
        else if (msg.state.status == GAME_LOST)
        {
            printf("\n=== DÉFAITE ! Plus aucun mouvement possible. ===\n");
            update_game(true);
            running = false;
        }

        fflush(stdout);
    }

    return 0;
}
