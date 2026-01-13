# Documentation : display_grid.c

## Vue d'ensemble

Le fichier `display_grid.c` implémente le **système d'affichage de la grille** du jeu 2048. Il gère le formatage visuel de la grille dans le terminal avec des bordures ASCII.

## Rôle principal

Ce module est responsable de :
- **Maintenir** une représentation formatée de la grille en ASCII
- **Mettre à jour** la grille avec les nouvelles valeurs
- **Centrer** les valeurs numériques dans leurs cellules
- **Fournir** la grille formatée pour l'affichage

## Structure de la grille

La grille est représentée comme une chaîne de caractères statique avec :
- **Bordures** : `+------+------+------+------+`
- **Lignes de cellules** : `|      |      |      |      |`
- **Format** : 4 lignes de bordures + 4 lignes de cellules = 9 lignes au total
- **Largeur de ligne** : 30 caractères (LINE_LEN)
- **Largeur de cellule** : 7 caractères (CELL_WIDTH = 7, dont 6 espaces pour le contenu)

## Fonctions principales

### `update_grid()`
Met à jour la grille avec les nouvelles valeurs :
1. Réinitialise la grille avec le template (toutes les cellules vides)
2. Parcourt la grille de jeu
3. Pour chaque case non vide :
   - Convertit la valeur en chaîne avec `snprintf()`
   - Appelle `set_cell()` pour placer la valeur dans la cellule formatée

### `set_cell()`
Place une valeur dans une cellule spécifique de la grille formatée :
- **Paramètres** : pointeur vers la grille, ligne, colonne, valeur (chaîne)
- **Calcul de position** :
  - Détermine la ligne dans la grille formatée : `line = 1 + row * 2`
  - Calcule le début de la cellule dans la ligne : `cell_start_in_line = col * CELL_WIDTH + 1`
  - Centre la valeur : `offset = (cell_width - len) / 2`
  - Calcule l'index absolu : `index = line * (LINE_LEN + 1) + start_in_line`
- **Placement** : Copie la valeur centrée dans la cellule

### `get_formatted_grid()`
Retourne un pointeur vers la grille formatée (lecture seule).

### `print_grid()`
Affiche la grille formatée dans le terminal avec `printf()`.

### `update_game()`
Fonction de notification pour la fin de partie :
- Affiche "=== GAME OVER ===" si la partie est terminée
- Actuellement simple, peut être étendue pour d'autres notifications

## Calcul du centrage

Le centrage des valeurs dans les cellules :
- **Largeur disponible** : 6 caractères (entre les bordures `|`)
- **Offset de centrage** : `(6 - longueur_valeur) / 2`
- **Exemples** :
  - Valeur "2" (1 caractère) : offset = 2 → position 3 (centre)
  - Valeur "2048" (4 caractères) : offset = 1 → positions 2-5

## Points techniques

- Utilise des tableaux statiques pour stocker la grille et le template
- Le template est copié à chaque mise à jour pour réinitialiser la grille
- Les calculs de position prennent en compte les retours à la ligne (`LINE_LEN + 1`)
- Limite la longueur des valeurs à 6 caractères pour éviter le débordement
- Vérifie les limites du tableau avant d'écrire pour éviter les erreurs

## Format de sortie

La grille affichée ressemble à :
```
+------+------+------+------+
|   2  |      |   4  |      |
+------+------+------+------+
|      |   2  |   4  |   8  |
+------+------+------+------+
|  16  |   8  |      |      |
+------+------+------+------+
|      |      |   2  |   4  |
+------+------+------+------+
```
