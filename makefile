
#OPTION FOR EXECUTABLE FILES
FLAGS =  -std=c89 -Wpedantic
#OPTIONS FOR .o FILE
OFLAGS = -c -std=c89 -Wpedantic

#OBJ = src/master.c src/player.c src/pawn.c

#DEPENDENCIES
DEPS = lib/sharedmem.c lib/sharedmem.h

#EXECUTABLE NAME
EXECUTABLE = main
#EXE DIRECTORY
EXDIR = exe
#SOURCE DIRECTORY
SDIR = src

#LIBRARY DIRECTORY
LIBDIR = lib
#LIBRARY NAME AFTER GENERATING .o
LIBNAME = sharedmem.o


#BASIC MAKE
all : createdir $(LIBDIR)/$(LIBNAME) $(EXDIR)/$(EXECUTABLE) $(EXDIR)/player $(EXDIR)/pawn copysettings

#MAKE RUN WITHOUT DELETING AT THE END
run : createdir $(LIBDIR)/$(LIBNAME) $(EXDIR)/$(EXECUTABLE) $(EXDIR)/player $(EXDIR)/pawn copysettings execute

#MAKE RUN WITH DELETES AT THE END
run_clean : run clean

#MAKE EXECUTE CAN EXECUTE THE PROGRAM
execute :
	cd $(EXDIR);./$(EXECUTABLE)

#CREATE EXE DIRECTORY IF IT DOESN'T EXISTS YET
createdir :
	mkdir -p $(EXDIR)

#COPY SETTINGS FILE FROM LIB TO EXE
copysettings :
	cp $(SDIR)/settings.txt $(EXDIR)

#CHECK FOR LIBRARIES TO RECOMPILE AND MOVE
$(LIBDIR)/$(LIBNAME): $(DEPS)
		$(CC)   $(OFLAGS) $(LIBDIR)/sharedmem.c -o $(LIBNAME)
		mv $(LIBNAME) $(LIBDIR)	

#CHECK FOR MAIN TO RECOMPILE (MASTER) AND MOVE
$(EXDIR)/$(EXECUTABLE) : src/master.c $(DEPS)
	$(CC)   $(FLAGS) $(SDIR)/master.c $(LIBDIR)/$(LIBNAME) -o $(EXECUTABLE)
	mv $(EXECUTABLE) $(EXDIR)

#CHECK FOR PLAYER TO RECOMPILE AND MOVE
$(EXDIR)/player : src/player.c $(DEPS)
	$(CC)   $(FLAGS) $(SDIR)/player.c $(LIBDIR)/$(LIBNAME) -o player
	mv player $(EXDIR)

#CHECK FOR PAWN TO RECOMPILE AND MOVE
$(EXDIR)/pawn : src/pawn.c $(DEPS)
	$(CC)   $(FLAGS) $(SDIR)/pawn.c $(LIBDIR)/$(LIBNAME) -o pawn
	mv pawn $(EXDIR)

#CLASSIC CLEAN
clean : 
	rm -f lib/*.o
	rm $(EXDIR)/*



