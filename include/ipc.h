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
    CMD_MOVE_UP,      // Commande de mouvement vers le haut
    CMD_MOVE_DOWN,    // Commande de mouvement vers le bas
    CMD_MOVE_LEFT,    // Commande de mouvement vers la gauche
    CMD_MOVE_RIGHT,   // Commande de mouvement vers la droite
    CMD_QUIT          // Commande pour quitter le jeu
} CommandType;

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
    CommandType command;  // Type de commande à exécuter
} CommandMessage;

/**
 * Message envoyé depuis game2048 vers le processus display
 * 
 * Utilisé via le pipe anonyme.
 * game2048 écrit ce message, le processus display le lit depuis stdin (redirigé).
 */
typedef struct {
    GameState state;     // État complet du jeu (grille, score, statut)
    bool game_over;      // Indicateur si la partie est terminée
} DisplayMessage;

#endif // IPC_H

