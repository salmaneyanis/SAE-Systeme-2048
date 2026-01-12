#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "game_logic.h"

// Initialise un nouvel état de jeu
void game_init(GameState* state) {
    memset(state->grid, 0, GRID_SIZE * GRID_SIZE * sizeof(int));
    state->score = 0;
    state->status = GAME_PLAYING;
    state->has_moved = false;
    
    // Ajouter deux tuiles initiales
    game_add_random_tile(state);
    game_add_random_tile(state);
}

// Trouve toutes les cases vides
static int get_empty_cells(const GameState* state, int empty_cells[GRID_SIZE * GRID_SIZE][2]) {
    int count = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (state->grid[i][j] == 0) {
                empty_cells[count][0] = i;
                empty_cells[count][1] = j;
                count++;
            }
        }
    }
    return count;
}

// Ajoute une nouvelle tuile (2 ou 4) aléatoirement sur une case vide
bool game_add_random_tile(GameState* state) {
    int empty_cells[GRID_SIZE * GRID_SIZE][2];
    int count = get_empty_cells(state, empty_cells);
    
    if (count == 0) {
        return false;  // Plus de cases vides
    }
    
    int index = rand() % count;
    int row = empty_cells[index][0];
    int col = empty_cells[index][1];
    
    // 90% de chance d'avoir 2, 10% de chance d'avoir 4
    state->grid[row][col] = (rand() % 10 == 0) ? 4 : 2;
    return true;
}

// Déplace une ligne vers la gauche et fusionne
static void move_line_left(int line[GRID_SIZE]) {
    // Étape 1: Déplacer toutes les valeurs non nulles vers la gauche
    int write_pos = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        if (line[i] != 0) {
            if (write_pos != i) {
                line[write_pos] = line[i];
                line[i] = 0;
            }
            write_pos++;
        }
    }
    
    // Étape 2: Fusionner les tuiles adjacentes identiques (de gauche à droite)
    for (int i = 0; i < GRID_SIZE - 1; i++) {
        if (line[i] != 0 && line[i] == line[i + 1]) {
            line[i] *= 2;
            line[i + 1] = 0;
        }
    }
    
    // Étape 3: Compacter à nouveau après fusion (déplacer toutes les valeurs non nulles vers la gauche)
    write_pos = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        if (line[i] != 0) {
            if (write_pos != i) {
                line[write_pos] = line[i];
                line[i] = 0;
            }
            write_pos++;
        }
    }
}

// Inverse une ligne
static void reverse_line(int line[GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE / 2; i++) {
        int temp = line[i];
        line[i] = line[GRID_SIZE - 1 - i];
        line[GRID_SIZE - 1 - i] = temp;
    }
}

// Transpose la grille
static void transpose_grid(int grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = i + 1; j < GRID_SIZE; j++) {
            int temp = grid[i][j];
            grid[i][j] = grid[j][i];
            grid[j][i] = temp;
        }
    }
}

// Effectue un mouvement dans la direction donnée
bool game_move(GameState* state, MoveDirection dir) {
    // Sauvegarder l'état avant mouvement
    int old_grid[GRID_SIZE][GRID_SIZE];
    memcpy(old_grid, state->grid, GRID_SIZE * GRID_SIZE * sizeof(int));
    
    int temp_line[GRID_SIZE];
    
    switch (dir) {
        case MOVE_LEFT:
            // Pour chaque ligne, déplacer vers la gauche
            for (int i = 0; i < GRID_SIZE; i++) {
                memcpy(temp_line, state->grid[i], GRID_SIZE * sizeof(int));
                move_line_left(temp_line);
                memcpy(state->grid[i], temp_line, GRID_SIZE * sizeof(int));
            }
            break;
            
        case MOVE_RIGHT:
            // Pour chaque ligne, inverser, déplacer vers la gauche, puis réinverser
            for (int i = 0; i < GRID_SIZE; i++) {
                memcpy(temp_line, state->grid[i], GRID_SIZE * sizeof(int));
                reverse_line(temp_line);
                move_line_left(temp_line);
                reverse_line(temp_line);
                memcpy(state->grid[i], temp_line, GRID_SIZE * sizeof(int));
            }
            break;
            
        case MOVE_UP:
            // Transposer, déplacer vers la gauche, puis transposer
            transpose_grid(state->grid);
            for (int i = 0; i < GRID_SIZE; i++) {
                memcpy(temp_line, state->grid[i], GRID_SIZE * sizeof(int));
                move_line_left(temp_line);
                memcpy(state->grid[i], temp_line, GRID_SIZE * sizeof(int));
            }
            transpose_grid(state->grid);
            break;
            
        case MOVE_DOWN:
            // Transposer, inverser, déplacer vers la gauche, réinverser, transposer
            transpose_grid(state->grid);
            for (int i = 0; i < GRID_SIZE; i++) {
                memcpy(temp_line, state->grid[i], GRID_SIZE * sizeof(int));
                reverse_line(temp_line);
                move_line_left(temp_line);
                reverse_line(temp_line);
                memcpy(state->grid[i], temp_line, GRID_SIZE * sizeof(int));
            }
            transpose_grid(state->grid);
            break;
    }
    
    // Vérifier si le mouvement a changé quelque chose
    state->has_moved = memcmp(old_grid, state->grid, GRID_SIZE * GRID_SIZE * sizeof(int)) != 0;
    
    // Calculer le nouveau score (somme de toutes les tuiles)
    if (state->has_moved) {
        state->score = 0;
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                state->score += state->grid[i][j];
            }
        }
    }
    
    return state->has_moved;
}

// Vérifie si un mouvement est possible
bool game_can_move(const GameState* state) {
    // Vérifier s'il y a des cases vides
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (state->grid[i][j] == 0) {
                return true;
            }
        }
    }
    
    // Vérifier s'il y a des tuiles adjacentes identiques
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int current = state->grid[i][j];
            // Vérifier à droite
            if (j < GRID_SIZE - 1 && state->grid[i][j + 1] == current) {
                return true;
            }
            // Vérifier en bas
            if (i < GRID_SIZE - 1 && state->grid[i + 1][j] == current) {
                return true;
            }
        }
    }
    
    return false;
}

// Vérifie si la partie est gagnée (tuile 2048 présente)
bool game_is_won(const GameState* state) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (state->grid[i][j] == WIN_VALUE) {
                return true;
            }
        }
    }
    return false;
}

// Vérifie si la partie est perdue (aucun mouvement possible)
bool game_is_lost(const GameState* state) {
    return !game_can_move(state);
}

// Copie l'état du jeu
void game_copy_state(GameState* dest, const GameState* src) {
    memcpy(dest, src, sizeof(GameState));
}

