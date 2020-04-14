CC = g++
CFLAGS = -Wall -Wextra
FILES = main.cpp PE.cpp Instruction.cpp Compiler.cpp Parser.cpp Bf_status.cpp
LIBS = -limagehlp

all:
	$(CC) $(FILES) $(LIBS) -o bfc.exe
	
clean:
	rm -f bfc.exe