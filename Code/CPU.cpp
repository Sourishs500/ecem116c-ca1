#include "CPU.h"


CPU::CPU(char* imem)
{
	PC = 0; //set PC to 0
	/*
	for (int i = 0; i < 4096; i++) //copy instrMEM
	{
		dmemory[i] = (0);
	}
      */
	this->instrMem = InstrMem(imem);
}


unsigned long CPU::readPC()
{
	return PC;
}
void CPU::incPC()
{
	printf("Incrementing PC from %lu to %lu\n\n", PC, nextPC);
	PC = nextPC;
}

string hex_char_to_bin(char c) {
	switch(toupper(c)) {
		case '0': return "0000";
		case '1': return "0001";
		case '2': return "0010";
		case '3': return "0011";
		case '4': return "0100";
		case '5': return "0101";
		case '6': return "0110";
		case '7': return "0111";
		case '8': return "1000";
		case '9': return "1001";
		case 'A': return "1010";
		case 'B': return "1011";
		case 'C': return "1100";
		case 'D': return "1101";
		case 'E': return "1110";
		case 'F': return "1111";
		default: return "";
	}
}


Instruction InstrMem::fetch(bitset<32> readAddr) {
	int32_t offset = ((int32_t) readAddr.to_ulong()) * 2;
	
	printf("Fetching instruction at address %d\n", offset);

	if (offset + 7 >= 4096) { // 4096 is IMEM size
		throw std::runtime_error("readAddr for imem is out of bounds");
	}
	
	string instr_str_hex = std::string() + imem[offset + 6] + imem[offset + 7] + 
		imem[offset + 4] + imem[offset + 5] +
		imem[offset + 2] + imem[offset + 3] +
		imem[offset] + imem[offset + 1];

	string instr_str = "";

	printf("Instruction in hex: %s\n", instr_str_hex.c_str());

	for (int i = 0; i < instr_str_hex.size(); i++) {
		instr_str += hex_char_to_bin(instr_str_hex[i]);
	}
	
	bitset<32> ret_instr(instr_str);
	return Instruction(ret_instr);
}



// Add other functions here ... 
