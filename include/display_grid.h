#ifndef DISPLAY_GRID_H
#define DISPLAY_GRID_H

#include <stdbool.h>
#include "game_state.h"

const char* get_formatted_grid(void); // Retourne un pointeur vers la grille formatée

void print_grid(const char grid[]); // Affichera la grille dans le terminal

void update_grid(const int new_grid[GRID_SIZE][GRID_SIZE]); // fonction qui recevra la grille à jour

void update_game(bool game_finished); // fonction qui recevra les infos du déroulement de la partie pour savoir si elle est terminé ou pas

#endif // DISPLAY_GRID_H