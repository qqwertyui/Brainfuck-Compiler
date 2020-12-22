#include "Compiler.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

const unsigned int CHUNK_SIZE = 0x2000;

Compiler::Compiler(const char *file_name, PE *pe) {
  this->good = true;
  this->pe = pe;
  this->chunks = 1;
  this->code_va = pe->ntHeader.OptionalHeader.ImageBase +
                  pe->ntHeader.OptionalHeader.BaseOfCode;
  memset(this->buffer, 0, MAX_INSTRUCTION_SIZE);

  this->imports.streams = this->pe->get_iat_rv("msvcrt.dll", "_iob");
  this->imports.getc = this->pe->get_iat_rv("msvcrt.dll", "fgetc");
  this->imports.putc = this->pe->get_iat_rv("msvcrt.dll", "fputc");
  this->imports.memset = this->pe->get_iat_rv("msvcrt.dll", "memset");

  Imports &i = this->imports;
  if (i.streams == 0 || i.getc == 0 || i.putc == 0 || i.memset == 0) {
    this->good = false;
    return;
  }
  std::fstream f(file_name, std::ios::in);
  if (f.good() == false) {
    this->good = false;
    return;
  }
  f.seekg(0, f.end);
  this->bf_code_length =
      1 + f.tellg();  // necessary when checking the code (j iterator is equals
                      // i+1, so av may be raised)
  f.seekg(0, f.beg);

  this->bf_code = new uchar[this->bf_code_length];
  f.read((char *)this->bf_code, this->bf_code_length);
  // add source code validator
  f.close();
}

void Compiler::compile(uint memory_size) {
  this->size = 0;  // internal compiler variable
#warning fix fixed code size
  this->i386_code = new uchar[this->chunks * CHUNK_SIZE];
  this->i386_code_length = 0;

  this->init_stack_frame(memory_size);

  uchar prevop = 0;
  int sameop = 1;  // changing int to uint breaks error checking

  for (uint i = 0; i < this->bf_code_length; i++) {
    if (this->i386_code_length > this->chunks * CHUNK_SIZE) {
      this->chunks++;
      unsigned char *i386_code_new = new uchar[this->chunks * CHUNK_SIZE];
      memcpy(i386_code_new, this->i386_code, (this->chunks - 1) * CHUNK_SIZE);
      delete[] this->i386_code;
      this->i386_code = i386_code_new;
    }
    uchar c = this->bf_code[i];
    prevop = this->bf_code[i + 1];

    if (prevop == c) {  // count '+', '-', '>' and '<' occurences
      sameop++;
    } else {
      if (c == '>')
        this->add_ptr(sameop);
      else if (c == '<')
        this->sub_ptr(sameop);
      else if (c == '+')
        this->add(sameop);
      else if (c == '-')
        this->sub(sameop);
      sameop = 1;
    }
    if (c == '.')
      this->write();
    else if (c == ',')
      this->read();
    else if (c == '[')
      this->loop_begin();
    else if (c == ']')
      this->loop_end();
  }

  this->fini_stack_frame();
}

void Compiler::create(Instruction::Opcodes command, uint arg1, uint arg2,
                      uint arg3, uint arg4) {
  this->size = Instruction::get(this->buffer, command, arg1, arg2, arg3, arg4);
  memcpy(this->i386_code + this->i386_code_length, this->buffer, this->size);
  this->i386_code_length += this->size;
}

void Compiler::insert_code(uint position, Instruction::Opcodes command,
                           uint arg1, uint arg2, uint arg3, uint arg4) {
  this->size = Instruction::get(this->buffer, command, arg1, arg2, arg3, arg4);
  memcpy(this->i386_code + position, this->buffer, this->size);
}

void Compiler::init_stack_frame(uint frame_size) {
  this->create(Instruction::PUSH_REG, Registers::EBP);
  this->create(Instruction::MOV_REG, Registers::EBP, Registers::ESP);
  this->create(Instruction::SUB_IMM, Registers::ESP, frame_size);
  this->create(Instruction::MOV_REG, Registers::EAX, Registers::ESP);
  this->create(Instruction::MOV_REG, Registers::EBX, Registers::EAX);
  this->create(Instruction::PUSH_IMM, frame_size);
  this->create(Instruction::PUSH_IMM, 0);
  this->create(Instruction::PUSH_REG, Registers::EAX);
  this->create(Instruction::MOV_IMM, Registers::EAX, this->imports.memset);
  this->create(Instruction::CALL, Registers::EAX, Mode::REG_DEREF);
  this->create(Instruction::ADD_IMM, Registers::ESP, 12);
  this->create(Instruction::MOV_IMM, Registers::ECX, 0);
  this->create(Instruction::MOV_IMM, Registers::EAX, 0);
}

