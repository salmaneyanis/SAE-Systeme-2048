# Documentation : display_process.c

## Vue d'ensemble

Le fichier `display_process.c` implémente le **processus d'affichage** du jeu. Il est responsable de l'affichage visuel de la grille et de l'état du jeu dans le terminal.

## Rôle principal

Ce processus est responsable de :
- **Recevoir** l'état du jeu depuis le processus `game2048` via un **pipe anonyme**
- **Afficher** la grille de jeu formatée dans le terminal
- **Afficher** le score et les messages de fin de partie (victoire/défaite)
- **Réagir** aux signaux pour mettre à jour l'affichage

## Architecture de communication

Le processus utilise :
- **Pipe anonyme** : Reçoit les messages `DisplayMessage` depuis `game2048` via `stdin` (redirigé)
- **Signaux** : 
  - `SIG_UPDATE_DISPLAY` (SIGUSR1) : Signal pour réveiller le processus et afficher une mise à jour
  - `SIG_GAME_OVER` (SIGUSR2) : Signal indiquant que la partie est terminée
  - `SIGTERM` : Signal pour terminer proprement le processus

## Fonctionnement

1. **Initialisation** : Configure les handlers de signaux avec `sigaction()`
2. **Boucle principale** :
   - Lit un message `DisplayMessage` depuis `stdin` (pipe anonyme)
   - Gère les interruptions par signal (EINTR)
   - Efface l'écran avec des codes ANSI (`\033[2J\033[H`)
   - Affiche le titre et le score
   - Met à jour et affiche la grille formatée
   - Affiche les messages de victoire/défaite si nécessaire
   - Se termine si la partie est terminée

## Gestion des signaux

- **update_handler** : Réveille le processus lorsqu'un signal de mise à jour est reçu
- **game_over_handler** : Met fin à la boucle principale lorsque la partie est terminée

## Affichage

- Utilise des **codes ANSI** pour effacer l'écran et repositionner le curseur
- Affiche le titre "=== 2048 ===" et le score
- Utilise les fonctions de `display_grid.c` pour formater et afficher la grille
- Affiche des messages clairs pour la victoire (tuile 2048 atteinte) et la défaite (plus de mouvements possibles)

## Points techniques

- Utilise `read()` sur `STDIN_FILENO` pour lire depuis le pipe anonyme
- Gère correctement les interruptions par signal avec `EINTR`
- Utilise `fflush(stdout)` pour forcer l'affichage immédiat
- Le processus se termine proprement lorsque `running` devient `false`
