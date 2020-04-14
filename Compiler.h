/*

	Class which holds all instruction-related classes, enums
	and operations

*/
#ifndef COMPILER_H
#define COMPILER_H


#include "Instruction.h"
#include "PE.h"
#include "Bf_status.h"
#include <vector>


class Loop
{
	public:
		uint begin;
		uint end;
	Loop(uint _begin);
};


class Compiler
{
	uint size;
	uint code_va;
	
	void call_wrap(uint addr);
	void create(Instruction::Opcodes command, uint arg1=0, uint arg2=0,
										uint arg3=Mode::REG, uint arg4=0);
	void insert_code(uint position, Instruction::Opcodes command, uint arg1=0,
							uint arg2=0, uint arg3=Mode::REG, uint arg4=0);
	std::vector <Loop*> loops;
	uchar buffer[MAX_INSTRUCTION_SIZE];
	
	uint streams_addr;
	uint memset_addr;
	uint putc_addr;
	uint getc_addr;
	
	public:
		PE *pe;
		Bf_status *status; // pointer because constructor has arguments
	
		uchar *code;
		uint code_length;
		
		uchar *bf_content;
		uint bf_content_length;
		
		Compiler(std::string filename, PE *pe);
		~Compiler();
		
		void compile(uint memory_size);
};


#endif