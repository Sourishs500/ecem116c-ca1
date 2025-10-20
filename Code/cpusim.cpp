#include "CPU.h"

#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
#include<fstream>
#include <sstream>
using namespace std;

/*
Add all the required standard and developed libraries here
*/

/*
Put/Define any helper function/definitions you need here
*/
int main(int argc, char* argv[])
{
	/* This is the front end of your project.
	You need to first read the instructions that are stored in a file and load them into an instruction memory.
	*/

	/* Each cell should store 1 byte. You can define the memory either dynamically, or define it as a fixed size with size 4KB (i.e., 4096 lines). Each instruction is 32 bits (i.e., 4 lines, saved in little-endian mode).
	Each line in the input file is stored as an hex and is 1 byte (each four lines are one instruction). You need to read the file line by line and store it into the memory. You may need a mechanism to convert these values to bits so that you can read opcodes, operands, etc.
	*/

	char instMem[IMEMORY_SIZE];

	// Initialize instruction memory to zero
	for (int i = 0; i < IMEMORY_SIZE; i++) {
		instMem[i] = 0;
	}


	if (argc < 2) {
		//cout << "No file name entered. Exiting...";
		return -1;
	}

	ifstream infile(argv[1]); //open the file
	if (!(infile.is_open() && infile.good())) {
		cout<<"error opening file\n";
		return 0; 
	}
	string line; 
	int i = 0;
	while (infile) {
			// Each single element of IMEM is one byte and stores one byte of the instruction. Four consecutive bytes form one instruction.
			infile>>line;
			stringstream line2(line);
			char x; 
			line2>>x;
			string instr_first_half = hex_char_to_bin(x);
			line2>>x;
			string instr_second_half = hex_char_to_bin(x);
			bitset<8> instr_bits = bitset<8>(instr_first_half + instr_second_half);
			char instr = (char) instr_bits.to_ulong();
			instMem[i] = instr; // be careful about hex
			i++;
		}
	int maxPC = i-5;
	// printf("maxPC = %d\n", maxPC);

	/* Instantiate your CPU object here.  CPU class is the main class in this project that defines different components of the processor.
	CPU class also has different functions for each stage (e.g., fetching an instruction, decoding, etc.).
	*/
	
	CPU myCPU = CPU(instMem);
	
	bool done = false;
	
	while (!done) // processor's main loop. Each iteration is equal to one clock cycle.  
	{
		myCPU.update(&done); // CPU goes through a single cycle and writes to done if opcode == 0
		myCPU.incPC(); // Set PC to nextPC
		if (myCPU.readPC() > maxPC) {
			// printf("PC %lu reached maxPC%d\n", myCPU.readPC(), maxPC);
			break;
		}
	}
	
	int a0 = myCPU.regFile.read(bitset<5>(10)); // x10
	int a1 = myCPU.regFile.read(bitset<5>(11)); // x11
	// print the results (you should replace a0 and a1 with your own variables that point to a0 and a1)
	cout << "(" << a0 << "," << a1 << ")" << endl;
	

	return 0;

}