void Compiler::fini_stack_frame() {
  this->create(Instruction::MOV_REG, Registers::ESP, Registers::EBP);
  this->create(Instruction::POP_REG, Registers::EBP);
  this->create(Instruction::RET);
}

void Compiler::add_ptr(uint number) {
  this->create(Instruction::ADD_IMM, Registers::ECX, number);
}

void Compiler::sub_ptr(uint number) {
  this->create(Instruction::SUB_IMM, Registers::ECX, number);
}

void Compiler::add(uint number) {
  this->create(Instruction::ADD_IMM, Registers::ESP, Registers::EBX,
               Registers::ECX, number);
}

void Compiler::sub(uint number) {
  this->create(Instruction::SUB_IMM, Registers::ESP, Registers::EBX,
               Registers::ECX, number);
}

void Compiler::write() {
  this->create(Instruction::PUSH_REG, Registers::ECX);
  this->create(Instruction::MOV_REG, Registers::EBP, Mode::REG_DEREF,
               Registers::EAX, this->imports.streams);
  this->create(Instruction::ADD_IMM, Registers::EAX, 0x20);  // offset to stdout
  this->create(Instruction::PUSH_REG, Registers::EAX);
  this->create(Instruction::MOV_IMM, Registers::EAX, 0);
  this->create(Instruction::MOV_REG, Registers::SIB, Registers::EBX,
               Registers::ECX, Registers::EAX);
  this->create(Instruction::PUSH_REG, Registers::EAX);
  this->create(Instruction::CALL, Registers::EBP, Mode::REG_DEREF,
               this->imports.putc);
  this->create(Instruction::ADD_IMM, Registers::ESP, 0x8);
  this->create(Instruction::POP_REG, Registers::ECX);
}

void Compiler::read() {
  this->create(Instruction::PUSH_REG, Registers::ECX);
  this->create(Instruction::MOV_REG, Registers::EBP, Mode::REG_DEREF,
               Registers::EAX, this->imports.streams);
  this->create(Instruction::PUSH_REG, Registers::EAX);
  this->create(Instruction::CALL, Registers::EBP, Mode::REG_DEREF,
               this->imports.getc);
  this->create(Instruction::ADD_IMM, Registers::ESP, 0x4);
  this->create(Instruction::POP_REG, Registers::ECX);
  this->create(Instruction::MOV_REG_2, Registers::ESP, Registers::EBX,
               Registers::ECX, Registers::EAX);
}

void Compiler::loop_begin() {
  this->create(Instruction::MOV_IMM, Registers::EAX, 0);
  this->create(Instruction::MOV_BYTE, Registers::ESP, Registers::EBX,
               Registers::ECX, Registers::EAX);
  this->create(Instruction::CMP, 0x0);
  loops.push_back(new Loop(this->i386_code_length));
  this->i386_code_length += 6;  // alloc space for 'je'
}

void Compiler::loop_end() {
  std::vector<Loop *> &l = this->loops;  // create reference for shorter names
  l[l.size() - 1]->end = this->i386_code_length;  // save file pointer
  this->create(Instruction::JMP, this->i386_code_length + this->code_va,
               l[l.size() - 1]->begin + this->code_va - 8);
  this->insert_code(l[l.size() - 1]->begin, Instruction::JE,
                    l[l.size() - 1]->begin + this->code_va,
                    this->code_va + l[l.size() - 1]->end + 5);
  delete l[l.size() - 1];  // Delete loop entry, because it gone
  l.pop_back();
}

bool Compiler::is_good() const { return this->good; }

Compiler::~Compiler() {
  delete[] this->bf_code;
  delete[] this->i386_code;
}

Loop::Loop(uint _begin) { this->begin = _begin; }
