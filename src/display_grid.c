/*
 * display_grid.c
 * 
 * Ce fichier implémente le système d'affichage de la grille du jeu 2048.
 * Il gère le formatage visuel de la grille dans le terminal avec des bordures ASCII.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "display_grid.h"
#include "game_state.h"

// ============================================
// CONSTANTES DE FORMATAGE
// ============================================

// Longueur d'une ligne de la grille formatée (30 caractères)
#define LINE_LEN 30

// Largeur d'une cellule (7 caractères : | + 6 espaces + |)
#define CELL_WIDTH 7

// ============================================
// STRUCTURE DE LA GRILLE FORMATÉE
// ============================================

// Grille d'affichage statique (modifiée à chaque mise à jour)
// Format : 9 lignes au total
// - Lignes impaires (0, 2, 4, 6, 8) : Bordures "+------+------+------+------+"
// - Lignes paires (1, 3, 5, 7) : Cellules "|      |      |      |      |"
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

// Template de la grille (utilisé pour réinitialiser la grille à chaque mise à jour)
// Toutes les cellules sont vides (remplies d'espaces)
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

// ============================================
// FONCTIONS DE MISE À JOUR DE LA GRILLE
// ============================================

/**
 * Place une valeur dans une cellule spécifique de la grille formatée
 * 
 * Cette fonction calcule la position exacte dans la chaîne de caractères
 * de la grille et centre la valeur dans la cellule.
 * 
 * Structure d'une ligne de cellule :
 *   |      |      |      |      |
 *   012345678901234567890123456789
 *   ^     ^     ^     ^     ^
 *   |     |     |     |     |
 *   |     |     |     |     +-- Colonne 3 (position 21)
 *   |     |     |     +-------- Colonne 2 (position 14)
 *   |     |     +-------------- Colonne 1 (position 7)
 *   |     +-------------------- Colonne 0 (position 0)
 *   +-------------------------- Début de ligne
 * 
 * @param grid_ptr : Pointeur vers la grille formatée à modifier
 * @param row : Numéro de ligne (0-3)
 * @param col : Numéro de colonne (0-3)
 * @param value : Chaîne de caractères à placer dans la cellule
 */
void set_cell(char *grid_ptr, int row, int col, const char *value) {
    // Calculer la ligne dans la grille formatée
    // Les lignes avec des cellules (contenant '|') sont aux indices 1, 3, 5, 7
    // Formule : ligne_formatée = 1 + row * 2
    int line = 1 + row * 2;
    
    // Longueur de la valeur à afficher
    int len = strlen(value);
    
    // ============================================
    // CALCUL DE LA POSITION DANS LA LIGNE
    // ============================================
    // Chaque cellule fait 7 caractères : | (0) + 6 espaces (1-6) + | (7)
    // Les espaces utilisables sont aux positions col*7+1 à col*7+6 dans la ligne
    int cell_start_in_line = col * CELL_WIDTH + 1;  // Début de la zone de texte
    int cell_width = 6;  // Largeur disponible pour le texte (6 espaces)
    
    // Limiter la longueur à la largeur de la cellule pour éviter le débordement
    if (len > cell_width) {
        len = cell_width;
    }
    
    // ============================================
    // CALCUL DU CENTRAGE
    // ============================================
    // Centrer la valeur dans la cellule
    // offset = (largeur_disponible - longueur_valeur) / 2
    // 
    // Exemples :
    // - len=1: offset = (6-1)/2 = 2 → position 3 (centre des 6 espaces)
    // - len=2: offset = (6-2)/2 = 2 → positions 3-4
    // - len=3: offset = (6-3)/2 = 1 → positions 2-4
    // - len=4: offset = (6-4)/2 = 1 → positions 2-5
    // - len=5: offset = (6-5)/2 = 0 → positions 1-5
    // - len=6: offset = (6-6)/2 = 0 → positions 1-6
    int offset = (cell_width - len) / 2;
    int start_in_line = cell_start_in_line + offset;
    
    // ============================================
    // CALCUL DE L'INDEX ABSOLU
    // ============================================
    // Calculer l'index absolu dans le tableau de caractères
    // Chaque ligne fait (LINE_LEN + 1) caractères (+1 pour le '\n')
    int index = line * (LINE_LEN + 1) + start_in_line;
    
    // ============================================
    // COPIE DE LA VALEUR
    // ============================================
    // Copier la valeur centrée dans la cellule
    // len est maintenant garanti <= cell_width, donc pas de débordement
    for (int i = 0; i < len; i++) {
        int pos = index + i;
        // Vérifier les limites pour éviter les erreurs
        if (pos >= 0 && pos < (int)sizeof(grid)) {
            grid_ptr[pos] = value[i];
        }
    }
}

// ============================================
// FONCTIONS PUBLIQUES
// ============================================

/**
 * Retourne un pointeur vers la grille formatée
 * 
 * @return : Pointeur vers la chaîne de caractères de la grille formatée
 */
const char* get_formatted_grid(void) {
    return grid;
}

/**
 * Affiche la grille formatée dans le terminal
 * 
 * @param grid_ptr : Pointeur vers la grille formatée à afficher
 */
void print_grid(const char *grid_ptr) {
    printf("%s", grid_ptr);
}

/**
 * Met à jour la grille formatée avec les nouvelles valeurs
 * 
 * Cette fonction :
 * 1. Réinitialise la grille avec le template (toutes les cellules vides)
 * 2. Parcourt la grille de jeu
 * 3. Pour chaque case non vide, convertit la valeur en chaîne et la place dans la cellule
 * 
 * @param new_grid : Grille 4x4 contenant les nouvelles valeurs du jeu
 */
void update_grid(const int new_grid[GRID_SIZE][GRID_SIZE]) {
    // ============================================
    // ÉTAPE 1 : RÉINITIALISATION
    // ============================================
    // Réinitialiser la grille avec le template
    // Cela efface toutes les valeurs précédentes et remet toutes les cellules vides
    memcpy(grid, grid_template, sizeof(grid));
    
    // ============================================
    // ÉTAPE 2 : MISE À JOUR DES VALEURS
    // ============================================
    // Buffer pour convertir les valeurs numériques en chaînes
    char value_str[8];
    
    // Parcourir toute la grille de jeu
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            // Si la case n'est pas vide (valeur != 0)
            if (new_grid[i][j] != 0) {
                // Convertir la valeur numérique en chaîne de caractères
                snprintf(value_str, sizeof(value_str), "%d", new_grid[i][j]);
                
                // Placer la valeur dans la cellule correspondante de la grille formatée
                set_cell(grid, i, j, value_str);
            }
            // Si la case est vide (valeur == 0), elle reste vide dans la grille formatée
        }
    }
}

/**
 * Notifie le module de la fin de partie
 * 
 * Cette fonction affiche un message lorsque la partie est terminée.
 * Elle peut être étendue pour d'autres notifications.
 * 
 * @param game_finished : true si la partie est terminée, false sinon
 */
void update_game(bool game_finished) {
    if (game_finished) {
        printf("=== GAME OVER ===\n");
    }
}
