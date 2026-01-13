/*
 * display_grid.h
 * 
 * Ce fichier définit l'interface publique du module d'affichage de la grille.
 */

#ifndef DISPLAY_GRID_H
#define DISPLAY_GRID_H

#include <stdbool.h>
#include "game_state.h"

// ============================================
// FONCTIONS D'AFFICHAGE
// ============================================

/**
 * Retourne un pointeur vers la grille formatée
 * 
 * La grille est formatée avec des bordures ASCII et les valeurs centrées dans leurs cellules.
 * 
 * @return : Pointeur vers la chaîne de caractères de la grille formatée
 */
const char* get_formatted_grid(void);

/**
 * Affiche la grille formatée dans le terminal
 * 
 * @param grid : Pointeur vers la grille formatée à afficher
 */
void print_grid(const char grid[]);

/**
 * Met à jour la grille formatée avec les nouvelles valeurs
 * 
 * Cette fonction réinitialise la grille et place les valeurs de la grille de jeu
 * dans les cellules correspondantes, centrées.
 * 
 * @param new_grid : Grille 4x4 contenant les nouvelles valeurs du jeu
 */
void update_grid(const int new_grid[GRID_SIZE][GRID_SIZE]);

/**
 * Notifie le module de la fin de partie
 * 
 * Cette fonction affiche un message lorsque la partie est terminée.
 * 
 * @param game_finished : true si la partie est terminée, false sinon
 */
void update_game(bool game_finished);

#endif // DISPLAY_GRID_H