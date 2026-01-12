#ifndef IPC_H
#define IPC_H

#include "game_state.h"

// Types de commandes depuis le processus main
typedef enum {
    CMD_MOVE_UP,
    CMD_MOVE_DOWN,
    CMD_MOVE_LEFT,
    CMD_MOVE_RIGHT,
    CMD_QUIT
} CommandType;

// Message simple pour le pipe nommé (main -> 2048)
typedef struct {
    CommandType command;
} CommandMessage;

// Message pour le pipe anonyme (2048 -> affichage)
typedef struct {
    GameState state;
    bool game_over;
} DisplayMessage;

#endif // IPC_H

