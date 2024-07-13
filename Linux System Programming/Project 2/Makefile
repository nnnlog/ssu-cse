DEBUG_OPTIONS = -g -DDEBUG=0
EXT_OPTIONS = -w
OPTIONS = $(DEBUG_OPTIONS) $(EXT_OPTIONS)


all: ssu_score
	@echo "Build complete"


build: all

ssu_score: main.c blank.o ssu_score.o linked_list.o
	@gcc $^ -o $@ $(OPTIONS)

blank.o: blank.c
	@gcc -c $^ -o $@ $(OPTIONS)

ssu_score.o: ssu_score.c
	@gcc -c $^ -o $@ $(OPTIONS)

linked_list.o: linked_list.c
	@gcc -c $^ -o $@ $(OPTIONS)


clean:
	rm -f ssu_backup test_* *.o
