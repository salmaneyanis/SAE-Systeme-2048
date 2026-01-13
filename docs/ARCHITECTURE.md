# Architecture du Jeu 2048

## Vue d'ensemble

Le jeu 2048 est implémenté avec une **architecture multiprocessus et multithread** en C. Le système est composé de **3 processus principaux** qui communiquent via des **pipes** (nommés et anonymes) et utilisent des **signaux** pour la synchronisation.

## Architecture des Processus

```
┌─────────────────┐
│  Processus main │  (Saisie des commandes)
└────────┬────────┘
         │ Pipe nommé (FIFO)
         │ /tmp/2048_main_pipe
         │ (CommandMessage)
         ▼
┌─────────────────┐
│ Processus game2048│  (Logique du jeu)
│                 │
│  ┌───────────┐  │
│  │ Thread    │  │  (Move&Score)
│  │ Move&Score│  │
│  └─────┬─────┘  │
│        │        │
│  ┌─────▼─────┐  │
│  │ Thread    │  │  (Goal)
│  │ Goal      │  │
│  └─────┬─────┘  │
└────────┼────────┘
         │ Pipe anonyme
         │ (DisplayMessage)
         │ + Signaux (SIGUSR1, SIGUSR2)
         ▼
┌─────────────────┐
│ Processus display│  (Affichage)
└─────────────────┘
```

## Les 3 Processus Principaux

### 1. Processus `main` (main_process.c)

**Rôle** : Interface utilisateur pour la saisie des commandes

**Fonctionnement** :
- Lit les commandes au clavier (w, s, a, d, q)
- Convertit les caractères en `CommandType` (CMD_MOVE_UP, CMD_MOVE_DOWN, etc.)
- Envoie les commandes au processus `game2048` via un **pipe nommé**

**Communication** :
- **Pipe nommé** : `/tmp/2048_main_pipe`
- **Direction** : `main` → `game2048` (écriture uniquement)
- **Message** : `CommandMessage` contenant un `CommandType`

**Commandes** :
- `w` / `W` : Mouvement vers le haut
- `s` / `S` : Mouvement vers le bas
- `a` / `A` : Mouvement vers la gauche
- `d` / `D` : Mouvement vers la droite
- `q` / `Q` : Quitter le jeu

### 2. Processus `game2048` (game_2048.c)

**Rôle** : Cœur du système - Gère la logique du jeu avec une architecture multithread

**Fonctionnement** :
- Crée le processus `display` en tant qu'enfant (fork + execl)
- Crée deux threads de traitement :
  - **Thread Move&Score** : Traite les mouvements
  - **Thread Goal** : Vérifie les conditions de fin de partie
- Lit les commandes depuis le pipe nommé
- Coordonne la synchronisation entre threads

**Architecture Multithread** :

#### Thread Principal
- Lit les commandes depuis le pipe nommé
- Transmet les commandes au thread Move&Score via des variables partagées
- Gère l'arrêt propre du système

#### Thread Move&Score
- Attend une commande via la variable conditionnelle `move_cond`
- Convertit la commande en direction de mouvement
- Exécute le mouvement avec `game_move()`
- Ajoute une nouvelle tuile si un mouvement a été effectué
- Réveille le thread Goal après traitement

#### Thread Goal
- Attend qu'un mouvement soit effectué via `goal_cond`
- Vérifie si la partie est gagnée (`game_is_won()`)
- Vérifie si la partie est perdue (`game_is_lost()`)
- Envoie l'état du jeu au processus `display` via le pipe anonyme
- Envoie un signal au processus `display` pour le réveiller

**Synchronisation** :
- **Mutex** : `game_mutex` protège l'accès à `shared_game_state`
- **Variables conditionnelles** :
  - `move_cond` : Réveille le thread Move&Score lorsqu'une commande arrive
  - `goal_cond` : Réveille le thread Goal après un mouvement

**Communication** :
- **Pipe nommé** (lecture) : Reçoit les commandes de `main`
- **Pipe anonyme** (écriture) : Envoie l'état du jeu à `display`
- **Signaux** : Envoie `SIG_UPDATE_DISPLAY` et `SIG_GAME_OVER` à `display`

