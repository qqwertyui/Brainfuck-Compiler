CC = i686-w64-mingw32-c++
CFLAGS = -Wall -Wextra
FILES = main.cpp PE.cpp Instruction.cpp Compiler.cpp Parser.cpp
LIBS = -limagehlp

all:
	$(CC) $(FILES) $(LIBS) -o bfc.exe -static

clean:
	rm -f bfc.exe
