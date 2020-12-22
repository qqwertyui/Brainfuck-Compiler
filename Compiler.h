/*

        Class which holds all instruction-related classes, enums
        and operations

*/
#ifndef COMPILER_H
#define COMPILER_H

#include <vector>

#include "Instruction.h"
#include "PE.h"

class Loop {
 public:
  uint begin;
  uint end;
  Loop(uint _begin);
};

class Compiler {
  bool good;
  uint size;
  uint code_va;
  uint chunks;

  void create(Instruction::Opcodes command, uint arg1 = 0, uint arg2 = 0,
              uint arg3 = Mode::REG, uint arg4 = 0);
  void insert_code(uint position, Instruction::Opcodes command, uint arg1 = 0,
                   uint arg2 = 0, uint arg3 = Mode::REG, uint arg4 = 0);

  // interace for building app
  void init_stack_frame(uint frame_size);
  void fini_stack_frame();
  void read();
  void write();
  void add(uint number);
  void sub(uint number);
  void add_ptr(uint number);
  void sub_ptr(uint number);
  void loop_begin();
  void loop_end();

  std::vector<Loop *> loops;
  uchar buffer[MAX_INSTRUCTION_SIZE];  // internal use only

  struct Imports {
    uint streams;
    uint memset;
    uint putc;
    uint getc;
  } imports;

 public:
  PE *pe;

  uchar *i386_code;
  uint i386_code_length;

  uchar *bf_code;
  uint bf_code_length;

  Compiler(const char *file_name, PE *pe);
  ~Compiler();

  void compile(uint memory_size);
  bool is_good() const;
  static bool is_brainfuck(unsigned char *data, size_t datasz);
};

#endif
