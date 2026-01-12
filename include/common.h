#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

// Noms des pipes
#define NAMED_PIPE_MAIN_TO_2048 "/tmp/2048_main_pipe"
#define ANONYMOUS_PIPE_SIZE 4096

// Signaux utilisés pour la synchronisation
#define SIG_UPDATE_DISPLAY SIGUSR1
#define SIG_GAME_OVER SIGUSR2

#endif // COMMON_H

