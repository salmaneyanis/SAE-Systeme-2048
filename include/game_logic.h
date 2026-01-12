#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "game_state.h"

// Initialise un nouvel état de jeu
void game_init(GameState* state);

// Ajoute une nouvelle tuile (2 ou 4) aléatoirement sur une case vide
bool game_add_random_tile(GameState* state);

// Effectue un mouvement dans la direction donnée
// Retourne true si le mouvement a été effectué (au moins une tuile déplacée)
bool game_move(GameState* state, MoveDirection dir);

// Vérifie si un mouvement est possible
bool game_can_move(const GameState* state);

// Vérifie si la partie est gagnée (tuile 2048 présente)
bool game_is_won(const GameState* state);

// Vérifie si la partie est perdue (aucun mouvement possible)
bool game_is_lost(const GameState* state);

// Copie l'état du jeu
void game_copy_state(GameState* dest, const GameState* src);

#endif // GAME_LOGIC_H

