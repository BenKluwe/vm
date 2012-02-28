#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"

// Get rid of this to make it stfu
#define DEBUG

#define WHICH_FIRST  0b10000000
#define WHICH_SECOND 0b01000000

#define CMP_GREATER 0b00000001
#define CMP_LESS    0b00000010
#define CMP_EQUAL   0b00000100
#define STATUS_RUN  0b00001000

/*
 * C VM thing, duno was just a test, not very safe and probably quite inefficient
 * lol no planning whatsoever
 * gcc -o vm vm.c
 * ./vm program_name.bin
 */

int op_nop (int);			// NOP  instruction prototype
int op_jmp (int);			// JMP  instruction prototype
int op_cmp (int);			// CMP  instruction prototype
int op_jne (int);			// JNE  instruction prototype
int op_je  (int);			// JE   instruction prototype
int op_jg  (int);			// JG   instruction prototype
int op_jge (int);			// JGE  instruction prototype
int op_jl  (int);			// JL   instruction prototype
int op_jle (int);			// JLE  instruction prototype
int op_mov (int);			// MOV  instruction prototype
int op_inc (int);			// INC  instruction prototype
int op_dec (int);			// DEC  instruction prototype
int op_add (int);			// ADD  instruction prototype
int op_sub (int);			// SUB  instruction prototype
int op_mul (int);			// MUL  instruction prototype
int op_hlt (int);			// HLT  instruction prototype
int op_push(int);			// PUSH instruction prototype
int op_pop (int);			// POP  instruction prototype
int op_call(int);			// CALL instruction prototype
int op_ret (int);			// RET  instruction prototype

uint16_t vm_getraw(uint16_t);				// vm_setraw prototype
uint16_t vm_getnum(uint8_t, uint8_t, uint16_t);		// vm_getnum prototype
void     vm_setraw(uint16_t, uint16_t);			// vm_setraw prototype

int (*instruction[]) (int) =		// Instruction array (function pointers)
{
	op_nop,				// NOP  at opcode 0x00
	op_jmp,				// JMP  at opcode 0x01
	op_cmp,				// CMP  at opcode 0x02
	op_jne,				// JNE  at opcode 0x03
	op_je,				// JE   at opcode 0x04
	op_jg,				// JG   at opcode 0x05
	op_jge,				// JGE  at opcode 0x06
	op_jl,				// JL   at opcode 0x07
	op_jle,				// JLE  at opcode 0x08
	op_mov,				// MOV  at opcode 0x09
	op_inc,				// INC  at opcode 0x0A
	op_dec,				// DEC  at opcode 0x0B
	op_add,				// ADD  at opcode 0x0C
	op_sub,				// SUB  at opcode 0x0D
	op_mul,				// MUL  at opcode 0x0E
	op_hlt,				// HLT  at opcode 0x0F
	op_push,			// PUSH at opcode 0x10
	op_pop,				// POP  at opcode 0x11
	op_call,			// CALL at opcode 0x12
	op_ret				// RET  at opcode 0x13
};

char *progmem = NULL;			// Program memory array
int   proglen = 0;			// Program memory array length

uint16_t pcounter = 0;			// Program counter
uint8_t  status   = 8;			// Status register for cmp instructions and maybe other stuff
					// 0b00000000
					//       ||||greater than
					//       |||less than
					//       ||equal
					//       |is running

uint16_t gpregister[] =			// General purpose registers. 8 of these things.
{
	0x0000,				// GP 0
	0x0000,				// GP 1
	0x0000,				// GP 2
	0x0000,				// GP 3
	0x0000,				// GP 4
	0x0000,				// GP 5
	0x0000,				// GP 6
	0x0000				// GP 7
};

uint16_t stack[32];			// Stack for calls/push/pop
int      spointer = 31;			// Stack pointer


int main(int argc, const char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s <program_file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	FILE *progfile = fopen(argv[1], "rb");
	if (progfile == NULL)
	{
		printf("Unable to open program %s.\n", argv[1]);
		return EXIT_FAILURE;
	}

	fseek(progfile, 0L, SEEK_END);
	proglen = ftell(progfile);
	fseek(progfile, 0L, SEEK_SET);
	progmem = malloc(sizeof(char) * (2 ^ 16));
	fread(progmem, 1, 2 ^ 16, progfile);
	fclose(progfile);

	#ifdef DEBUG
	printf("Loaded program (%d bytes) into memory.\n", proglen);
	#endif

	while (pcounter < (2 ^ 16) && status & STATUS_RUN)
	{
		uint8_t instr  = progmem[pcounter];

		#ifdef DEBUG
		printf("PC: %d; Op: %X\n", pcounter, instr);
		#endif

		int offset = (*instruction[instr])(pcounter);
		pcounter += offset + 1;
	}

	free(progmem); // now with 25% more bidet
	return EXIT_SUCCESS;
}

