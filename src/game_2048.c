
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include "common.h"
#include "game_state.h"
#include "game_logic.h"
#include "ipc.h"

typedef struct {
    int           game_id;
    bool          active;
    GameState     state;
    MoveDirection pending_dir;
    int           display_pipe[2];
    pid_t         display_pid;
} GameEntry;

static volatile sig_atomic_t server_running = 1;
static GameEntry       *games      = NULL;
static int              nb_games   = 0;
static pthread_mutex_t  games_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t move_tid;
static pthread_t goal_tid;
static volatile int current_game_id = -1;
static SharedSlot *shm_slots   = NULL;
static int         shm_n_slots = 1;
static int         shm_fd      = -1;

static int shm_acquire_slot(void)
{
    for (int i = 0; i < shm_n_slots; i++) {
        if (sem_trywait(&shm_slots[i].sem_slot) == 0) {
            if (!shm_slots[i].in_use) {
                shm_slots[i].in_use = true;
                return i;
            }
            sem_post(&shm_slots[i].sem_slot);
        }
    }
    for (int i = 0; i < shm_n_slots; i++) {
        sem_wait(&shm_slots[i].sem_slot);
        if (!shm_slots[i].in_use) {
            shm_slots[i].in_use = true;
            return i;
        }
        sem_post(&shm_slots[i].sem_slot);
    }
    return -1;
}

static void shm_release_slot(int idx)
{
    shm_slots[idx].in_use  = false;
    shm_slots[idx].game_id = -1;
    sem_post(&shm_slots[idx].sem_slot);
}

void *move_score_thread(void *arg)
{
    (void)arg;
    sigset_t ws;
    sigemptyset(&ws);
    sigaddset(&ws, SIG_MOVE);

    while (server_running) {
        int sig;
        if (sigwait(&ws, &sig) != 0) { perror("sigwait move"); break; }
        if (!server_running) break;

        int gid = current_game_id;

        pthread_mutex_lock(&games_lock);
        GameEntry *entry = NULL;
        for (int i = 0; i < nb_games; i++) {
            if (games[i].active && games[i].game_id == gid) {
                entry = &games[i];
                break;
            }
        }
        if (!entry) {
            pthread_mutex_unlock(&games_lock);
            pthread_kill(goal_tid, SIG_GOAL);
            continue;
        }

        MoveDirection dir = entry->pending_dir;

        int slot = shm_acquire_slot();
        if (slot < 0) {
            fprintf(stderr, "[Move&Score] Aucun slot SHM\n");
            pthread_mutex_unlock(&games_lock);
            pthread_kill(goal_tid, SIG_GOAL);
            continue;
        }

        shm_slots[slot].game_id = gid;
        memcpy(&shm_slots[slot].state, &entry->state, sizeof(GameState));

        bool moved = game_move(&shm_slots[slot].state, dir);
        if (moved)
            game_add_random_tile(&shm_slots[slot].state);

        memcpy(&entry->state, &shm_slots[slot].state, sizeof(GameState));
        shm_release_slot(slot);

        pthread_mutex_unlock(&games_lock);
        pthread_kill(goal_tid, SIG_GOAL);
    }
    return NULL;
}

