

FLAGS =  -std=c89 -Wpedantic -D_POSIX_C_SOURCE=199309L 


all : master.c player.c pawn.c lib/sharedmem.c lib/sharedmem.h
	$(CC)   $(FLAGS) master.c lib/sharedmem.c -o main
	$(CC)   $(FLAGS) player.c lib/sharedmem.c -o player
	$(CC)   $(FLAGS) pawn.c lib/sharedmem.c -o pawn

clean : 
	rm -f *.o
	rm main
	rm player
	rm pawn