### 3. Processus `display` (display_process.c)

**Rôle** : Affichage visuel de la grille et de l'état du jeu

**Fonctionnement** :
- Lit l'état du jeu depuis `stdin` (redirigé vers le pipe anonyme)
- Attend les signaux pour se réveiller et afficher
- Affiche la grille formatée dans le terminal
- Affiche les messages de victoire/défaite

**Communication** :
- **Pipe anonyme** (lecture) : Reçoit les messages depuis `game2048`
- **Signaux** :
  - `SIG_UPDATE_DISPLAY` : Réveille le processus pour afficher une mise à jour
  - `SIG_GAME_OVER` : Indique la fin de partie
  - `SIGTERM` : Arrêt propre

## Communication Inter-Processus

### Pipe Nommé (FIFO)

**Chemin** : `/tmp/2048_main_pipe`

**Utilisation** :
- Créé par `main` ou `game2048` avec `mkfifo()`
- `main` l'ouvre en écriture (`O_WRONLY`)
- `game2048` l'ouvre en lecture (`O_RDONLY`)
- Communication unidirectionnelle : `main` → `game2048`

**Message** :
```c
typedef struct {
    CommandType command;  // CMD_MOVE_UP, CMD_MOVE_DOWN, etc.
} CommandMessage;
```

**Caractéristiques** :
- Bloquant : `read()` et `write()` attendent qu'un processus se connecte
- Persistant : Le fichier existe dans le système de fichiers
- Unidirectionnel : Un seul écrivain, un seul lecteur

### Pipe Anonyme

**Création** : `pipe(display_pipe)` dans `game2048`

**Utilisation** :
- Créé par `game2048` avant le `fork()`
- `game2048` garde l'extrémité d'écriture (`display_pipe[1]`)
- `display` reçoit l'extrémité de lecture via `dup2()` sur `stdin`
- Communication unidirectionnelle : `game2048` → `display`

**Message** :
```c
typedef struct {
    GameState state;     // État complet du jeu
    bool game_over;      // Indicateur de fin de partie
} DisplayMessage;
```

**Caractéristiques** :
- Non persistant : Existe uniquement en mémoire
- Unidirectionnel : Un seul écrivain, un seul lecteur
- Hérité par le processus enfant lors du `fork()`

## Synchronisation avec les Signaux

### SIG_UPDATE_DISPLAY (SIGUSR1)

**Émetteur** : `game2048` (thread Goal)

**Récepteur** : `display`

**Usage** : Réveille le processus `display` lorsqu'une mise à jour est disponible

**Fonctionnement** :
1. Le thread Goal écrit un `DisplayMessage` dans le pipe anonyme
2. Le thread Goal envoie `SIG_UPDATE_DISPLAY` à `display`
3. Le signal interrompt `read()` dans `display` (erreur `EINTR`)
4. `display` relance `read()` et récupère le message
5. `display` affiche la grille mise à jour

### SIG_GAME_OVER (SIGUSR2)

**Émetteur** : `game2048` (thread Goal)

**Récepteur** : `display`

**Usage** : Indique que la partie est terminée

**Fonctionnement** :
1. Le thread Goal détecte que la partie est terminée
2. Le thread Goal envoie `SIG_GAME_OVER` à `display`
3. Le handler `game_over_handler()` dans `display` met `running = false`
4. Le processus `display` se termine proprement

## Flux de Données Complet

### 1. Initialisation

```
game2048 démarre
    ↓
Initialise le générateur aléatoire
    ↓
Crée le pipe anonyme (pipe())
    ↓
Crée le processus display (fork() + execl())
    ↓
Initialise l'état du jeu (game_init())
    ↓
Envoie l'état initial à display
    ↓
Crée les threads Move&Score et Goal
    ↓
Crée le pipe nommé (mkfifo())
    ↓
Ouvre le pipe nommé en lecture
    ↓
Attend les commandes de main
```

### 2. Cycle de Jeu Normal

