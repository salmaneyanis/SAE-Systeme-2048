/*
 * game_logic.h
 * 
 * Ce fichier définit l'interface publique de la logique du jeu 2048.
 * Toutes les fonctions de manipulation de l'état du jeu sont déclarées ici.
 */

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "game_state.h"

// ============================================
// INITIALISATION
// ============================================

/**
 * Initialise un nouvel état de jeu
 * 
 * Remet à zéro la grille, initialise le score à 0, et ajoute deux tuiles initiales.
 * 
 * @param state : Pointeur vers l'état du jeu à initialiser
 */
void game_init(GameState* state);

// ============================================
// GÉNÉRATION DE TUILES
// ============================================

/**
 * Ajoute une nouvelle tuile aléatoirement sur une case vide
 * 
 * La tuile aura une valeur de 2 (90% de chance) ou 4 (10% de chance).
 * 
 * @param state : État du jeu à modifier
 * @return : true si une tuile a été ajoutée, false s'il n'y a plus de cases vides
 */
bool game_add_random_tile(GameState* state);

// ============================================
// MOUVEMENTS
// ============================================

/**
 * Effectue un mouvement dans la direction donnée
 * 
 * Cette fonction applique le mouvement à toute la grille, fusionne les tuiles
 * identiques adjacentes, et recalcule le score.
 * 
 * @param state : État du jeu à modifier
 * @param dir : Direction du mouvement (UP, DOWN, LEFT, RIGHT)
 * @return : true si le mouvement a été effectué (au moins une tuile déplacée), false sinon
 */
bool game_move(GameState* state, MoveDirection dir);

// ============================================
// VÉRIFICATIONS
// ============================================

/**
 * Vérifie si au moins un mouvement est possible
 * 
 * Un mouvement est possible s'il y a des cases vides ou des tuiles adjacentes identiques.
 * 
 * @param state : État du jeu à vérifier
 * @return : true si un mouvement est possible, false sinon
 */
bool game_can_move(const GameState* state);

/**
 * Vérifie si la partie est gagnée
 * 
 * La partie est gagnée si une tuile de valeur WIN_VALUE (2048) est présente.
 * 
 * @param state : État du jeu à vérifier
 * @return : true si la partie est gagnée, false sinon
 */
bool game_is_won(const GameState* state);

/**
 * Vérifie si la partie est perdue
 * 
 * La partie est perdue si aucun mouvement n'est possible.
 * 
 * @param state : État du jeu à vérifier
 * @return : true si la partie est perdue, false sinon
 */
bool game_is_lost(const GameState* state);

// ============================================
// UTILITAIRES
// ============================================

/**
 * Copie l'état du jeu d'une structure à une autre
 * 
 * @param dest : Destination de la copie
 * @param src : Source de la copie
 */
void game_copy_state(GameState* dest, const GameState* src);

#endif // GAME_LOGIC_H

