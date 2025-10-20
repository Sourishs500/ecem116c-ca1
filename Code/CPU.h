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

#define IMEMORY_SIZE 1000000
#define DMEMORY_SIZE 1000000

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

class Instruction {
public:
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

class InstrMem {
private:
	char* imem;
public:	
	InstrMem();

	InstrMem(char* imem);

	// Outputs
	Instruction instr;

	// Returns a 32-bit instruction based on a read address
	Instruction fetch(bitset<32> readAddr);
	
	// Inputs are update() arguments
	void update(bitset<32> readAddr);
};

class RegFile {
private:
	// RISCV-32 ISA includes 32 registers x0-x31 (not including floating point), each 32 bits wide
	int32_t regFile[32];

	void write(bitset<5> registerNum, bitset<32> val);

public:
	// Outputs
	bitset<32> readData1;
	bitset<32> readData2;
	
	RegFile();

	// Must make this public so we can check a0 and a1
	int32_t read(bitset<5> registerNum);

	void regDump();

	void update_load(bitset<5> rs1, bitset<5> rs2);
	
	void update_store(bitset<5> rd, bitset<32> writeData, bitset<1> regWrite);
};

class ImmGen {
public:
	// Outputs
	bitset<32> imm;

	void update(bitset<32> instr, bitset<7> opcode);
};

class Mux2 {
public:
	bitset<32> output;

	void update(bitset<32> input0, bitset<32> input1, bitset<1> sel);
};

// Made for the writeback Mux, with input0 = alu result, input1 = readData, input2 = PC+4, sel0 = memToReg, sel1 = jalr
class Mux3 {
public:
	bitset<32> output;

	void update(bitset<32> input0, bitset<32> input1, bitset<32> input2, bitset<1> sel0, bitset<1> sel1);
};

class ALU {
public:
	bitset<32> zero;
	bitset<32> aluResult;
	
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

class Add {
public:
	bitset<32> sum;

	bitset<32> add(bitset<32> i1, bitset<32> i2);

	void update(bitset<32> i1, bitset<32> i2);
};

class ShiftLeft {
public:
	bitset<32> result;

	void update(bitset<32> input);
};

class DataMem {
private:
	char dmemory[DMEMORY_SIZE];

	bitset<32> read(bitset<32> addr_bitset, bitset<2> rw_size);
	void write(bitset<32> addr_bitset, bitset<32> writeData_bitset, bitset<2> rw_size);

public:
	DataMem();

	// Output
	bitset<32> readData;
	
	void update(bitset<32> addr, bitset<32> writeData, bitset<1> memRead, bitset<1> memWrite, bitset<2> rw_size);
};


// Controller
class Controller {
public:
	// Outputs (initialized to 0)
	bitset<1> branch;
	bitset<1> memRead;
	bitset<1> memToReg;
	bitset<4> aluOp;
	bitset<1> memWrite;
	bitset<1> aluSrc;
	bitset<1> regWrite;

	bitset<1> lui; // For lui 
	bitset<1> jalr; // For jalr

	void update(bitset<7> opcode);

};

class ALUController {
public:
	
	// Output
	bitset<4> aluControlOp; // Control signal to be routed to ALU
	
	bitset<2> lui;
	bitset<2> rw_size;

	void update(bitset<1> bit30, bitset<3> bits14to12, bitset<4> aluOp);
};

class CPU {
private:
	// int dmemory[4096]; //data memory byte addressable in little endian fashion;
	unsigned long PC; //pc 
	unsigned long nextPC;

public:
	unsigned long readPC();
	void incPC();

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
	Mux2 jalrPCMux;
	Mux2 pcMux; // Mux that selects between PC+4 and PC+imm
	
	DataMem dataMem;
	Mux3 wbMux;
	
	CPU(char* imem);

	void update(bool* done);
};
