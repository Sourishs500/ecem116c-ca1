/*
* Design for a simple RISC-V ISA, 32-bit CPU with support for a handful of instructions
* Author: Sourish Saswade
*/

#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
using namespace std;

#define IMEMORY_SIZE 1000000 // 1 MB instruction memory
#define DMEMORY_SIZE 1000000 // 1 MB data memory

/* Note: All class functions are commented upon in CPU.cpp, along with their implementation*/

// For ALUOp
enum {
	ADD,
	SUB,
	SL, // shift-left
	SRA, // shift-right arithmetic
	AND,
	OR,
	LESS, // less than
	NOTEQUAL, // not equal to
	EQUAL, // equal to
	R, // R-type (func3/func7)
	I, // I-type (func3)
	L, // lw/lbu
	S // sw/sh
};

// For lbu, sh sizes
enum {
	WORD,
	HALF,
	BYTE
};

string hex_char_to_bin(char c);

// Instruction class to parse and hold fields from a 32-bit instruction
class Instruction {
public:
	// All fields extracted from instruction that we need
	bitset<32> instr;

	bitset<7> opcode;
	bitset<5> rs1;
	bitset<5> rs2;
	bitset<5> rd;

	bitset<1> bit30;
	bitset<3> bit14to12;

	Instruction(bitset<32> raw_instr = bitset<32>(0));
};

// Datapath: Each functional unit will expose a set of input(s), a set of output(s), and an update function to be called every cycle

// Instruction Memory to fetch instructions based on read address
class InstrMem {
private:
	char* imem; // Instruction memory array, passed in from cpusim.cpp main code
public:	
	InstrMem();

	InstrMem(char* imem);

	// Outputs
	Instruction instr; // Fetched instruction

	// Returns a 32-bit instruction based on a read address
	Instruction fetch(bitset<32> readAddr);
	
	// Inputs are update() arguments
	void update(bitset<32> readAddr);
};

// Register File with 32 registers, each 32 bits wide, and read/write capabilities
class RegFile {
private:
	// RISCV-32 ISA includes 32 registers x0-x31 (not including floating point), each 32 bits wide
	int32_t regFile[32];

	void write(bitset<5> registerNum, bitset<32> val);

public:
	// Outputs
	bitset<32> readData1; // rs1 data
	bitset<32> readData2; // rs2 data
	
	RegFile();

	// Must make this public so we can check a0 and a1
	int32_t read(bitset<5> registerNum);

	void regDump();

	void update_load(bitset<5> rs1, bitset<5> rs2);
	
	void update_store(bitset<5> rd, bitset<32> writeData, bitset<1> regWrite);
};

// Generate immediate based on instruction and opcode
class ImmGen {
public:
	// Outputs
	bitset<32> imm; // Generated immediate

	void update(bitset<32> instr, bitset<7> opcode);
};

// 2-input Mux
class Mux2 {
public:
	bitset<32> output; // Selected input

	void update(bitset<32> input0, bitset<32> input1, bitset<1> sel);
};

// Made for the writeback Mux, with input0 = alu result, input1 = readData, input2 = PC+4, sel0 = memToReg, sel1 = jalr
class Mux3 {
public:
	bitset<32> output; // Selected input

	void update(bitset<32> input0, bitset<32> input1, bitset<32> input2, bitset<1> sel0, bitset<1> sel1);
};

// ALU unit to perform arithmetic/logic operations
class ALU {
public:
	bitset<32> zero; // zero flag for branch instructions
	bitset<32> aluResult; // Result of ALU operation
	
	/*
	ADD,
	SUB,
	SL, // shift-left
	SRA, // shift-right arithmetic
	AND,
	OR,
	LESS, // less than
	NOTEQUAL, // not equal to
	EQUAL, // equal to
	*/

	void update(bitset<32> input0, bitset<32> input1, bitset<4> aluOp);
};

// Adder unit to add two 32-bit inputs
class Add {
public:
	bitset<32> sum;

	bitset<32> add(bitset<32> i1, bitset<32> i2);

	void update(bitset<32> i1, bitset<32> i2);
};

// Shift-left by 1
class ShiftLeft {
public:
	bitset<32> result;

	void update(bitset<32> input);
};

// Data Memory unit to read/write data from/to data memory
class DataMem {
private:
	char dmemory[DMEMORY_SIZE]; // Data memory array

	bitset<32> read(bitset<32> addr_bitset, bitset<2> rw_size); 
	void write(bitset<32> addr_bitset, bitset<32> writeData_bitset, bitset<2> rw_size);

public:
	DataMem();

	// Output is data read from memory, if memRead is set
	bitset<32> readData;
	
	void update(bitset<32> addr, bitset<32> writeData, bitset<1> memRead, bitset<1> memWrite, bitset<2> rw_size);
};


// Controller that takes in opcode and generates control signals
class Controller {
public:
	// Control signals
	bitset<1> branch;
	bitset<1> memRead;
	bitset<1> memToReg;
	bitset<4> aluOp;
	bitset<1> memWrite;
	bitset<1> aluSrc;
	bitset<1> regWrite;

	bitset<1> lui; // Control signal to be set if instruction is lui, used to select between rs1 and 0 in ALU input mux
	bitset<1> jalr; // Control signal to be set if instruction is jalr, used to select between PC+4 and rs1+imm in mux

	void update(bitset<7> opcode);

};

// ALU Controller that takes in funct3/funct7 bits and aluOp from Controller to generate final aluControlOp
class ALUController {
public:
	bitset<4> aluControlOp; // Control signal to be routed to ALU
	
	bitset<2> rw_size; // Control signal to be set for load/store size (word/half/byte)

	void update(bitset<1> bit30, bitset<3> bits14to12, bitset<4> aluOp);
};

// Top-level CPU class that contains all components of the datapath
class CPU {
private:
	unsigned long PC; // PC
	unsigned long nextPC; // next PC

public:
	unsigned long readPC(); // Returns current PC
	void incPC(); // Sets PC to next PC
	
	// CPU contains one class object to represent each part of the datapath
	Controller controller; 
	ALUController aluController;

	InstrMem instrMem;
	Add addPC4;

	RegFile regFile;

	ImmGen immGen;
	
	Mux2 luiMux; // Mux that selects between 0 (if lui control bit is set) and rs1 (if lui control bit is not set)
	Mux2 aluMux; // Mux that selects between readData2 and immGen
	
	ALU alu;

	ShiftLeft shiftLeftOne;
	Add addPCImm;
	Mux2 jalrPCMux; // Mux selects between PC+4 and rs1+imm
	Mux2 pcMux; // Mux that selects between PC+4 and PC+imm
	
	DataMem dataMem;
	Mux3 wbMux;
	
	CPU(char* imem);

	void update(bool* done);
};
