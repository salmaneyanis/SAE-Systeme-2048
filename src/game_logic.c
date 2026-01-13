/*
 * game_logic.c
 * 
 * Ce fichier implémente toute la logique métier du jeu 2048.
 * Il contient les algorithmes de déplacement, de fusion, de génération
 * de tuiles et de vérification des conditions de fin de partie.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "game_logic.h"

// ============================================
// INITIALISATION DU JEU
// ============================================

/**
 * Initialise un nouvel état de jeu
 * 
 * Cette fonction :
 * 1. Remet à zéro la grille (toutes les cases à 0)
 * 2. Initialise le score à 0
 * 3. Met le statut à GAME_PLAYING
 * 4. Ajoute deux tuiles initiales aléatoirement
 * 
 * @param state : Pointeur vers l'état du jeu à initialiser
 */
void game_init(GameState* state) {
    // Remettre à zéro toute la grille (4x4 = 16 cases)
    memset(state->grid, 0, GRID_SIZE * GRID_SIZE * sizeof(int));
    
    // Initialiser les autres champs
    state->score = 0;              // Score initial à 0
    state->status = GAME_PLAYING;  // Partie en cours
    state->has_moved = false;      // Aucun mouvement effectué
    
    // Ajouter deux tuiles initiales (règle du jeu 2048)
    // Ces tuiles seront placées aléatoirement sur la grille
    game_add_random_tile(state);
    game_add_random_tile(state);
}

// ============================================
// GÉNÉRATION DE TUILES
// ============================================

/**
 * Trouve toutes les cases vides dans la grille
 * 
 * Cette fonction parcourt la grille et stocke les coordonnées de toutes
 * les cases vides (valeur 0) dans un tableau.
 * 
 * @param state : État du jeu à analyser
 * @param empty_cells : Tableau de sortie contenant les coordonnées [ligne, colonne]
 * @return : Nombre de cases vides trouvées
 */
static int get_empty_cells(const GameState* state, int empty_cells[GRID_SIZE * GRID_SIZE][2]) {
    int count = 0;
    
    // Parcourir toute la grille
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            // Si la case est vide (valeur 0), l'ajouter au tableau
            if (state->grid[i][j] == 0) {
                empty_cells[count][0] = i;  // Ligne
                empty_cells[count][1] = j;  // Colonne
                count++;
            }
        }
    }
    return count;
}

/**
 * Ajoute une nouvelle tuile aléatoirement sur une case vide
 * 
 * Cette fonction :
 * 1. Trouve toutes les cases vides
 * 2. Sélectionne une case aléatoirement
 * 3. Place une tuile de valeur 2 (90% de chance) ou 4 (10% de chance)
 * 
 * @param state : État du jeu à modifier
 * @return : true si une tuile a été ajoutée, false s'il n'y a plus de cases vides
 */
bool game_add_random_tile(GameState* state) {
    // Tableau pour stocker les coordonnées des cases vides
    int empty_cells[GRID_SIZE * GRID_SIZE][2];
    
    // Trouver toutes les cases vides
    int count = get_empty_cells(state, empty_cells);
    
    // Si aucune case vide, on ne peut pas ajouter de tuile
    if (count == 0) {
        return false;  // Plus de cases vides
    }
    
    // Sélectionner une case vide aléatoirement
    int index = rand() % count;
    int row = empty_cells[index][0];
    int col = empty_cells[index][1];
    
    // Placer une tuile : 90% de chance d'avoir 2, 10% de chance d'avoir 4
    // C'est la règle du jeu 2048 : les nouvelles tuiles sont généralement des 2
    state->grid[row][col] = (rand() % 10 == 0) ? 4 : 2;
    return true;
}

// ============================================
// ALGORITHMES DE MOUVEMENT
// ============================================

/**
 * Déplace une ligne vers la gauche et fusionne les tuiles identiques
 * 
 * C'est l'algorithme principal pour le mouvement. Il fonctionne en 3 étapes :
 * 1. Compaction : Déplace toutes les valeurs non nulles vers la gauche
 * 2. Fusion : Fusionne les tuiles adjacentes identiques (de gauche à droite)
 * 3. Recompaction : Compacte à nouveau après fusion
 * 
 * Exemple : [2, 0, 2, 4] → [4, 4, 0, 0]
 *   Étape 1 : [2, 2, 4, 0]
 *   Étape 2 : [4, 0, 4, 0] (les deux 2 fusionnent en 4)
 *   Étape 3 : [4, 4, 0, 0]
 * 
 * @param line : Tableau de 4 entiers représentant une ligne de la grille
 */
