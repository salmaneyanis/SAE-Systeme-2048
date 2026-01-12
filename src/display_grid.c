#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "display_grid.h"
#include "game_state.h"

#define LINE_LEN 30
#define CELL_WIDTH 7

// Grille d'affichage statique
static char grid[] =
"+------+------+------+------+ \n"
"|      |      |      |      | \n"
"+------+------+------+------+ \n"
"|      |      |      |      | \n"
"+------+------+------+------+ \n"
"|      |      |      |      | \n"
"+------+------+------+------+ \n"
"|      |      |      |      | \n"
"+------+------+------+------+ \n";

static char grid_template[] =
"+------+------+------+------+ \n"
"|      |      |      |      | \n"
"+------+------+------+------+ \n"
"|      |      |      |      | \n"
"+------+------+------+------+ \n"
"|      |      |      |      | \n"
"+------+------+------+------+ \n"
"|      |      |      |      | \n"
"+------+------+------+------+ \n";

void set_cell(char *grid_ptr, int row, int col, const char *value) {
    int line = 1 + row * 2;              // lignes avec '|'
    int len = strlen(value);
    
    // Chaque cellule fait 7 caractères : | (0) + 6 espaces (1-6) + | (7)
    // Les espaces sont aux positions col*7+1 à col*7+6 dans la ligne
    int cell_start_in_line = col * CELL_WIDTH + 1;  // Début des espaces
    int cell_width = 6;  // Largeur de la zone de texte (6 espaces)
    
    // Limiter la longueur à la largeur de la cellule pour éviter le débordement
    if (len > cell_width) {
        len = cell_width;
    }
    
    // Centrer : offset = (cell_width - len) / 2
    // Pour len=1: offset = (6-1)/2 = 2 -> position 3 (centre des 6 espaces)
    // Pour len=2: offset = (6-2)/2 = 2 -> positions 3-4
    // Pour len=3: offset = (6-3)/2 = 1 -> positions 2-4
    // Pour len=4: offset = (6-4)/2 = 1 -> positions 2-5
    // Pour len=5: offset = (6-5)/2 = 0 -> positions 1-5
    // Pour len=6: offset = (6-6)/2 = 0 -> positions 1-6
    int offset = (cell_width - len) / 2;
    int start_in_line = cell_start_in_line + offset;
    
    // Calculer l'index absolu dans le tableau
    int index = line * (LINE_LEN + 1) + start_in_line;
    
    // Copier la valeur centrée dans la cellule
    // len est maintenant garanti <= cell_width, donc pas de débordement
    for (int i = 0; i < len; i++) {
        int pos = index + i;
        if (pos >= 0 && pos < (int)sizeof(grid)) {
            grid_ptr[pos] = value[i];
        }
    }
}

const char* get_formatted_grid(void) {
    return grid;
}

void print_grid(const char *grid_ptr) {
    printf("%s", grid_ptr);
}

void update_grid(const int new_grid[GRID_SIZE][GRID_SIZE]) {
    // Réinitialiser la grille avec le template
    memcpy(grid, grid_template, sizeof(grid));
    
    // Convertir les valeurs et les afficher
    char value_str[8];
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (new_grid[i][j] != 0) {
                snprintf(value_str, sizeof(value_str), "%d", new_grid[i][j]);
                set_cell(grid, i, j, value_str);
            }
        }
    }
}

void update_game(bool game_finished) {
    if (game_finished) {
        printf("=== GAME OVER ===\n");
    }
}
