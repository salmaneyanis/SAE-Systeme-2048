/*
 * common.h
 * 
 * Ce fichier définit les constantes communes utilisées pour la communication
 * inter-processus dans le jeu 2048.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

/* Pipe nomme unique partage par TOUS les processus main */
#define NAMED_PIPE_PATH     "/tmp/2048_pipe"

/* Prefixe des pipes de retour pour l'enregistrement.
 * Chaque main cree /tmp/2048_reg_<son_pid> pour recevoir son game_id */
#define REG_PIPE_PREFIX     "/tmp/2048_reg_"

/* Signaux inter-threads (a l'interieur de game2048) */
#define SIG_MOVE  SIGUSR1   /* thread principal -> Move&Score */
#define SIG_GOAL  SIGUSR2   /* Move&Score -> Goal             */

/* Signaux inter-processus (game2048 -> display) */
#define SIG_UPDATE_DISPLAY  SIGUSR1
#define SIG_GAME_OVER       SIGUSR2

/* Nom du segment de memoire partagee (etapes 2 et 3) */
#define SHM_NAME            "/shm_2048"

#endif // COMMON_H