static void move_line_left(int line[GRID_SIZE]) {
    // ============================================
    // ÉTAPE 1 : COMPACTION VERS LA GAUCHE
    // ============================================
    // Déplacer toutes les valeurs non nulles vers la gauche
    // write_pos indique la prochaine position d'écriture
    int write_pos = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        if (line[i] != 0) {
            // Si la valeur n'est pas déjà à la bonne position, la déplacer
            if (write_pos != i) {
                line[write_pos] = line[i];
                line[i] = 0;
            }
            write_pos++;  // Passer à la position suivante
        }
    }
    
    // ============================================
    // ÉTAPE 2 : FUSION DES TUILES IDENTIQUES
    // ============================================
    // Fusionner les tuiles adjacentes identiques (de gauche à droite)
    // Important : on ne fusionne qu'une fois par mouvement (règle du 2048)
    for (int i = 0; i < GRID_SIZE - 1; i++) {
        if (line[i] != 0 && line[i] == line[i + 1]) {
            // Fusionner : doubler la valeur de gauche, vider la droite
            line[i] *= 2;
            line[i + 1] = 0;
        }
    }
    
    // ============================================
    // ÉTAPE 3 : RECOMPACTION APRÈS FUSION
    // ============================================
    // Après fusion, il peut y avoir des trous (valeurs 0)
    // On compacte à nouveau pour les éliminer
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

/**
 * Inverse une ligne (premier élément ↔ dernier élément)
 * 
 * Utilisé pour transformer un mouvement vers la gauche en mouvement vers la droite.
 * 
 * Exemple : [1, 2, 3, 4] → [4, 3, 2, 1]
 * 
 * @param line : Tableau de 4 entiers à inverser
 */
static void reverse_line(int line[GRID_SIZE]) {
    // Échanger les éléments symétriques par rapport au centre
    for (int i = 0; i < GRID_SIZE / 2; i++) {
        int temp = line[i];
        line[i] = line[GRID_SIZE - 1 - i];
        line[GRID_SIZE - 1 - i] = temp;
    }
}

/**
 * Transpose la grille (échange lignes et colonnes)
 * 
 * Utilisé pour transformer un mouvement horizontal en mouvement vertical.
 * 
 * Exemple :
 *   Avant :    Après :
 *   [1, 2]     [1, 3]
 *   [3, 4]     [2, 4]
 * 
 * @param grid : Grille 4x4 à transposer (modifiée en place)
 */
static void transpose_grid(int grid[GRID_SIZE][GRID_SIZE]) {
    // Parcourir seulement la moitié supérieure (évite les doubles échanges)
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = i + 1; j < GRID_SIZE; j++) {
            // Échanger grid[i][j] avec grid[j][i]
            int temp = grid[i][j];
            grid[i][j] = grid[j][i];
            grid[j][i] = temp;
        }
    }
}

/**
 * Effectue un mouvement dans la direction donnée
 * 
 * Cette fonction applique le mouvement à toute la grille en utilisant
 * l'algorithme move_line_left() et des transformations (transpose, reverse)
 * pour gérer les 4 directions.
 * 
 * Stratégie :
 * - GAUCHE : Applique move_line_left() directement sur chaque ligne
 * - DROITE : Inverse chaque ligne, applique move_line_left(), puis réinverse
 * - HAUT : Transpose, applique move_line_left() sur chaque ligne, puis transpose
 * - BAS : Transpose, inverse, applique move_line_left(), réinverse, puis transpose
 * 
 * @param state : État du jeu à modifier
 * @param dir : Direction du mouvement (UP, DOWN, LEFT, RIGHT)
 * @return : true si au moins une tuile a été déplacée, false sinon
 */
