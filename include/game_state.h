/*
 * game_state.h
 * 
 * Ce fichier définit les structures de données représentant l'état du jeu
 * et les types énumérés pour les directions et statuts de partie.
 */

#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>

// ============================================
// CONSTANTES
// ============================================

// Taille de la grille (4x4)
#define GRID_SIZE 4

// Valeur cible pour la victoire (une tuile doit atteindre cette valeur)
#define WIN_VALUE 2048

// ============================================
// TYPES ÉNUMÉRÉS
// ============================================

/**
 * Directions de mouvement possibles dans le jeu
 */
typedef enum {
    MOVE_UP,      // Mouvement vers le haut
    MOVE_DOWN,    // Mouvement vers le bas
    MOVE_LEFT,    // Mouvement vers la gauche
    MOVE_RIGHT    // Mouvement vers la droite
} MoveDirection;

/**
 * Statut de la partie
 */
typedef enum {
    GAME_PLAYING,  // Partie en cours
    GAME_WON,      // Partie gagnée (tuile 2048 atteinte)
    GAME_LOST      // Partie perdue (plus de mouvements possibles)
} GameStatus;

// ============================================
// STRUCTURES DE DONNÉES
// ============================================

/**
 * Structure principale représentant l'état complet du jeu
 */
typedef struct {
    int grid[GRID_SIZE][GRID_SIZE];  // Grille 4x4 contenant les valeurs des tuiles
    int score;                        // Score actuel (somme de toutes les tuiles)
    GameStatus status;                // Statut de la partie (PLAYING/WON/LOST)
    bool has_moved;                   // Indique si un mouvement a été effectué lors du dernier appel à game_move()
} GameState;

/**
 * Structure pour les messages entre processus
 * 
 * Note : Cette structure est définie mais n'est pas utilisée dans l'implémentation actuelle.
 * Les structures CommandMessage et DisplayMessage dans ipc.h sont utilisées à la place.
 */
typedef struct {
    GameState state;  // État du jeu
    bool quit;        // Indicateur de sortie
    bool new_move;    // Indicateur de nouveau mouvement
} GameMessage;

#endif // GAME_STATE_H

