/*
 * ipc.h
 * 
 * Ce fichier définit les structures de communication inter-processus (IPC).
 * Il contient les types de messages échangés entre les différents processus.
 */

#ifndef IPC_H
#define IPC_H

#include "game_state.h"

// ============================================
// TYPES DE COMMANDES
// ============================================

/**
 * Types de commandes envoyées depuis le processus main vers game2048
 */
typedef enum {
    CMD_MOVE_UP,
    CMD_MOVE_DOWN,
    CMD_MOVE_LEFT,
    CMD_MOVE_RIGHT,
    CMD_QUIT,
    CMD_NEW_GAME
} CommandType;

#define MAX_GAMES 8

// ============================================
// STRUCTURES DE MESSAGES
// ============================================

/**
 * Message envoyé depuis le processus main vers game2048
 * 
 * Utilisé via le pipe nommé (/tmp/2048_main_pipe).
 * Le processus main écrit ce message, game2048 le lit.
 */
typedef struct {
    int         game_id;  //identifiant de la partie, -1 si pas encore attribue 
    CommandType command;  // Type de commande à exécuter
} CommandMessage;

//Ajout de game_id pour que la commande soit envoyé à la bonne partie car toutes les commandes sont envoyé dans le même pipes depuis le main

/**
 * Message envoyé depuis game2048 vers le processus display
 * 
 * Utilisé via le pipe anonyme.
 * game2048 écrit ce message, le processus display le lit depuis stdin (redirigé).
 */
typedef struct {
    int       game_id;
    GameState state;     // État complet du jeu (grille, score, statut)
    bool game_over;      // Indicateur si la partie est terminée
} DisplayMessage;

// reponse envoye par game2048 au main lors de l'enregistrement

typedef struct {
    int game_id;
} RegistrationReply;

#endif // IPC_H