uint16_t vm_getraw(uint16_t addr)
{
	uint8_t high = progmem[addr];
	uint8_t low  = progmem[addr + 1];
	
	return (high << 8) + low;
}

// ugly, could really do with some work too
uint16_t vm_getnum(uint8_t regselect, uint8_t which, uint16_t addr)
{
	if (which == WHICH_FIRST)
	{
		if (regselect & WHICH_FIRST)
		{
			// 16 bit number coming next
			pcounter += 2;
			
			return vm_getraw(addr);
		}
		else
		{
			// Register
			uint8_t reg = (regselect & 0b00111000) >> 3;
			
			return gpregister[reg];
		}
	}
	else
	{
		if (regselect & WHICH_SECOND)
		{
			// 16 bit number coming next
			// but what if the first number was also a number?
			uint8_t offs = 0;
			if (regselect & WHICH_FIRST)
			{
				offs = 2;
				pcounter += 2;
			}
			
			pcounter += 2;
			
			return vm_getraw(addr + offs);
		}
		else
		{
			// Register
			uint8_t reg = regselect & 0b00000111;
			
			return gpregister[reg];
		}
	}
}

void vm_setraw(uint16_t addr, uint16_t value)
{
	uint8_t high = (value & 0xFF00) >> 8;
	uint8_t low  = value & 0x00FF;
	progmem[addr] = high;
	progmem[addr + 1] = low;
}

int op_nop(int addr)
{
	// Do nothing

	#ifdef DEBUG
	printf("NOP\n");
	#endif

	return 0;
}

int op_jmp(int addr)
{
	pcounter = vm_getraw(addr + 1) - 1;

	#ifdef DEBUG
	printf("JMP: to %d\n", pcounter + 1);
	#endif

	return 0;
}

int op_jne(int addr)
{
	// jump if not equal
	if (!(status & CMP_EQUAL))
	 return op_jmp(addr);
	else
	 return 2;
}

int op_je(int addr)
{
	// jump if equal
	if (status & CMP_EQUAL)
	 return op_jmp(addr);
	else
	 return 2;
}

int op_jg(int addr)
{
	// jump if greater
	if (status & CMP_GREATER)
	 return op_jmp(addr);
	else
	 return 2;
}

int op_jge(int addr)
{
	// jump if greater or equal
	if (status & (CMP_GREATER | CMP_EQUAL))
	 return op_jmp(addr);
	else
	 return 2;
}

int op_jl(int addr)
{
	// jump if less than
	if (status & CMP_LESS)
	 return op_jmp(addr);
	else
	 return 2;
}

int op_jle(int addr)
{
	// jump if less or equal
	if (status & (CMP_LESS | CMP_EQUAL))
	 return op_jmp(addr);
	else
	 return 2;
}

// these notes are sort of irrelevant as vm_getnum is used and follows these rules instead
// Notes on CMP here
// "first" operand is 1 byte
// 0b00000000
//   ||||||||3 bit register selection for second operand
//   |||||||-
//   ||||||--
//   |||||3 bit register selection for first operand
//   ||||-
//   |||--
//   ||next operand (for comparison, after below if necessary) is 0 -> register, 1 -> 16 bit number coming next
//   |first operand (for comparison) is 0 -> register, 1 -> 16 bit number coming next

int op_cmp(int addr)
{
	uint8_t regselect = progmem[addr + 1];
	uint16_t cmp_a, cmp_b;

	cmp_a = vm_getnum(regselect, WHICH_FIRST, addr + 2);
	cmp_b = vm_getnum(regselect, WHICH_SECOND, addr + 2);

	// I bet this is really inefficient
	status = (status & ~CMP_EQUAL)   | (cmp_a == cmp_b) << 2;
	status = (status & ~CMP_LESS)    | (cmp_a < cmp_b)  << 1;
	status = (status & ~CMP_GREATER) | (cmp_a > cmp_b);

	#ifdef DEBUG
	printf("CMP: %d and %d\n", cmp_a, cmp_b);
	#endif

	return 1;
}