void *goal_thread(void *arg)
{
    (void)arg;
    sigset_t ws;
    sigemptyset(&ws);
    sigaddset(&ws, SIG_GOAL);

    while (server_running) {
        int sig;
        if (sigwait(&ws, &sig) != 0) { perror("sigwait goal"); break; }
        if (!server_running) break;

        int gid = current_game_id;

        pthread_mutex_lock(&games_lock);
        GameEntry *entry = NULL;
        for (int i = 0; i < nb_games; i++) {
            if (games[i].active && games[i].game_id == gid) {
                entry = &games[i];
                break;
            }
        }
        if (!entry) { pthread_mutex_unlock(&games_lock); continue; }

        if (entry->state.status == GAME_PLAYING) {
            if (game_is_won(&entry->state))
                entry->state.status = GAME_WON;
            else if (game_is_lost(&entry->state))
                entry->state.status = GAME_LOST;
        }

        DisplayMessage dmsg;
        dmsg.game_id   = gid;
        dmsg.game_over = (entry->state.status != GAME_PLAYING);
        memcpy(&dmsg.state, &entry->state, sizeof(GameState));

        int   dpipe_w = entry->display_pipe[1];
        pid_t dpid    = entry->display_pid;
        pthread_mutex_unlock(&games_lock);

        if (write(dpipe_w, &dmsg, sizeof(dmsg)) == -1)
            perror("goal write display pipe");
        if (dpid > 0)
            kill(dpid, SIG_UPDATE_DISPLAY);

        if (dmsg.game_over) {
            if (dpid > 0) kill(dpid, SIG_GAME_OVER);
            pthread_mutex_lock(&games_lock);
            for (int i = 0; i < nb_games; i++) {
                if (games[i].game_id == gid) {
                    close(games[i].display_pipe[1]);
                    waitpid(games[i].display_pid, NULL, 0);
                    games[i].active = false;
                    break;
                }
            }
            pthread_mutex_unlock(&games_lock);
        }
    }
    return NULL;
}

void cleanup_handler(int s)
{
    (void)s;
    server_running = 0;
    pthread_kill(move_tid, SIG_MOVE);
    pthread_kill(goal_tid, SIG_GOAL);
}

static int alloc_game_entry(int game_id)
{
    for (int i = 0; i < nb_games; i++) {
        if (!games[i].active) {
            games[i].active  = true;
            games[i].game_id = game_id;
            game_init(&games[i].state);
            return i;
        }
    }
    if (nb_games >= MAX_GAMES) return -1;
    GameEntry *tmp = realloc(games, (nb_games + 1) * sizeof(GameEntry));
    if (!tmp) return -1;
    games = tmp;
    int idx = nb_games++;
    memset(&games[idx], 0, sizeof(GameEntry));
    games[idx].active  = true;
    games[idx].game_id = game_id;
    game_init(&games[idx].state);
    return idx;
}

