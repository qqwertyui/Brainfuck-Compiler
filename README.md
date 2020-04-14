# Brainfuck-Compiler
Simple brainfuck language to x86 PE file compiler.

Currently, it is very buggy and ugly-looking version which needs to be fixed in few places.

Basicly all what this program does is:
- creating PE file backbone (.text and .idata section, filling headers)
- importing fputc, fgetc, memset and _iob symbols from msvcrt.dll
- convering brainfuck language instructions into it's corresponding x86 assembly operation(s)
- creating stack frame and allocating 0x1000 (4096) bytes on stack

Allthough it's still very buggy version for example it will crash, when generates too large output asm code (buffer size is fixed). Also warning detection system warns too often when it's not necessary.
