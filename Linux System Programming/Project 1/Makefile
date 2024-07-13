DEBUG_OPTIONS = -g -DDEBUG=0
CRYPTO_OPTIONS = -lcrypto
OPTIONS = $(DEBUG_OPTIONS) $(CRYPTO_OPTIONS)


all: ssu_backup command
	@echo "Build complete"


ssu_backup: ssu_backup.c file.o usage.o linked_list.o file.o hash.o debug.o path.o
	@gcc $^ -o $@ $(OPTIONS)


command: command_add command_remove command_recover command_help

command_help: commands/command_help.c usage.o
	@gcc $^ -o $@ $(OPTIONS)

command_add: commands/command_add.c file.o usage.o linked_list.o file.o hash.o debug.o init.o path.o
	@gcc $^ -o $@ $(OPTIONS)

command_remove: commands/command_remove.c file.o usage.o linked_list.o file.o hash.o debug.o init.o path.o
	@gcc $^ -o $@ $(OPTIONS)

command_recover: commands/command_recover.c file.o usage.o linked_list.o file.o hash.o debug.o init.o path.o
	@gcc $^ -o $@ $(OPTIONS)


init.o: commands/init.c
	@gcc -c $^ -o $@ $(OPTIONS)

file.o: file/file.c
	@gcc -c $^ -o $@ $(OPTIONS)

path.o: file/path.c
	@gcc -c $^ -o $@ $(OPTIONS)

hash.o: file/hash.c
	@gcc -c $^ -o $@ $(OPTIONS)

linked_list.o: data/linked_list.c
	@gcc -c $^ -o $@ $(OPTIONS)

usage.o: usage/usage.c
	@gcc -c $^ -o $@ $(OPTIONS)

debug.o: debug/debug.c
	@gcc -c $^ -o $@ $(OPTIONS)


clean:
	rm -f ssu_backup command_add command_remove command_recover command_help *.o