bool game_move(GameState* state, MoveDirection dir) {
    // Sauvegarder l'état avant mouvement pour détecter les changements
    int old_grid[GRID_SIZE][GRID_SIZE];
    memcpy(old_grid, state->grid, GRID_SIZE * GRID_SIZE * sizeof(int));
    
    // Ligne temporaire pour les transformations
    int temp_line[GRID_SIZE];
    
    // Appliquer le mouvement selon la direction
    switch (dir) {
        case MOVE_LEFT:
            // Pour chaque ligne, déplacer vers la gauche
            // C'est le cas le plus simple : on applique directement move_line_left()
            for (int i = 0; i < GRID_SIZE; i++) {
                memcpy(temp_line, state->grid[i], GRID_SIZE * sizeof(int));
                move_line_left(temp_line);
                memcpy(state->grid[i], temp_line, GRID_SIZE * sizeof(int));
            }
            break;
            
        case MOVE_RIGHT:
            // Pour chaque ligne, inverser, déplacer vers la gauche, puis réinverser
            // Cela transforme un mouvement vers la gauche en mouvement vers la droite
            for (int i = 0; i < GRID_SIZE; i++) {
                memcpy(temp_line, state->grid[i], GRID_SIZE * sizeof(int));
                reverse_line(temp_line);      // Inverser
                move_line_left(temp_line);    // Déplacer vers la gauche
                reverse_line(temp_line);      // Réinverser
                memcpy(state->grid[i], temp_line, GRID_SIZE * sizeof(int));
            }
            break;
            
        case MOVE_UP:
            // Transposer la grille (lignes ↔ colonnes)
            // Puis appliquer move_line_left() sur chaque ligne (qui sont maintenant les colonnes)
            // Puis transposer à nouveau
            transpose_grid(state->grid);
            for (int i = 0; i < GRID_SIZE; i++) {
                memcpy(temp_line, state->grid[i], GRID_SIZE * sizeof(int));
                move_line_left(temp_line);
                memcpy(state->grid[i], temp_line, GRID_SIZE * sizeof(int));
            }
            transpose_grid(state->grid);
            break;
            
        case MOVE_DOWN:
            // Transposer, inverser chaque ligne, déplacer vers la gauche, réinverser, transposer
            // Cela transforme un mouvement vers la gauche en mouvement vers le bas
            transpose_grid(state->grid);
            for (int i = 0; i < GRID_SIZE; i++) {
                memcpy(temp_line, state->grid[i], GRID_SIZE * sizeof(int));
                reverse_line(temp_line);      // Inverser
                move_line_left(temp_line);    // Déplacer vers la gauche
                reverse_line(temp_line);      // Réinverser
                memcpy(state->grid[i], temp_line, GRID_SIZE * sizeof(int));
            }
            transpose_grid(state->grid);
            break;
    }
    
    // ============================================
    // DÉTECTION DES CHANGEMENTS ET CALCUL DU SCORE
    // ============================================
    // Vérifier si le mouvement a changé quelque chose
    // memcmp() compare les deux grilles octet par octet
    state->has_moved = memcmp(old_grid, state->grid, GRID_SIZE * GRID_SIZE * sizeof(int)) != 0;
    
    // Calculer le nouveau score (somme de toutes les tuiles)
    // Le score est recalculé seulement si un mouvement a été effectué
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

// ============================================
// VÉRIFICATION DES CONDITIONS DE FIN DE PARTIE
// ============================================

/**
 * Vérifie si au moins un mouvement est possible
 * 
 * Un mouvement est possible si :
 * 1. Il y a au moins une case vide, OU
 * 2. Il y a au moins deux tuiles adjacentes identiques (horizontalement ou verticalement)
 * 
 * @param state : État du jeu à vérifier
 * @return : true si un mouvement est possible, false sinon
 */
bool game_can_move(const GameState* state) {
    // ============================================
    // VÉRIFICATION 1 : CASES VIDES
    // ============================================
    // S'il y a au moins une case vide, un mouvement est possible
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (state->grid[i][j] == 0) {
                return true;  // Au moins une case vide = mouvement possible
            }
        }
    }
    
    // ============================================
    // VÉRIFICATION 2 : TUILES ADJACENTES IDENTIQUES
    // ============================================
    // Si toutes les cases sont remplies, vérifier s'il y a des tuiles identiques adjacentes
    // On ne vérifie que vers la droite et vers le bas pour éviter les doublons
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int current = state->grid[i][j];
            
            // Vérifier à droite (même ligne, colonne suivante)
            if (j < GRID_SIZE - 1 && state->grid[i][j + 1] == current) {
                return true;  // Deux tuiles identiques côte à côte = fusion possible
            }
            
            // Vérifier en bas (ligne suivante, même colonne)
            if (i < GRID_SIZE - 1 && state->grid[i + 1][j] == current) {
                return true;  // Deux tuiles identiques l'une sur l'autre = fusion possible
            }
        }
    }
    
    // Aucun mouvement possible
    return false;
}

/**
 * Vérifie si la partie est gagnée
 * 
 * La partie est gagnée si une tuile de valeur WIN_VALUE (2048) est présente
 * sur la grille.
 * 
 * @param state : État du jeu à vérifier
 * @return : true si la partie est gagnée, false sinon
 */
bool game_is_won(const GameState* state) {
    // Parcourir toute la grille à la recherche d'une tuile 2048
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (state->grid[i][j] == WIN_VALUE) {
                return true;  // Tuile 2048 trouvée = victoire !
            }
        }
    }
    return false;  // Aucune tuile 2048 = partie non gagnée
}

/**
 * Vérifie si la partie est perdue
 * 
 * La partie est perdue si aucun mouvement n'est possible.
 * 
 * @param state : État du jeu à vérifier
 * @return : true si la partie est perdue, false sinon
 */
bool game_is_lost(const GameState* state) {
    // La partie est perdue si aucun mouvement n'est possible
    return !game_can_move(state);
}

// ============================================
// UTILITAIRES
// ============================================

/**
 * Copie l'état du jeu d'une structure à une autre
 * 
 * Utilisé pour envoyer une copie de l'état au processus d'affichage
 * sans risquer de modifier l'état original pendant l'affichage.
 * 
 * @param dest : Destination de la copie
 * @param src : Source de la copie
 */
void game_copy_state(GameState* dest, const GameState* src) {
    // Copier toute la structure octet par octet
    memcpy(dest, src, sizeof(GameState));
}

