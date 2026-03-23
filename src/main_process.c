/*
 * main_process.c
 *
 * Usage : ./bin/main
 *
 * Ce processus :
 * 1. Cree un pipe de retour /tmp/2048_reg_<pid> pour recevoir son game_id
 * 2. Envoie CMD_NEW_GAME a game2048 via le pipe nomme unique
 * 3. Attend la reponse (son game_id)
 * 4. Lit les commandes clavier et les envoie avec son game_id
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "common.h"
#include "ipc.h"

int main(void)
{
    /* --- Ouverture du pipe nomme en ecriture --- */
    int pipe_fd = open(NAMED_PIPE_PATH, O_WRONLY);
    if (pipe_fd == -1) {
        perror("open named pipe");
        fprintf(stderr, "Lancez d'abord game2048.\n");
        exit(EXIT_FAILURE);
    }

    /*
     * --- Enregistrement ---
     * On cree un pipe de retour avec notre PID dans le nom.
     * game2048 lira ce nom depuis le message et nous repondra dedans.
     */
    pid_t my_pid = getpid();
    char reg_path[64];
    snprintf(reg_path, sizeof(reg_path), "%s%d", REG_PIPE_PREFIX, (int)my_pid);

    if (mkfifo(reg_path, 0600) == -1 && errno != EEXIST) {
        perror("mkfifo reg pipe");
        close(pipe_fd);
        exit(EXIT_FAILURE);
    }

    /* Envoyer la demande : game_id = notre PID comme cookie */
    CommandMessage req;
    req.game_id = (int)my_pid;
    req.command = CMD_NEW_GAME;
    if (write(pipe_fd, &req, sizeof(req)) == -1) {
        perror("write new_game");
        close(pipe_fd);
        unlink(reg_path);
        exit(EXIT_FAILURE);
    }

    /* Attendre la reponse de game2048 (bloquant) */
    int reg_fd = open(reg_path, O_RDONLY);
    if (reg_fd == -1) {
        perror("open reg pipe");
        close(pipe_fd);
        unlink(reg_path);
        exit(EXIT_FAILURE);
    }

    RegistrationReply reply;
    if (read(reg_fd, &reply, sizeof(reply)) != sizeof(reply)) {
        fprintf(stderr, "Reponse d'enregistrement invalide\n");
        close(reg_fd);
        close(pipe_fd);
        unlink(reg_path);
        exit(EXIT_FAILURE);
    }
    close(reg_fd);
    unlink(reg_path);

    int game_id = reply.game_id;
    printf("[main] Partie %d demarree !\n", game_id);
    printf("Commandes : w(haut) s(bas) a(gauche) d(droite) q(quitter)\n");

    /* --- Boucle de saisie --- */
    char input;
    CommandMessage msg;
    msg.game_id = game_id;

    while (1) {
        printf("[partie %d] > ", game_id);
        fflush(stdout);

        if (scanf(" %c", &input) != 1) {
            if (feof(stdin)) break;
            continue;
        }

        switch (input) {
            case 'w': case 'W': msg.command = CMD_MOVE_UP;    break;
            case 's': case 'S': msg.command = CMD_MOVE_DOWN;  break;
            case 'a': case 'A': msg.command = CMD_MOVE_LEFT;  break;
            case 'd': case 'D': msg.command = CMD_MOVE_RIGHT; break;
            case 'q': case 'Q':
                msg.command = CMD_QUIT;
                write(pipe_fd, &msg, sizeof(msg));
                close(pipe_fd);
                exit(EXIT_SUCCESS);
            default:
                printf("Commande invalide.\n");
                continue;
        }

        if (write(pipe_fd, &msg, sizeof(msg)) == -1) {
            perror("write command");
            break;
        }
    }

    close(pipe_fd);
    return 0;
}