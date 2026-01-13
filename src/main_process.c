/*
 * main_process.c
 * 
 * Ce fichier implémente le processus de saisie des commandes utilisateur.
 * Il lit les commandes au clavier et les envoie au processus principal du jeu
 * via un pipe nommé (FIFO).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "common.h"
#include "ipc.h"

/**
 * Fonction principale du processus de saisie
 * 
 * Ce processus :
 * 1. Crée ou utilise un pipe nommé pour communiquer avec game2048
 * 2. Lit les commandes utilisateur au clavier
 * 3. Convertit les commandes en messages structurés
 * 4. Envoie les messages au processus game2048
 */
int main() {
    // ============================================
    // ÉTAPE 1 : Création du pipe nommé (FIFO)
    // ============================================
    // Le pipe nommé permet la communication entre processus indépendants.
    // mkfifo() crée un fichier spécial qui agit comme un tube.
    // Les permissions 0666 permettent la lecture et l'écriture pour tous.
    if (mkfifo(NAMED_PIPE_MAIN_TO_2048, 0666) == -1) {
        // Si le pipe existe déjà (EEXIST), c'est normal (redémarrage du jeu).
        // Sinon, c'est une vraie erreur qu'on doit gérer.
        if (errno != EEXIST) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }
    
    // ============================================
    // ÉTAPE 2 : Ouverture du pipe en écriture
    // ============================================
    // O_WRONLY : mode écriture uniquement.
    // Cette opération bloque jusqu'à ce qu'un processus ouvre le pipe en lecture.
    int pipe_fd = open(NAMED_PIPE_MAIN_TO_2048, O_WRONLY);
    if (pipe_fd == -1) {
        perror("open pipe");
        exit(EXIT_FAILURE);
    }
    
    // ============================================
    // ÉTAPE 3 : Affichage des instructions
    // ============================================
    printf("=== Jeu 2048 ===\n");
    printf("Commandes: w (haut), s (bas), a (gauche), d (droite), q (quitter)\n");
    
    // Variables pour la boucle principale
    char command;              // Commande lue au clavier
    CommandMessage msg;        // Message structuré à envoyer
    
    // ============================================
    // ÉTAPE 4 : Boucle principale de saisie
    // ============================================
    while (1) {
        // Afficher le prompt et forcer l'affichage immédiat
        printf("> ");
        fflush(stdout);
        
        // Lire une commande au clavier
        // Le format " %c" ignore les espaces et lit un caractère.
        if (scanf(" %c", &command) != 1) {
            // Si on atteint la fin du fichier (EOF), sortir de la boucle
            if (feof(stdin)) {
                break;
            }
            // Sinon, ignorer l'entrée invalide et continuer
            continue;
        }
        
        // ============================================
        // ÉTAPE 5 : Conversion de la commande
        // ============================================
        // Convertir le caractère saisi en CommandType pour le message.
        switch (command) {
            case 'w':
            case 'W':
                msg.command = CMD_MOVE_UP;      // Mouvement vers le haut
                break;
            case 's':
            case 'S':
                msg.command = CMD_MOVE_DOWN;    // Mouvement vers le bas
                break;
            case 'a':
            case 'A':
                msg.command = CMD_MOVE_LEFT;    // Mouvement vers la gauche
                break;
            case 'd':
            case 'D':
                msg.command = CMD_MOVE_RIGHT;   // Mouvement vers la droite
                break;
            case 'q':
            case 'Q':
                // Commande spéciale : quitter le jeu
                msg.command = CMD_QUIT;
                // Envoyer immédiatement la commande de sortie
                if (write(pipe_fd, &msg, sizeof(CommandMessage)) == -1) {
                    perror("write quit");
                }
                // Fermer le pipe et terminer proprement
                close(pipe_fd);
                exit(EXIT_SUCCESS);
            default:
                // Commande invalide : afficher un message d'aide
                printf("Commande invalide. Utilisez w/s/a/d pour jouer, q pour quitter.\n");
                continue;  // Ignorer cette commande et continuer la boucle
        }
        
        // ============================================
        // ÉTAPE 6 : Envoi de la commande
        // ============================================
        // Envoyer le message structuré au processus game2048 via le pipe.
        // write() est bloquant : il attend que le récepteur lise les données.
        if (write(pipe_fd, &msg, sizeof(CommandMessage)) == -1) {
            perror("write command");
            break;  // En cas d'erreur, sortir de la boucle
        }
    }
    
    // ============================================
    // NETTOYAGE : Fermeture du pipe
    // ============================================
    close(pipe_fd);
    return 0;
}
