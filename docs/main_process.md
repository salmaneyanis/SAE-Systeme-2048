# Documentation : main_process.c

## Vue d'ensemble

Le fichier `main_process.c` implémente le **processus de saisie des commandes utilisateur**. C'est le premier point d'entrée du jeu où l'utilisateur interagit avec le système.

## Rôle principal

Ce processus est responsable de :
- **Lire les commandes** saisies par l'utilisateur au clavier (w, s, a, d pour les mouvements, q pour quitter)
- **Convertir** ces commandes en messages structurés
- **Envoyer** ces commandes au processus principal du jeu (`game2048`) via un **pipe nommé** (FIFO)

## Architecture de communication

Le processus utilise un **pipe nommé** (FIFO) pour communiquer avec le processus `game2048` :
- **Nom du pipe** : `/tmp/2048_main_pipe`
- **Direction** : `main_process` → `game2048` (écriture uniquement)
- **Type de message** : `CommandMessage` contenant un `CommandType`

## Fonctionnement

1. **Initialisation** : Crée le pipe nommé s'il n'existe pas déjà
2. **Ouverture** : Ouvre le pipe en mode écriture (bloquant jusqu'à ce qu'un lecteur se connecte)
3. **Boucle principale** :
   - Affiche un prompt (`> `)
   - Lit une commande au clavier
   - Convertit la commande (w/s/a/d) en `CommandType` (CMD_MOVE_UP/DOWN/LEFT/RIGHT)
   - Envoie le message via le pipe
   - Si la commande est 'q', envoie CMD_QUIT et termine

## Commandes supportées

- **w / W** : Mouvement vers le haut → `CMD_MOVE_UP`
- **s / S** : Mouvement vers le bas → `CMD_MOVE_DOWN`
- **a / A** : Mouvement vers la gauche → `CMD_MOVE_LEFT`
- **d / D** : Mouvement vers la droite → `CMD_MOVE_RIGHT`
- **q / Q** : Quitter le jeu → `CMD_QUIT`

## Points techniques

- Utilise `mkfifo()` pour créer le pipe nommé avec les permissions 0666
- Gère l'erreur `EEXIST` si le pipe existe déjà (normal au redémarrage)
- Utilise `write()` pour envoyer les messages de manière synchrone
- Le processus se termine proprement après l'envoi de CMD_QUIT
