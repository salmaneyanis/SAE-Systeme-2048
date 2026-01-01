#ifndef DISPLAY_GRID_H
#define DISPLAY_GRID_H
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool


 
void print_grid(char grid[]); // Affichera la grille dans le terminal

void update_grid(char new_grid[]);// fonction qui recevra la grille à jour du thread Main

void update_game(bool game_finished);// fonction qui recevra les infos du déroulement de la partie pour savoir si elle est terminé ou pas du thread Goal

#endif // DISPLAY_GRID_H