# Documentation : Fichiers d'en-tête (Headers)

## Vue d'ensemble

Les fichiers d'en-tête définissent les structures de données, les constantes et les interfaces utilisées dans le projet. Ils permettent la séparation entre l'interface publique et l'implémentation.

---

## common.h

### Rôle
Définit les **constantes communes** utilisées pour la communication inter-processus.

### Définitions

- **NAMED_PIPE_MAIN_TO_2048** : Chemin du pipe nommé (`/tmp/2048_main_pipe`)
  - Utilisé pour la communication entre le processus `main` et `game2048`
  
- **ANONYMOUS_PIPE_SIZE** : Taille du pipe anonyme (4096 octets)
  - Utilisé pour dimensionner les buffers si nécessaire

- **SIG_UPDATE_DISPLAY** : Signal personnalisé (`SIGUSR1`)
  - Envoyé au processus d'affichage pour le réveiller et afficher une mise à jour

- **SIG_GAME_OVER** : Signal personnalisé (`SIGUSR2`)
  - Envoyé au processus d'affichage pour indiquer la fin de partie

---

## game_state.h

### Rôle
Définit les **structures de données** représentant l'état du jeu et les types énumérés pour les directions et statuts.

### Définitions

- **GRID_SIZE** : Taille de la grille (4x4)

- **WIN_VALUE** : Valeur cible pour la victoire (2048)

- **MoveDirection** : Énumération des directions de mouvement
  - `MOVE_UP`, `MOVE_DOWN`, `MOVE_LEFT`, `MOVE_RIGHT`

- **GameStatus** : Énumération des statuts de partie
  - `GAME_PLAYING` : Partie en cours
  - `GAME_WON` : Partie gagnée (tuile 2048 atteinte)
  - `GAME_LOST` : Partie perdue (plus de mouvements possibles)

- **GameState** : Structure principale de l'état du jeu
  - `grid[GRID_SIZE][GRID_SIZE]` : Grille 4x4 contenant les valeurs des tuiles
  - `score` : Score actuel (somme de toutes les tuiles)
  - `status` : Statut de la partie (PLAYING/WON/LOST)
  - `has_moved` : Indicateur booléen si un mouvement a été effectué

- **GameMessage** : Structure pour les messages entre processus (définie mais non utilisée dans l'implémentation actuelle)

---

## game_logic.h

### Rôle
Définit l'**interface publique** de la logique du jeu. Toutes les fonctions de manipulation de l'état du jeu sont déclarées ici.

### Fonctions déclarées

- **game_init()** : Initialise un nouvel état de jeu
- **game_add_random_tile()** : Ajoute une tuile aléatoire sur une case vide
- **game_move()** : Effectue un mouvement dans une direction
- **game_can_move()** : Vérifie si un mouvement est possible
- **game_is_won()** : Vérifie si la partie est gagnée
- **game_is_lost()** : Vérifie si la partie est perdue
- **game_copy_state()** : Copie l'état du jeu

---

## display_grid.h

### Rôle
Définit l'**interface publique** du module d'affichage de la grille.

### Fonctions déclarées

- **get_formatted_grid()** : Retourne un pointeur vers la grille formatée
- **print_grid()** : Affiche la grille dans le terminal
- **update_grid()** : Met à jour la grille avec de nouvelles valeurs
- **update_game()** : Notifie le module de la fin de partie

---

## ipc.h

### Rôle
Définit les **structures de communication inter-processus** (IPC - Inter-Process Communication).

### Définitions

- **CommandType** : Énumération des types de commandes
  - `CMD_MOVE_UP`, `CMD_MOVE_DOWN`, `CMD_MOVE_LEFT`, `CMD_MOVE_RIGHT`
  - `CMD_QUIT` : Commande pour quitter le jeu

- **CommandMessage** : Structure pour les messages du processus `main` vers `game2048`
  - `command` : Type de commande à exécuter
  - Utilisé via le pipe nommé `/tmp/2048_main_pipe`

- **DisplayMessage** : Structure pour les messages de `game2048` vers le processus `display`
  - `state` : État complet du jeu (GameState)
  - `game_over` : Indicateur booléen si la partie est terminée
  - Utilisé via le pipe anonyme

---

## Organisation des dépendances

```
common.h          (aucune dépendance)
    ↓
game_state.h      (dépend de common.h pour stdbool.h)
    ↓
game_logic.h      (dépend de game_state.h)
display_grid.h    (dépend de game_state.h)
ipc.h             (dépend de game_state.h)
```

Les fichiers sources incluent les headers nécessaires selon leurs besoins spécifiques.