```
Utilisateur appuie sur 'w' dans main
    ↓
main convertit en CMD_MOVE_UP
    ↓
main écrit CommandMessage dans pipe nommé
    ↓
game2048 (thread principal) lit la commande
    ↓
game2048 transmet la commande au thread Move&Score
    ↓
Thread Move&Score :
    - Convertit CMD_MOVE_UP en MOVE_UP
    - Exécute game_move()
    - Ajoute une nouvelle tuile si mouvement effectué
    - Réveille le thread Goal
    ↓
Thread Goal :
    - Vérifie game_is_won() et game_is_lost()
    - Met à jour le statut si nécessaire
    - Écrit DisplayMessage dans pipe anonyme
    - Envoie SIG_UPDATE_DISPLAY à display
    ↓
display reçoit le signal et lit le message
    ↓
display affiche la grille mise à jour
```

### 3. Fin de Partie

```
Thread Goal détecte GAME_WON ou GAME_LOST
    ↓
Thread Goal met game_over = true
    ↓
Thread Goal écrit DisplayMessage avec game_over = true
    ↓
Thread Goal envoie SIG_GAME_OVER à display
    ↓
Thread Goal met game_running = false
    ↓
display affiche le message de fin
    ↓
display se termine (running = false)
    ↓
game2048 attend la fin des threads (pthread_join())
    ↓
game2048 attend la fin de display (waitpid())
    ↓
game2048 se termine
```

## Structures de Données Partagées

### GameState (partagé entre threads)

```c
typedef struct {
    int grid[4][4];        // Grille 4x4
    int score;             // Score actuel
    GameStatus status;     // PLAYING, WON, LOST
    bool has_moved;        // Indicateur de mouvement
} GameState;
```

**Protection** : Accès protégé par `game_mutex`

**Modifications** :
- Thread Move&Score : Modifie `grid`, `score`, `has_moved`
- Thread Goal : Lit `grid` et `status`, modifie `status`

## Gestion des Erreurs

### Interruptions par Signal (EINTR)

Les opérations bloquantes (`read()`, `write()`) peuvent être interrompues par des signaux. Le code gère cela en vérifiant `errno == EINTR` et en relançant l'opération.

### Fermeture des Pipes

Si un pipe est fermé :
- `read()` retourne 0 (EOF)
- `write()` peut échouer avec EPIPE
- Les processus détectent cela et se terminent proprement

### Arrêt Propre

Lors de l'arrêt (Ctrl+C, SIGTERM, ou commande 'q') :
1. `game_running` est mis à `false`
2. Les threads sont réveillés et se terminent
3. Les pipes sont fermés
4. Les processus enfants sont attendus avec `waitpid()`

## Points Clés de l'Architecture

1. **Séparation des responsabilités** :
   - `main` : Saisie utilisateur
   - `game2048` : Logique du jeu
   - `display` : Affichage

2. **Parallélisme** :
   - Threads pour traiter les mouvements et vérifier les conditions en parallèle
   - Processus séparés pour isoler les fonctionnalités

3. **Communication asynchrone** :
   - Pipes pour les données
   - Signaux pour la synchronisation

4. **Synchronisation thread-safe** :
   - Mutex pour protéger les données partagées
   - Variables conditionnelles pour coordonner les threads

5. **Robustesse** :
   - Gestion des interruptions par signal
   - Arrêt propre de tous les processus et threads
   - Vérification des erreurs à chaque étape

## Compilation et Exécution

### Compilation

```bash
make
```

Génère 3 exécutables dans `bin/` :
- `bin/main` : Processus de saisie
- `bin/game2048` : Processus principal
- `bin/display` : Processus d'affichage

### Exécution

**Option 1 : Manuel**
```bash
# Terminal 1
./bin/game2048

# Terminal 2
./bin/main
```

**Option 2 : Automatique**
```bash
make run-game
```

### Ordre de Lancement

1. **game2048** doit être lancé en premier (il crée le processus display)
2. **main** peut être lancé ensuite (il se connecte au pipe nommé)

## Conclusion

Cette architecture permet une séparation claire des responsabilités, une communication efficace entre processus, et une gestion robuste de la synchronisation. Le système est conçu pour être modulaire, maintenable et extensible.
