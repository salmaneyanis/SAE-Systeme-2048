#include <stdio.h>
#include <string.h>

#define LINE_LEN 25
#define CELL_WIDTH 7


char grid[] =
"+------+------+------+------+ \n"
"|      |      |      |      |\n"
"+------+------+------+------+ \n"
"|      |      |      |      |\n"
"+------+------+------+------+ \n"
"|      |      |      |      |\n"
"+------+------+------+------+ \n"
"|      |      |      |      |\n"
"+------+------+------+------+ \n";



void set_cell(char *grid, int row, int col, char value) {
    int line = 1 + row * 2;              // lignes avec '|'
    int pos  = col * CELL_WIDTH + 3;     // centre de la cellule

    int index = line * (LINE_LEN + 1) + pos;
    grid[index] = value;
}


void print_grid(const char *grid) {
    printf("%s", grid);
}

void update_grid(const char new_grid[]) {}

void update_game(bool game_finished) {
    if (game_finished) {
        printf("=== GAME OVER ===\n");
    }
}