int main(int argc, char *argv[])
{
    srand((unsigned)time(NULL));

    if (argc >= 2) {
        shm_n_slots = atoi(argv[1]);
        if (shm_n_slots < 1) shm_n_slots = 1;
        if (shm_n_slots > MAX_GAMES) shm_n_slots = MAX_GAMES;
    }
    printf("[game2048] Demarrage avec %d slot(s) SHM.\n", shm_n_slots);

    sigset_t tmask;
    sigemptyset(&tmask);
    sigaddset(&tmask, SIG_MOVE);
    sigaddset(&tmask, SIG_GOAL);
    pthread_sigmask(SIG_BLOCK, &tmask, NULL);

    shm_unlink(SHM_NAME);
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
    if (shm_fd == -1) { perror("shm_open"); exit(EXIT_FAILURE); }

    size_t shm_size = (size_t)shm_n_slots * sizeof(SharedSlot);
    if (ftruncate(shm_fd, (off_t)shm_size) == -1) { perror("ftruncate"); exit(EXIT_FAILURE); }

    shm_slots = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_slots == MAP_FAILED) { perror("mmap"); exit(EXIT_FAILURE); }

    for (int i = 0; i < shm_n_slots; i++) {
        sem_init(&shm_slots[i].sem_slot, 1, 1);
        shm_slots[i].in_use  = false;
        shm_slots[i].game_id = -1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = cleanup_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    pthread_create(&move_tid, NULL, move_score_thread, NULL);
    pthread_create(&goal_tid, NULL, goal_thread,       NULL);

    unlink(NAMED_PIPE_PATH);
    if (mkfifo(NAMED_PIPE_PATH, 0666) == -1) { perror("mkfifo"); exit(EXIT_FAILURE); }

    int named_fd = open(NAMED_PIPE_PATH, O_RDWR);
    if (named_fd == -1) { perror("open named pipe"); exit(EXIT_FAILURE); }

    printf("[game2048] Pret. En attente de connexions...\n");

    int next_id = 0;
    CommandMessage msg;

    while (server_running) {
        ssize_t n = read(named_fd, &msg, sizeof(msg));
        if (n == 0) {
            struct timespec ts = {0, 1000000};
            nanosleep(&ts, NULL);
            continue;
        }
        if (n == -1) {
            if (errno == EINTR) continue;
            perror("read named pipe");
            break;
        }
        if (n != (ssize_t)sizeof(msg)) continue;

        if (msg.command == CMD_NEW_GAME) {
            int new_id = next_id++;
            pthread_mutex_lock(&games_lock);
            int idx = alloc_game_entry(new_id);
            pthread_mutex_unlock(&games_lock);

            if (idx < 0) { fprintf(stderr, "Trop de parties.\n"); continue; }

            if (pipe(games[idx].display_pipe) == -1) { perror("pipe display"); continue; }

            pid_t dpid = fork();
            if (dpid == 0) {
                close(games[idx].display_pipe[1]);
                dup2(games[idx].display_pipe[0], STDIN_FILENO);
                close(games[idx].display_pipe[0]);
                execl("./bin/display", "display", NULL);
                perror("execl display");
                exit(EXIT_FAILURE);
            }
            close(games[idx].display_pipe[0]);
            games[idx].display_pid = dpid;

            DisplayMessage init_msg;
            init_msg.game_id   = new_id;
            init_msg.game_over = false;
            memcpy(&init_msg.state, &games[idx].state, sizeof(GameState));
            write(games[idx].display_pipe[1], &init_msg, sizeof(init_msg));
            kill(dpid, SIG_UPDATE_DISPLAY);

            char reg_path[64];
            snprintf(reg_path, sizeof(reg_path), "%s%d", REG_PIPE_PREFIX, msg.game_id);
            int reg_fd = open(reg_path, O_WRONLY);
            if (reg_fd != -1) {
                RegistrationReply reply = { .game_id = new_id };
                write(reg_fd, &reply, sizeof(reply));
                close(reg_fd);
            }
            printf("[game2048] Partie %d creee (display PID=%d).\n", new_id, dpid);
            continue;
        }

        if (msg.command == CMD_QUIT) {
            pthread_mutex_lock(&games_lock);
            for (int i = 0; i < nb_games; i++) {
                if (games[i].active && games[i].game_id == msg.game_id) {
                    kill(games[i].display_pid, SIGTERM);
                    close(games[i].display_pipe[1]);
                    waitpid(games[i].display_pid, NULL, 0);
                    games[i].active = false;
                    printf("[game2048] Partie %d abandonnee.\n", msg.game_id);
                    break;
                }
            }
            pthread_mutex_unlock(&games_lock);
            continue;
        }

        pthread_mutex_lock(&games_lock);
        for (int i = 0; i < nb_games; i++) {
            if (games[i].active && games[i].game_id == msg.game_id) {
                games[i].pending_dir = (MoveDirection)msg.command;
                current_game_id      = msg.game_id;
                pthread_mutex_unlock(&games_lock);
                pthread_kill(move_tid, SIG_MOVE);
                goto next_msg;
            }
        }
        pthread_mutex_unlock(&games_lock);
next_msg:;
    }

    close(named_fd);
    unlink(NAMED_PIPE_PATH);
    pthread_kill(move_tid, SIG_MOVE);
    pthread_kill(goal_tid, SIG_GOAL);
    pthread_join(move_tid, NULL);
    pthread_join(goal_tid, NULL);

    pthread_mutex_lock(&games_lock);
    for (int i = 0; i < nb_games; i++) {
        if (games[i].active) {
            kill(games[i].display_pid, SIGTERM);
            close(games[i].display_pipe[1]);
            waitpid(games[i].display_pid, NULL, WNOHANG);
        }
    }
    pthread_mutex_unlock(&games_lock);
    free(games);

    for (int i = 0; i < shm_n_slots; i++)
        sem_destroy(&shm_slots[i].sem_slot);
    munmap(shm_slots, shm_size);
    close(shm_fd);
    shm_unlink(SHM_NAME);

    return 0;
}