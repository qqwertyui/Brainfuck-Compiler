#include "Instruction.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

Instruction::Instruction() {}

uint Instruction::get(uchar *buffer, Instruction::Opcodes operation,
                      uint argument1, uint argument2, uint argument3,
                      uint argument4) {
  uint tmp = 0;
  uint instruction_size = 0;
  memset(buffer, 0, MAX_INSTRUCTION_SIZE);
  switch (operation) {
    case Instruction::MOV_IMM: {
      instruction_size = sizeof(uchar) +  // opcode
                         sizeof(uint);    // immediate value

      tmp = Instruction::MOV_IMM + argument1;
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      memcpy(buffer + 1, (void *)&argument2, sizeof(uint));
      break;
    }
    case Instruction::MOV_REG: {
      instruction_size = 2 * sizeof(uchar);  // opcode

      tmp = Instruction::MOV_REG;
      uchar regs, sib;
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      if (argument1 == Registers::ESP && argument3 != Mode::REG) {
        // argument1 == SIB
        // argument2 == source1
        // argument3 == source2
        // argument4 == destination
        regs = Instruction::modrm(0x0, argument4, argument1);
        sib = Instruction::sib(0x0, argument2, argument3);
        memcpy(buffer + 1, (void *)&regs, sizeof(uchar));
        memcpy(buffer + 2, (void *)&sib, sizeof(uchar));

        instruction_size += 1;
      } else if (argument1 == Registers::EBP && argument4 != 0) {
        // argument1 == DISP
        // argument2 == mode
        // argument3 == destination
        // argument4 == disp_value
        regs = Instruction::modrm(argument2, argument3, argument1);
        memcpy(buffer + 1, (void *)&regs, sizeof(uchar));
        memcpy(buffer + 2, (void *)&argument4, sizeof(uint));
        instruction_size += sizeof(uint);
      } else {
        // argument1 == destination
        // argument2 == source
        // argument3 == mode
        // argument4 ==
        regs = Instruction::modrm(argument3, argument1, argument2);
        memcpy(buffer + 1, (void *)&regs, sizeof(uchar));
      }
      break;
    }
    case Instruction::MOV_REG_2: {
      instruction_size = 2 * sizeof(uchar);  // opcode

      tmp = Instruction::MOV_REG_2;
      uchar regs, sib;
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      if (argument1 == Registers::ESP && argument3 != Mode::REG) {
        // argument1 == SIB
        // argument2 == source1
        // argument3 == source2
        // argument4 == destination
        regs = Instruction::modrm(0x0, argument4, argument1);
        sib = Instruction::sib(0x0, argument2, argument3);
        memcpy(buffer + 1, (void *)&regs, sizeof(uchar));
        memcpy(buffer + 2, (void *)&sib, sizeof(uchar));

        instruction_size += 1;
      } else {
        // argument1 == destination
        // argument2 == source
        // argument3 ==
        // argument4 ==
        regs = Instruction::modrm(argument3, argument1, argument2);
        memcpy(buffer + 1, (void *)&regs, sizeof(uchar));
      }
      break;
    }
    case Instruction::INC: {
      instruction_size = sizeof(uchar);  // opcode

      tmp = Instruction::INC + argument1;
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      break;
    }
    case Instruction::DEC: {
      instruction_size = sizeof(uchar);  // opcode

      tmp = Instruction::DEC + argument1;
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      break;
    }
    case Instruction::CALL: {
      instruction_size = 2 * sizeof(uchar);  // opcode

      tmp = Instruction::CALL;
      uchar modrm = Instruction::modrm(argument2, 0x2, argument1);
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      memcpy(buffer + 1, (void *)&modrm, sizeof(uchar));
      if (argument1 == Registers::EBP && argument3 != Mode::REG) {
        instruction_size += sizeof(uint);
        memcpy(buffer + 2, (void *)&argument3, sizeof(uint));
      }
      break;
    }
    case Instruction::CMP: {
      instruction_size = sizeof(uchar) +  // opcode
                         sizeof(uint);    // immediate value

      tmp = Instruction::CMP;
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      memcpy(buffer + 1, (void *)&argument1, sizeof(uint));
      break;
    }
    case Instruction::JMP: {
      instruction_size = sizeof(uchar) +  // opcode
                         sizeof(uint);

      tmp = Instruction::JMP;
      uint target = argument2 - (argument1 + 5);
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      memcpy(buffer + 1, (void *)&target, sizeof(uint));
      break;
    }
    case Instruction::JE: {
      instruction_size = 2 * sizeof(uchar) +  // opcode
                         sizeof(uint);        // immediate value

      tmp = Instruction::JE;
      uint target = argument2 - (argument1 + 6);
      memcpy(buffer, (void *)&tmp, 2 * sizeof(uchar));
      memcpy(buffer + 2, (void *)&target, sizeof(uint));
      break;
    }
    case Instruction::PUSH_REG: {
      instruction_size = sizeof(uchar);  // opcode

      tmp = Instruction::PUSH_REG + argument1;
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      break;
    }
    case Instruction::POP_REG: {
      instruction_size = sizeof(uchar);  // opcode

      tmp = Instruction::POP_REG + argument1;
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      break;
    }
    case Instruction::RET: {
      instruction_size = sizeof(uchar);  // opcode

      tmp = Instruction::RET;
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      break;
    }
    case Instruction::SUB_IMM: {
      instruction_size = 2 * sizeof(uchar) +  // opcode and modrm
                         sizeof(uint);        // immediate value

      tmp = Instruction::SUB_IMM >> 4;
      uchar reg, sib;
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      if (argument1 == Registers::ESP && argument4 != 0) {
        // argument1 == SIB
        // argument2 == part1
        // argument3 == part2
        // argument4 == value
        reg = Instruction::modrm(Mode::REG_DEREF, 0x5, argument1);
        sib = Instruction::sib(0x0, argument2, argument3);
        memcpy(buffer + 1, (void *)&reg, sizeof(uchar));
        memcpy(buffer + 2, (void *)&sib, sizeof(uchar));
        memcpy(buffer + 3, (void *)&argument4, sizeof(uint));

        instruction_size += 1;
      } else {
        // argument1 == destination
        // argument2 == value
        // argument3 == mode
        reg = Instruction::modrm(argument3, 0x5, argument1);
        memcpy(buffer + 1, (void *)&reg, sizeof(uchar));
        memcpy(buffer + 2, (void *)&argument2, sizeof(uint));
      }
      break;
    }
    case Instruction::ADD_IMM: {
      instruction_size = 2 * sizeof(uchar) +  // opcode and modrm
                         sizeof(uint);        // immediate value

      tmp = Instruction::ADD_IMM >> 4;
      uchar reg, sib;
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      if (argument1 == Registers::ESP && argument4 != 0) {
        // argument1 == SIB
        // argument2 == part1
        // argument3 == part2
        // argument4 == value
        reg = Instruction::modrm(0x0, 0x0, argument1);
        sib = Instruction::sib(0x0, argument2, argument3);
        memcpy(buffer + 1, (void *)&reg, sizeof(uchar));
        memcpy(buffer + 2, (void *)&sib, sizeof(uchar));
        memcpy(buffer + 3, (void *)&argument4, sizeof(uint));

        instruction_size += 1;
      } else {
        // argument1 == destination
        // argument2 == value
        // argument3 == mode
        reg = Instruction::modrm(argument3, 0x0, argument1);
        memcpy(buffer + 1, (void *)&reg, sizeof(uchar));
        memcpy(buffer + 2, (void *)&argument2, sizeof(uint));
      }
      break;
    }
    case Instruction::PUSH_IMM: {
      instruction_size = sizeof(uchar) +  // opcode and modrm
                         sizeof(uint);    // immediate value

      tmp = Instruction::PUSH_IMM;
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      memcpy(buffer + 1, (void *)&argument1, sizeof(uint));
      break;
    }
    case Instruction::MOV_BYTE: {
      instruction_size = 2 * sizeof(uchar);  // opcode

      tmp = Instruction::MOV_BYTE;
      uchar regs, sib;
      memcpy(buffer, (void *)&tmp, sizeof(uchar));
      if (argument1 == Registers::ESP && argument3 != Mode::REG) {
        // argument1 == SIB
        // argument2 == source1
        // argument3 == source2
        // argument4 == destination
        regs = Instruction::modrm(0x0, argument4, argument1);
        sib = Instruction::sib(0x0, argument2, argument3);
        memcpy(buffer + 1, (void *)&regs, sizeof(uchar));
        memcpy(buffer + 2, (void *)&sib, sizeof(uchar));

        instruction_size += 1;
      } else {
        regs = Instruction::modrm(argument3, argument1, argument2);
        memcpy(buffer + 1, (void *)&regs, sizeof(uchar));
      }
      break;
    }
  }

  return instruction_size;
}

uchar Instruction::modrm(uchar mode, uchar regop, uchar rm) {
  return ((mode << 6) | (regop << 3) | (rm << 0));
}

uchar Instruction::sib(uchar scale, uchar index, uchar base) {
  return ((scale << 6) | (index << 3) | (base << 0));
}
