/*

	Classes and enums which represent assembly (x86) instruction.

*/
#ifndef INSTRUCTION_H
#define INSTRUCTION_H


#include "Defs.h"
#include <string>


enum Registers
{
	EAX, ECX, EDX, EBX, ESP_SIB, EBP_DISP, ESI, EDI
};


enum Mode
{
	REG_DEREF, REG8_DEREF, REG32_DEREF, REG
};


class Instruction
{
	static uchar modrm(uchar mode, uchar regop, uchar rm);
	static uchar sib(uchar scale, uchar index, uchar base);
	
	public:
		enum Opcodes
		{
			MOV_IMM  = 	0xB8, 	// MOV r32,imm32
			MOV_REG = 	0x8B,	// MOV r32,r/m32
			MOV_REG_2 = 0x89,	// MOV r/m32,r32
			MOV_BYTE = 	0x8A,	// MOV r8,r/m8
			INC =    	0x40, 	// INC r32
			DEC = 	 	0x48,	// DEC r32
			CALL =	 	0xFF,	// CALL r/m32  /2
			CMP =	 	0x3D,	// CMP EAX,imm32
			JMP =	 	0xE9,	// JMP rel32
			JE =	 	0x840F, // JE rel32
			PUSH_REG =	0x50,	// PUSH r32
			POP_REG =  	0x58,	// POP r32
			RET = 	 	0xC3,	// RET
			SUB_IMM = 	0x815,	// SUB r/m32,imm32 /5
			ADD_IMM = 	0x810,	// ADD r/m32,imm32 /0
			PUSH_IMM = 	0x68	// PUSH imm32
		};

		Instruction();
		static uint get(uchar *buffer, Instruction::Opcodes operation, 
							uint argument1=0, uint argument2=0, 
							uint argument3=Mode::REG, uint argument4=0);
};

#endif