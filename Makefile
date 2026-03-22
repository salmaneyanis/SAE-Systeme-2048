CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11 -pthread
INCLUDES = -Iinclude
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# Créer les dossiers s'ils n'existent pas
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

# Fichiers sources
GAME_LOGIC_SRC = $(SRCDIR)/game_logic.c
DISPLAY_GRID_SRC = $(SRCDIR)/display_grid.c
MAIN_PROCESS_SRC = $(SRCDIR)/main_process.c
GAME_2048_SRC = $(SRCDIR)/game_2048.c
DISPLAY_PROCESS_SRC = $(SRCDIR)/display_process.c

# Fichiers objets
GAME_LOGIC_OBJ = $(OBJDIR)/game_logic.o
DISPLAY_GRID_OBJ = $(OBJDIR)/display_grid.o
MAIN_PROCESS_OBJ = $(OBJDIR)/main_process.o
GAME_2048_OBJ = $(OBJDIR)/game_2048.o
DISPLAY_PROCESS_OBJ = $(OBJDIR)/display_process.o

# Bibliothèques communes
COMMON_OBJS = $(GAME_LOGIC_OBJ) $(DISPLAY_GRID_OBJ)

# Règle par défaut
all: $(BINDIR)/main $(BINDIR)/game2048 $(BINDIR)/display

# Compilation des objets
$(OBJDIR)/game_logic.o: $(GAME_LOGIC_SRC) include/game_logic.h include/game_state.h | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR)/display_grid.o: $(DISPLAY_GRID_SRC) include/display_grid.h include/game_state.h | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR)/main_process.o: $(MAIN_PROCESS_SRC) include/common.h include/ipc.h | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR)/game_2048.o: $(GAME_2048_SRC) include/common.h include/game_state.h include/game_logic.h include/ipc.h | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR)/display_process.o: $(DISPLAY_PROCESS_SRC) include/common.h include/game_state.h include/display_grid.h include/ipc.h | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Exécutables
$(BINDIR)/main: $(MAIN_PROCESS_OBJ) | $(BINDIR)
	$(CC) $(CFLAGS) $< -o $@

$(BINDIR)/game2048: $(GAME_2048_OBJ) $(GAME_LOGIC_OBJ) | $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@ -pthread

$(BINDIR)/display: $(DISPLAY_PROCESS_OBJ) $(DISPLAY_GRID_OBJ) | $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@

# Nettoyage
clean:
	rm -rf $(OBJDIR) $(BINDIR)
	rm -f /tmp/2048_main_pipe

# Nettoyage complet (y compris les pipes)
distclean: clean
	rm -f /tmp/2048_main_pipe

# Règle pour lancer le jeu (dans l'ordre : game2048 puis main)
run: all
	@echo "Lancement du jeu 2048..."
	@echo "Lancez d'abord: ./bin/game2048"
	@echo "Puis dans un autre terminal: ./bin/main"
	@echo "OU utilisez: make run-game"

run-game: all
	@echo "Lancement automatique avec pipe..."
	@mkfifo /tmp/2048_main_pipe || true
	@./bin/game2048 < /tmp/2048_main_pipe > /tmp/2048_main_pipe &
	@sleep 1
	@./bin/main < /tmp/2048_main_pipe > /tmp/2048_main_pipe
	@rm -f /tmp/2048_main_pipe


.PHONY: all clean distclean run run-game

