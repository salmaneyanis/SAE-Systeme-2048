#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "common.h"
#include "ipc.h"

int main() {
    // Créer le pipe nommé (FIFO)
    if (mkfifo(NAMED_PIPE_MAIN_TO_2048, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }
    
    // Ouvrir le pipe en écriture
    int pipe_fd = open(NAMED_PIPE_MAIN_TO_2048, O_WRONLY);
    if (pipe_fd == -1) {
        perror("open pipe");
        exit(EXIT_FAILURE);
    }
    
    printf("=== Jeu 2048 ===\n");
    printf("Commandes: w (haut), s (bas), a (gauche), d (droite), q (quitter)\n");
    
    char command;
    CommandMessage msg;
    
    while (1) {
        printf("> ");
        fflush(stdout);
        
        // Lire une commande
        if (scanf(" %c", &command) != 1) {
            if (feof(stdin)) {
                break;
            }
            continue;
        }
        
        // Convertir la commande
        switch (command) {
            case 'w':
            case 'W':
                msg.command = CMD_MOVE_UP;
                break;
            case 's':
            case 'S':
                msg.command = CMD_MOVE_DOWN;
                break;
            case 'a':
            case 'A':
                msg.command = CMD_MOVE_LEFT;
                break;
            case 'd':
            case 'D':
                msg.command = CMD_MOVE_RIGHT;
                break;
            case 'q':
            case 'Q':
                msg.command = CMD_QUIT;
                if (write(pipe_fd, &msg, sizeof(CommandMessage)) == -1) {
                    perror("write quit");
                }
                close(pipe_fd);
                exit(EXIT_SUCCESS);
            default:
                printf("Commande invalide. Utilisez w/s/a/d pour jouer, q pour quitter.\n");
                continue;
        }
        
        // Envoyer la commande
        if (write(pipe_fd, &msg, sizeof(CommandMessage)) == -1) {
            perror("write command");
            break;
        }
    }
    
    close(pipe_fd);
    return 0;
}
