/*
 * common.h
 * 
 * Ce fichier définit les constantes communes utilisées pour la communication
 * inter-processus dans le jeu 2048.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

// ============================================
// PIPES
// ============================================

// Nom du pipe nommé (FIFO) pour la communication entre le processus main et game2048
// Le processus main écrit dans ce pipe, game2048 lit depuis ce pipe
#define NAMED_PIPE_MAIN_TO_2048 "/tmp/2048_main_pipe"

// Taille du pipe anonyme (utilisé pour dimensionner les buffers si nécessaire)
#define ANONYMOUS_PIPE_SIZE 4096

// ============================================
// SIGNaux
// ============================================

// Signal personnalisé pour réveiller le processus d'affichage
// Envoyé par game2048 au processus display lorsqu'une mise à jour est disponible
#define SIG_UPDATE_DISPLAY SIGUSR1

// Signal personnalisé pour indiquer la fin de partie
// Envoyé par game2048 au processus display lorsque la partie est terminée
#define SIG_GAME_OVER SIGUSR2

#endif // COMMON_H

