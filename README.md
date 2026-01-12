# SAE-Systeme-2048

Implémentation du jeu 2048 en C avec architecture multiprocessus et multithreads.

## Architecture

Le projet est composé de 3 processus principaux :

1. **Processus `main`** : Lit les commandes utilisateur (w/s/a/d pour les mouvements, q pour quitter) et les envoie via un pipe nommé au processus 2048.

2. **Processus `2048`** : Contient 3 threads :
   - **Thread principal** : Gère la création des threads, lit les commandes depuis le pipe nommé, et coordonne l'arrêt du système
   - **Thread Move&Score** : Effectue les déplacements des tuiles selon les règles du jeu et calcule le score
   - **Thread Goal** : Vérifie si la partie est terminée (victoire ou défaite) et envoie l'état du jeu au processus affichage via un pipe anonyme

3. **Processus `affichage`** : Lit l'état du jeu depuis le pipe anonyme et l'affiche dans la console.

## Compilation

```bash
make
```

Cela compile les 3 exécutables dans le dossier `bin/` :
- `bin/main` : Processus de saisie des commandes
- `bin/game2048` : Processus de logique du jeu
- `bin/display` : Processus d'affichage

## Lancement

Le processus `game2048` doit être lancé en premier (il crée le processus `affichage` en tant qu'enfant), puis le processus `main` peut être lancé dans un autre terminal :

```bash
# Terminal 1
./bin/game2048

# Terminal 2
./bin/main
```

Ou utilisez la commande make pour un lancement automatique :

```bash
make run-game
```

## Commandes de jeu

- **w** : Déplacer vers le haut (↑)
- **s** : Déplacer vers le bas (↓)
- **a** : Déplacer vers la gauche (←)
- **d** : Déplacer vers la droite (→)
- **q** : Quitter le jeu

## Nettoyage

```bash
make clean      # Supprime les fichiers compilés
make distclean  # Supprime aussi les pipes nommés
```

## Structure du projet

```
.
├── include/           # Fichiers d'en-tête
│   ├── common.h       # Définitions communes (pipes, signaux)
│   ├── game_state.h   # Structures de données du jeu
│   ├── game_logic.h   # Interface de la logique du jeu
│   ├── display_grid.h # Interface d'affichage
│   └── ipc.h          # Structures de communication inter-processus
├── src/               # Fichiers sources
│   ├── game_logic.c   # Implémentation de la logique 2048
│   ├── display_grid.c # Implémentation de l'affichage
│   ├── main_process.c # Processus de saisie
│   ├── game_2048.c    # Processus principal avec threads
│   └── display_process.c # Processus d'affichage
├── Makefile           # Fichier de compilation
└── README.md          # Ce fichier
```