int op_mov(int addr)
{
	uint8_t regselect = progmem[addr + 1];

	uint16_t value = vm_getnum(regselect, WHICH_SECOND, addr + 2);

	if (regselect & WHICH_FIRST)
	{
		// Destination is a 16 bit address
		uint16_t dest = vm_getnum(regselect, WHICH_FIRST, addr + 2);
		vm_setraw(dest, value);

		#ifdef DEBUG
		printf("MOV: %d into %d\n", value, dest);
		#endif

		return 1;
	}
	else
	{
		// Register destination
		uint8_t reg = (regselect & 0b00111000) >> 3;
		gpregister[reg] = value;

		#ifdef DEBUG
		printf("MOV: %d into register %d\n", value, reg);
		#endif

		return 1;
	}
}

int op_inc(int addr)// vm_getnum(uint8_t regselect, uint8_t which, uint16_t addr)
{
	uint8_t regselect = progmem[addr + 1];
	if (regselect & WHICH_FIRST)
	{
		// 16 bit address
		uint16_t incaddr = vm_getnum(regselect, WHICH_FIRST, addr + 2);
		uint16_t incnum  = vm_getraw(incaddr);
		vm_setraw(incaddr, incnum + 1);

		#ifdef DEBUG
		printf("INC: %d\n", incaddr);
		#endif

		return 1;
	}
	else
	{
		// Register
		uint8_t reg = (regselect & 0b00111000) >> 3;
		gpregister[reg]++;

		#ifdef DEBUG
		printf("INC: register %d\n", reg);
		#endif

		return 1;
	}

	return 1;
}

int op_dec(int addr)
{
	uint8_t regselect = progmem[addr + 1];
	if (regselect & WHICH_FIRST)
	{
		// 16 bit address
		uint16_t decaddr = vm_getnum(regselect, WHICH_FIRST, addr + 2);
		uint16_t decnum  = vm_getraw(decaddr);
		vm_setraw(decaddr, decnum - 1);

		#ifdef DEBUG
		printf("DEC: %d\n", decaddr);
		#endif

		return 1;
	}
	else
	{
		// Register
		uint8_t reg = (regselect & 0b00111000) >> 3;
		gpregister[reg]--;

		#ifdef DEBUG
		printf("DEC: register %d\n", reg);
		#endif

		return 1;
	}

	return 1;
}

int op_add(int addr)
{
	return 0;
}

int op_sub(int addr)
{
	return 0;
}

int op_mul(int addr)
{
	return 0;
}

int op_hlt(int addr)
{
	status = status & ~STATUS_RUN;
	return 0;
}

int op_push(int addr)
{
	uint8_t regselect = progmem[addr + 1];
	uint16_t value;
	value = vm_getnum(regselect, WHICH_FIRST, addr + 2);

	stack[spointer] = value;
	spointer--;

	#ifdef DEBUG
	printf("PUSH: %d onto stack; SP: %d\n", value, spointer);
	#endif

	return 1;
}

int op_pop(int addr)
{
	uint8_t regselect = progmem[addr + 1];
	spointer++;
	uint16_t value = stack[spointer];

	if (regselect & WHICH_FIRST)
	{
		// 16 bit dest addr coming...
		uint16_t dest = vm_getnum(regselect, WHICH_FIRST, addr + 2);
		vm_setraw(dest, value);

		#ifdef DEBUG
		printf("POP: %d into %d; SP: %d\n", value, dest, spointer);
		#endif

		return 1;
	}
	else
	{
		// register dest
		uint8_t reg = (regselect & 0b00111000) >> 3;
		gpregister[reg] = value;

		#ifdef DEBUG
		printf("POP: %d into register %d; SP: %d\n", value, reg, spointer);
		#endif

		return 1;
	}
}

int op_call(int addr)
{
	stack[spointer] = addr + 3;
	spointer--;

	#ifdef DEBUG
	printf("CALL: pushed %d onto stack; SP: %d\n", addr + 3, spointer);
	#endif

	return op_jmp(addr);;
}

int op_ret(int addr)
{
	spointer++;
	uint16_t retaddr = stack[spointer];

	#ifdef DEBUG
	printf("RET: returning to %d; SP: %d\n", retaddr, spointer);
	#endif

	pcounter = retaddr - 1;

	return 0;
}

