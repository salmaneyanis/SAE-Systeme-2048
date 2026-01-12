#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>

#define GRID_SIZE 4
#define WIN_VALUE 2048

// Directions de mouvement
typedef enum {
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT
} MoveDirection;

// État de la partie
typedef enum {
    GAME_PLAYING,
    GAME_WON,
    GAME_LOST
} GameStatus;

// Structure pour l'état du jeu
typedef struct {
    int grid[GRID_SIZE][GRID_SIZE];
    int score;
    GameStatus status;
    bool has_moved;  // Indique si un mouvement a été effectué
} GameState;

// Structure pour les messages entre processus
typedef struct {
    GameState state;
    bool quit;
    bool new_move;
} GameMessage;

#endif // GAME_STATE_H

