#include "Compiler.h"
#include <cstdlib>
#include <string>
#include <cstring>
#include <fstream>
#include <iostream>


Compiler::Compiler(std::string filename, PE *pe)
{
	this->pe = pe;
	this->code_va = pe->ntHeader.OptionalHeader.ImageBase + pe->ntHeader.OptionalHeader.BaseOfCode;
	memset(this->buffer, 0, MAX_INSTRUCTION_SIZE);
	
	this->streams_addr = this->pe->get_iat_rv("msvcrt.dll", "_iob");
	this->getc_addr = this->pe->get_iat_rv("msvcrt.dll", "fgetc");
	this->putc_addr = this->pe->get_iat_rv("msvcrt.dll", "fputc");
	this->memset_addr = this->pe->get_iat_rv("msvcrt.dll", "memset");
	
	std::fstream f(filename.c_str(), std::ios::in | std::ios::binary);
	f.seekg (0, f.end);
    this->bf_content_length = 1 + f.tellg(); // necessary when checking the code (j iterator is equals i+1, so av may be raised)
    f.seekg (0, f.beg);
	
	this->bf_content = new uchar [this->bf_content_length];
	f.read( (char*)bf_content, this->bf_content_length);
	
	f.close();
}

void 
Compiler::compile(uint memory_size)
{
	#warning fix fixed code size
	this->code = new uchar [0x4000];
	this->code_length = 0;
	this->size = 0;
	status = new Bf_status(memory_size);
	
	/*
		push ebp
		mov ebp, esp
		sub esp, memory_size
		mov eax, esp
		mov ebx, eax
		push memory_size
		push 0
		push eax
		call [iat_memset_addr]
		add esp, 12
		mov ecx, 0
		mov eax, 0
	*/
	this->create(Instruction::PUSH_REG, Registers::EBP_DISP);
	this->create(Instruction::MOV_REG, Registers::EBP_DISP, Registers::ESP_SIB);
	this->create(Instruction::SUB_IMM, Registers::ESP_SIB, memory_size);
	
	this->create(Instruction::MOV_REG, Registers::EAX, Registers::ESP_SIB);
	this->create(Instruction::MOV_REG, Registers::EBX, Registers::EAX);
	
	this->create(Instruction::PUSH_IMM, memory_size);
	this->create(Instruction::PUSH_IMM, 0);
	this->create(Instruction::PUSH_REG, Registers::EAX);
	this->create(Instruction::MOV_IMM, Registers::EAX, this->memset_addr);
	this->create(Instruction::CALL, Registers::EAX, Mode::REG_DEREF);
	this->create(Instruction::ADD_IMM, Registers::ESP_SIB, 12);
	
	this->create(Instruction::MOV_IMM, Registers::ECX, 0);
	this->create(Instruction::MOV_IMM, Registers::EAX, 0);
	

	uchar op_last = 0;
	int op_counter = 1; // changing int to uint breaks error checking
	
	for(uint i=0,j=1; i<this->bf_content_length; i++, j++)
	{
		uchar c = this->bf_content[i];
		op_last = this->bf_content[j];
		
		// count '+', '-', '>' and '<' occurences
		if(op_last == c)
			op_counter++;
		else
		{
				if(c == '>')
				{
					/*
						add ecx, op_counter
					*/
					this->create(Instruction::ADD_IMM, Registers::ECX, op_counter);
					if(status->memp == memory_size)
						status->warnings.push_back(new Warning(Bf_status::Flag::COUNTER_OVERFLOW, i+1) );
					status->memp += op_counter;
				}
				else if(c == '<')
				{
					/*
						sub ecx, op_counter
					*/
					this->create(Instruction::SUB_IMM, Registers::ECX, op_counter);
					if(status->memp == 0)
					{
						status->warnings.push_back(new Warning(Bf_status::Flag::COUNTER_UNDERFLOW, i+1) );
					}
					status->memp -= op_counter;
				}
				else if(c == '+')
				{
					/*
						add [ebx+ecx], op_counter
					*/
					this->create(Instruction::ADD_IMM, Registers::ESP_SIB, Registers::EBX, Registers::ECX, op_counter);
					if(status->memory[status->memp] == 0xff) // 255 = max uchar
						status->warnings.push_back(new Warning(Bf_status::Flag::MEMORY_POSITION_OVERRUN, i+1) );
					status->memory[status->memp] += op_counter;
				}
				else if(c == '-')
				{
					/*
						sub [ebx+ecx], op_counter
					*/
					this->create(Instruction::SUB_IMM, Registers::ESP_SIB, Registers::EBX, Registers::ECX, op_counter);
					
					if(status->memory[status->memp] == 0x0)
						status->warnings.push_back(new Warning(Bf_status::Flag::MEMORY_POSITION_UNDERRUN, i+1) );
					status->memory[status->memp] -= op_counter;
				}
				op_counter = 1;
		}
			if(c == '.')
			{
				/*
					push ecx
					push stdout_addr
					mov eax, 0
					mov eax, [ebx+ecx]
					push eax
					call [iat_putc_addr]
					add esp, 8
					pop ecx
				*/
				this->create(Instruction::PUSH_REG, Registers::ECX);
				this->create(Instruction::MOV_REG, Registers::EBP_DISP, Mode::REG_DEREF, Registers::EAX, this->streams_addr);
				this->create(Instruction::ADD_IMM, Registers::EAX, 0x20); // offset to stdout
				this->create(Instruction::PUSH_REG, Registers::EAX);
				
				this->create(Instruction::MOV_IMM, Registers::EAX, 0);
				this->create(Instruction::MOV_REG, Registers::ESP_SIB, Registers::EBX, Registers::ECX, Registers::EAX);
				this->create(Instruction::PUSH_REG, Registers::EAX);
				
				this->create(Instruction::CALL, Registers::EBP_DISP, Mode::REG_DEREF, this->putc_addr);
				
				this->create(Instruction::ADD_IMM, Registers::ESP_SIB, 0x8);
				this->create(Instruction::POP_REG, Registers::ECX);
			}
			else if(c == ',')
			{
				/*
					push ecx
					push stdin_addr
					call [iat_getc_addr]
					add esp, 4
					pop ecx
					mov eax, [ebx+ecx]
				*/
				this->create(Instruction::PUSH_REG, Registers::ECX);
				this->create(Instruction::MOV_REG, Registers::EBP_DISP, Mode::REG_DEREF, Registers::EAX, this->streams_addr);
				this->create(Instruction::PUSH_REG, Registers::EAX);
				
				this->create(Instruction::CALL, Registers::EBP_DISP, Mode::REG_DEREF, this->getc_addr);
				
				this->create(Instruction::ADD_IMM, Registers::ESP_SIB, 0x4);
				this->create(Instruction::POP_REG, Registers::ECX);
				this->create(Instruction::MOV_REG_2, Registers::ESP_SIB, Registers::EBX, Registers::ECX, Registers::EAX);
			}
			else if(c == '[')
			{
				/* 
					mov eax, 0
					mov al, [ecx+ebx]
					cmp eax, 0
					(preparing space for 'je', address will be computer later)
				*/
				this->create(Instruction::MOV_IMM, Registers::EAX, 0);
				this->create(Instruction::MOV_BYTE, Registers::ESP_SIB, Registers::EBX, Registers::ECX, Registers::EAX);
				
				this->create(Instruction::CMP, 0x0);
				loops.push_back(new Loop(this->code_length));
				
				this->code_length += 6; // alloc space for 'je'
			}
			else if(c == ']')
			{
				const uint jmp_size = 5;
				
				// save file pointer
				loops[loops.size()-1]->end = this->code_length;
				
				this->create(Instruction::JMP, this->code_length + this->code_va, loops[loops.size()-1]->begin + this->code_va - 8);
				
				/* 
					fill previous je with valid address
					Arguments are:
					insert_code(inserting_position, operation and its arguments);
				*/
				this->insert_code(loops[loops.size()-1]->begin, Instruction::JE, loops[loops.size()-1]->begin + this->code_va, this->code_va + loops[loops.size()-1]->end + jmp_size);
				
				// Delete loop entry, because it gone
				delete loops[loops.size()-1];
				loops.pop_back();
			}
	}
	
	/*
		mov esp, ebp
		pop ebp
		ret
	*/
	this->create(Instruction::MOV_REG, Registers::ESP_SIB, Registers::EBP_DISP);
	this->create(Instruction::POP_REG, Registers::EBP_DISP);
	this->create(Instruction::RET);
}


void 
Compiler::create(Instruction::Opcodes command, uint arg1, 
										uint arg2, uint arg3, uint arg4)
{
	this->size = Instruction::get(this->buffer, command, arg1, arg2, arg3, arg4);
	memcpy(this->code + this->code_length, this->buffer, this->size);
	this->code_length += this->size;
}


void 
Compiler::insert_code(uint position, Instruction::Opcodes command, 
									uint arg1,uint arg2, uint arg3, uint arg4)
{
	this->size = Instruction::get(this->buffer, command, arg1, arg2, arg3, arg4);
	memcpy(this->code + position, this->buffer, this->size);
}


Compiler::~Compiler()
{
	delete this->status;
	delete [] this->bf_content;
	delete [] this->code;
}

Loop::Loop(uint _begin)
{
	this->begin = _begin;
}
