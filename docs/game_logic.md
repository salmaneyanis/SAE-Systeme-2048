# Documentation : game_logic.c

## Vue d'ensemble

Le fichier `game_logic.c` implémente toute la **logique métier du jeu 2048**. Il contient les algorithmes de déplacement, de fusion, de génération de tuiles et de vérification des conditions de fin de partie.

## Rôle principal

Ce module est responsable de :
- **Initialiser** un nouvel état de jeu
- **Générer** des tuiles aléatoires (2 ou 4) sur des cases vides
- **Effectuer** les mouvements dans les 4 directions (haut, bas, gauche, droite)
- **Fusionner** les tuiles identiques adjacentes
- **Calculer** le score (somme de toutes les tuiles)
- **Vérifier** les conditions de victoire (tuile 2048) et de défaite (plus de mouvements possibles)

## Fonctions principales

### `game_init()`
Initialise un nouvel état de jeu :
- Remet à zéro la grille (toutes les cases à 0)
- Initialise le score à 0
- Met le statut à `GAME_PLAYING`
- Ajoute deux tuiles initiales aléatoirement

### `game_add_random_tile()`
Ajoute une nouvelle tuile sur une case vide :
- Trouve toutes les cases vides
- Sélectionne une case aléatoirement
- Place une tuile de valeur 2 (90% de chance) ou 4 (10% de chance)
- Retourne `false` s'il n'y a plus de cases vides

### `game_move()`
Effectue un mouvement dans une direction donnée :
- Sauvegarde l'état avant le mouvement
- Applique le mouvement selon la direction :
  - **GAUCHE** : Applique `move_line_left()` sur chaque ligne
  - **DROITE** : Inverse chaque ligne, applique `move_line_left()`, puis réinverse
  - **HAUT** : Transpose la grille, applique `move_line_left()` sur chaque ligne, puis transpose
  - **BAS** : Transpose, inverse, applique `move_line_left()`, réinverse, puis transpose
- Compare avec l'état précédent pour déterminer si un mouvement a eu lieu
- Recalcule le score si un mouvement a été effectué
- Retourne `true` si un mouvement a été effectué

### `game_can_move()`
Vérifie si au moins un mouvement est possible :
- Vérifie s'il y a des cases vides
- Vérifie s'il y a des tuiles adjacentes identiques (horizontalement ou verticalement)
- Retourne `true` si un mouvement est possible

### `game_is_won()`
Vérifie si la partie est gagnée :
- Parcourt la grille à la recherche d'une tuile de valeur `WIN_VALUE` (2048)
- Retourne `true` si une tuile 2048 est trouvée

### `game_is_lost()`
Vérifie si la partie est perdue :
- Utilise `game_can_move()` pour déterminer s'il reste des mouvements possibles
- Retourne `true` si aucun mouvement n'est possible

## Algorithmes de mouvement

### `move_line_left()`
Algorithme principal pour déplacer et fusionner une ligne vers la gauche :

1. **Étape 1 - Compaction** : Déplace toutes les valeurs non nulles vers la gauche
   - Parcourt la ligne de gauche à droite
   - Pour chaque valeur non nulle, la place à la position `write_pos`
   - Incrémente `write_pos` après chaque placement

2. **Étape 2 - Fusion** : Fusionne les tuiles adjacentes identiques
   - Parcourt la ligne de gauche à droite
   - Si deux tuiles adjacentes ont la même valeur, les fusionne :
     - Double la valeur de la tuile de gauche
     - Met la tuile de droite à 0

3. **Étape 3 - Recompaction** : Compacte à nouveau après fusion
   - Répète l'étape 1 pour déplacer les tuiles après fusion

### Transformations pour les autres directions

- **DROITE** : Inverse la ligne, applique `move_line_left()`, puis réinverse
- **HAUT** : Transpose la grille (lignes ↔ colonnes), applique `move_line_left()` sur chaque ligne, puis transpose
- **BAS** : Transpose, inverse chaque ligne, applique `move_line_left()`, réinverse, puis transpose

### `transpose_grid()`
Transpose la grille (échange lignes et colonnes) :
- Parcourt la moitié supérieure de la grille
- Échange `grid[i][j]` avec `grid[j][i]`

### `reverse_line()`
Inverse une ligne :
- Échange les éléments symétriques par rapport au centre

## Calcul du score

Le score est calculé comme la **somme de toutes les tuiles** présentes sur la grille. Il est recalculé après chaque mouvement réussi.

## Points techniques

- Utilise `memcpy()` pour sauvegarder et comparer les états
- Utilise `memcmp()` pour détecter si un mouvement a changé la grille
- Utilise `rand()` pour la génération aléatoire (initialisé dans `game_2048.c`)
- Les fonctions de transformation (transpose, reverse) sont appliquées en place pour éviter les copies inutiles
- L'algorithme de mouvement garantit qu'une tuile ne peut fusionner qu'une seule fois par mouvement
