DEBUG_OPTIONS = -g -DDEBUG=0
CRYPTO_OPTIONS =
EXT_OPTIONS = -w
OPTIONS = $(DEBUG_OPTIONS) $(CRYPTO_OPTIONS) $(EXT_OPTIONS)


all: ssu_monitor daemon_process command
	@echo "Build complete"


# --- main program ---
ssu_monitor: ssu_monitor.c path.o
	@gcc $^ -o $@ $(OPTIONS)


# --- daemon program ---
daemon_process: daemon/daemon_process.c path.o daemon_manager.o daemon_tree.o
	@gcc $^ -o $@ $(OPTIONS)


# --- commands program ---
command: command_add command_delete command_tree command_help

command_add: commands/command_add.c daemon_manager.o path.o usage.o
	@gcc $^ -o $@ $(OPTIONS)

command_delete: commands/command_delete.c daemon_manager.o path.o usage.o
	@gcc $^ -o $@ $(OPTIONS)

command_tree: commands/command_tree.c daemon_manager.o path.o usage.o tree.o
	@gcc $^ -o $@ $(OPTIONS)

command_help: commands/command_help.c usage.o
	@gcc $^ -o $@ $(OPTIONS)


# --- common components ---
path.o: commons/path.c
	@gcc -c $^ -o $@ $(OPTIONS)

usage.o: commons/usage.c
	@gcc -c $^ -o $@ $(OPTIONS)

daemon_manager.o: commons/daemon_manager.c
	@gcc -c $^ -o $@ $(OPTIONS)


# --- tree command component ---
tree.o: tree/tree.c
	@gcc -c $^ -o $@ $(OPTIONS)


# --- daemon component ---
daemon_tree.o: daemon/daemon_tree.c
	@gcc -c $^ -o $@ $(OPTIONS)


clean:
	rm -f ssu_monitor daemon_process command_* *.o
