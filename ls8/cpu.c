#include "cpu.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
  RAM interface functions
*/

unsigned char cpu_ram_read(struct cpu *cpu, unsigned char address)
{
 return cpu->ram[address];
}

void cpu_ram_write(struct cpu *cpu, unsigned char value, unsigned char address)
{
  cpu->ram[address] = value;
}

/*
  Helper functions
*/
void cpu_PUSH(struct cpu *cpu, unsigned char value_to_add)
{
  cpu->SP--;
  cpu_ram_write(cpu, value_to_add, cpu->SP);
}
unsigned char cpu_POP(struct cpu *cpu)
{
  unsigned char return_value = cpu_ram_read(cpu, cpu->SP);
  cpu->SP++;
  return return_value;
}

void cpu_JMP(struct cpu *cpu, unsigned char new_address)
{
  // the PC has to be moved back by 2, since this is always called
  // where there is one operand (destination) plus this instruction
  cpu->PC = cpu->reg[new_address] - 2;
}

/**
 * Load the binary bytes from a .ls8 source file into a RAM array
 */
void cpu_load(struct cpu *cpu, char *filename)
{

  FILE *fp;

  char line[1024];
  if ((fp = fopen(filename, "r")) == NULL) {
    fprintf(stderr, "Cannot open file: %s\n", filename);
    // exit(1);
  }

  int address = 0;

  while (fgets(line, sizeof(line), fp) != NULL) {

    char *ptr;
    unsigned char ret = strtol(line, &ptr, 2);

    if (ptr == line) {
      continue;
    }

    cpu_ram_write(cpu, ret, address);
    address ++;

  }
}

/**
 * ALU
 */
void alu(struct cpu *cpu, enum alu_op op, unsigned char regA, unsigned char regB)
{
  switch (op) {
    case ALU_ADD:
      cpu->reg[regA] = cpu->reg[regA] + cpu->reg[regB];
      break;
    case ALU_MUL:
      cpu->reg[regA] = cpu->reg[regA] * cpu->reg[regB];
      break;
    case ALU_MOD:
      cpu->reg[regA] = cpu->reg[regA] % cpu->reg[regB];
      break;
    case ALU_CMP:
      if (cpu->reg[regA] == cpu->reg[regB]) {
        cpu->FL = 0b00000001;
      } else if (cpu->reg[regA] < cpu->reg[regB]) {
        cpu->FL = 0b00000100;
      } else {
        cpu->FL = 0b00000010;
      }
      break;
    case ALU_AND:
      cpu->reg[regA] = cpu->reg[regA] & cpu->reg[regB];
      break;
    case ALU_NOT:
      cpu->reg[regA] = ~(cpu->reg[regA]);
      break;
    case ALU_OR:
      cpu->reg[regA] = cpu->reg[regA] | cpu->reg[regB];
      break;
    case ALU_XOR:
      cpu->reg[regA] = cpu->reg[regA] ^ cpu->reg[regB];
      break;
    case ALU_SHL:
      cpu->reg[regA] = cpu->reg[regA] << cpu->reg[regB];
      break;
    case ALU_SHR:
      cpu->reg[regA] = cpu->reg[regA] >> cpu->reg[regB];
      break;
  }
}

/**
 * Run the CPU
 */
void cpu_run(struct cpu *cpu)
{
  int running = 1; // True until we get a HLT instruction

  unsigned char operand_a;
  unsigned char operand_b;
  cpu->SP = cpu->reg[7];

  while (running) {
    // 1. Get the value of the current instruction (in address PC).
    unsigned char IR = cpu_ram_read(cpu, cpu->PC);
    // 2. Figure out how many operands this next instruction requires
    unsigned int number_of_operands = (IR >> 6);
    // 3. Get the appropriate value(s) of the operands following this instruction
    if (number_of_operands == 2) {
      operand_a = cpu_ram_read(cpu, (cpu->PC+1) & 0xff);
      operand_b = cpu_ram_read(cpu, (cpu->PC+2) & 0xff);
    } else if (number_of_operands == 1) {
      operand_a = cpu_ram_read(cpu, (cpu->PC+1) & 0xff);
    } else {

    }
    // 4. switch() over it to decide on a course of action.
    // 5. Do whatever the instruction should do according to the spec.
    switch (IR) {
      case LDI:
        cpu->reg[operand_a] = operand_b;
        break;
      case PRN:
        printf("%d\n", cpu->reg[operand_a]);
        break;

      // stack management
      case PUSH:
        cpu_PUSH(cpu, cpu->reg[operand_a]);
        break;
      case POP:
        cpu->reg[operand_a] = cpu_POP(cpu);
        break;

      // ALU functions
      case MUL:
        alu(cpu, ALU_MUL, operand_a, operand_b);
        break;
      case ADD:
        alu(cpu, ALU_ADD, operand_a, operand_b);
        break;
      case MOD:
        if (operand_b == 0) {
          printf("Cannot divide by zero. Halting -->|\n");
          // HLT instruction; refactor?
          running = 0;
          break;
        }
        alu(cpu, ALU_MOD, operand_a, operand_b);
        break;
      case CMP:
        alu(cpu, ALU_CMP, operand_a, operand_b);
        break;
      case AND:
        alu(cpu, ALU_AND, operand_a, operand_b);
        break;
      case NOT:
        alu(cpu, ALU_NOT, operand_a, '0');
        break;
      case OR:
        alu(cpu, ALU_OR, operand_a, operand_b);
        break;
      case XOR:
        alu(cpu, ALU_XOR, operand_a, operand_b);
        break;
      case SHL:
        alu(cpu, ALU_SHL, operand_a, operand_b);
        break;
      case SHR:
        alu(cpu, ALU_SHR, operand_a, operand_b);
        break;

      // set PC counter
      case CALL:
        cpu_PUSH(cpu, cpu->PC+1);
        cpu_JMP(cpu, operand_a);
        break;
      case RET:
        cpu->PC = cpu_POP(cpu);
        break;
      case JMP:
        cpu_JMP(cpu, operand_a);
        break;
      case JEQ:
        if (cpu->FL == 1) {
          cpu_JMP(cpu, operand_a);
        }
        break;
      case JNE:
        if (cpu->FL % 2 == 0) {
          cpu_JMP(cpu, operand_a);
        }
        break;
      case JGE:
        if ((cpu->FL == 1) || (cpu->FL == 4)) {
          cpu_JMP(cpu, operand_a);
        }
        break;
      case JGT:
        if (cpu->FL == 4) {
          cpu_JMP(cpu, operand_a);
        }
        break;
      case JLE:
        if ((cpu->FL == 2) || (cpu->FL == 0)) {
          cpu_JMP(cpu, operand_a);
        }
        break;
      case JLT:
        if (cpu->FL == 2) {
          cpu_JMP(cpu, operand_a);
        }
        break;

      // halt
      case HLT:
        running = 0;
        break;
      default:
        break;
    }
    // 6. Move the PC to the next instruction.
    cpu->PC = cpu->PC + number_of_operands + 1;

  }
}

/**
 * Initialize a CPU struct
 */
void cpu_init(struct cpu *cpu)
{
  // TODO: Initialize the PC and other special registers
  memset(cpu->ram, 0, 256);

  cpu->PC = 0;
  cpu->FL = 0;

  memset(cpu->reg, 0, 8);
  cpu->reg[7] = 0xF4;
  // cpu->SP = reg[7];
}
