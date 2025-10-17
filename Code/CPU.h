/*
* Design for a simple RISC-V ISA, 32-bit CPU with support for a handful of instructions
*/

#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
using namespace std;

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
};


class Instruction {
public:
	Instruction(bitset<32> raw_instr = bitset<32>(0)) {
		instr = raw_instr;
		opcode = bitset<7>(instr.to_ulong() & 0x7F);
		rs1 = bitset<5>((instr.to_ulong() >> 15) & 0x1F);
		rs2 = bitset<5>((instr.to_ulong() >> 20) & 0x1F);
		rd = bitset<5>((instr.to_ulong() >> 7) & 0x1F);
		bit30 = bitset<1>((instr.to_ulong() >> 30) & 1);
		bit14to12 = bitset<3>((instr.to_ulong() >> 12) & 7);

		// DEBUG
		if (raw_instr.to_ulong() == 0) return;

		cout << "Instruction: " << instr << endl;
		cout << "Opcode: " << opcode << endl;
		cout << "rs1: " << rs1 << endl;
		cout << "rs2: " << rs2 << endl;
		cout << "rd: " << rd << endl;
		cout << "bit30: " << bit30 << endl;
		cout << "bit14to12: " << bit14to12 << "\n\n";
	}

	bitset<32> instr;

	bitset<7> opcode;
	bitset<5> rs1;
	bitset<5> rs2;
	bitset<5> rd;

	bitset<1> bit30;
	bitset<3> bit14to12;
};

// Datapath: Each functional unit will expose a set of input(s), a set of output(s), and an update function to be called every cycle

class InstrMem {
private:
	char* imem;
public:	
	InstrMem() {};

	InstrMem(char* imem) {
		this->imem = imem;
	}

	// Outputs
	Instruction instr;

	// Returns a 32-bit instruction based on a read address
	Instruction fetch(bitset<32> readAddr);
	
	// Inputs are update() arguments
	void update(bitset<32> readAddr) {
		instr = fetch(readAddr);
	}
};

class RegFile {
private:
	// RISCV-32 ISA includes 32 registers x0-x31 (not including floating point), each 32 bits wide
	int32_t regFile[32];

	void write(bitset<5> registerNum, bitset<32> val){
		regFile[(int32_t) registerNum.to_ulong()] = (int32_t) val.to_ulong();
	}

public:
	// Outputs
	bitset<32> readData1;
	bitset<32> readData2;
	
	RegFile() {
		for (int i = 0; i < 32; i++) {
			regFile[i] = 0;
		}
	}

	// Must make this public so we can check a0 and a1
	int32_t read(bitset<5> registerNum){
		return regFile[(int32_t) registerNum.to_ulong()];
	}

	void regDump() {
		printf(" ");
		for (int i = 0; i < 32; i++) {
			printf("x%d = %d ", i, regFile[i]);
		}
		printf("\n");
	}

	void update_load(bitset<5> rs1, bitset<5> rs2) {
		readData1 = read(rs1);
		readData2 = read(rs2);
	}
	
	void update_store(bitset<5> rd, bitset<32> writeData, bitset<1> regWrite) {
		if (regWrite.to_ulong() == 1) write(rd, writeData);
	}
};

class ImmGen {
public:
	// Outputs
	bitset<32> imm;

	void update(bitset<32> instr, bitset<7> opcode) {
		// R-type: no immediate
		// I-type: imm[11:0] <-> bits 31:20
		// S-type: imm[11:5] <-> bits 31:25, imm[4:0] <-> bits 11:7
		// B-type: imm[12|10:5] <-> bits 31:25, imm[4:1|11] <-> 11:7
		// U-type: imm[31:12] <-> bits 31:12
		// J-type: imm[20|10:1|11|19:12] <-> bits 31:12
		switch (opcode.to_ulong()) {
			case 0b0110011: // R-type
				imm = bitset<32>(0);
				break;
			case 0b0010011: // I-type (not lw/lbu)	
			case 0b1100111: // jalr
			case 0b0000011: // lw / lbu
				imm = bitset<32>((instr.to_ulong() & 0xFFF00000) >> 20);
				break;
			case 0b0100011: { // sw / sh
				unsigned long imm_11_5 = (instr.to_ulong() & 0xFE000000) >> 20;
				unsigned long imm_4_0 = (instr.to_ulong() & 0x00000F80) >> 7;
				imm = bitset<32>(imm_11_5 & imm_4_0);	

				break;
			}
			case 0b1100011: { // bne
				unsigned long imm_12 = (instr.to_ulong() & 0x80000000) >> 12; // instr[31]
				unsigned long imm_10_5 = (instr.to_ulong() & 0x7E000000) >> 20; // instr[30:25]
				unsigned long imm_4_1 = (instr.to_ulong() & 0x00000F00) >> 7; // instr[11:8]
				unsigned long imm_11 = (instr.to_ulong() & 0x00000080) >> 4; // instr[7]	
				imm = bitset<32>(imm_12 & imm_10_5 & imm_4_1 & imm_11);

				break;
			}
			case 0b0110111: { // lui
				unsigned long imm_31_12 = instr.to_ulong() & 0xFFFFF000;	
				imm = bitset<32>(imm_31_12);

				break;
			}
			default:
				break;
		}
		// DEBUG
		cout << "immGen: " << imm << "\n";
	}
};

class Mux2 {
public:
	bitset<32> output;

	void update(bitset<32> input0, bitset<32> input1, bitset<1> sel) {
		if ((sel).to_ulong() == 1) output = input1;
		else if ((sel).to_ulong() == 0) output = input0;
	}
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

	void update(bitset<32> input0, bitset<32> input1, bitset<4> aluOp) {
		unsigned long input0_ulong = input0.to_ulong();
		unsigned long input1_ulong = input1.to_ulong();
		
		zero = bitset<32>(!(input0_ulong - input1_ulong)); // for bne (?)

		switch (aluOp.to_ulong()) {
			case ADD:
				aluResult = bitset<32>(input0_ulong + input1_ulong);
				break;
			case SUB:
				aluResult = bitset<32>(input0_ulong - input1_ulong);
				break;
			case SL:
				aluResult = bitset<32>(input0_ulong << input1_ulong);
				break;
			case SRA:
				aluResult = bitset<32>((int32_t) input0_ulong >> input1_ulong);
				break;
			case AND:
				aluResult = bitset<32>(input0_ulong & input1_ulong);
				break;
			case OR:
				aluResult = bitset<32>(input0_ulong | input1_ulong);
				break;
			case LESS:
				aluResult = bitset<32>((int32_t) input0_ulong < (int32_t) input1_ulong ? 1 : 0);
				break;
			case NOTEQUAL:
				aluResult = bitset<32>(input0_ulong != input1_ulong ? 1 : 0);
				break;
			case EQUAL:
				aluResult = bitset<32>(input0_ulong == input1_ulong ? 1 : 0);
				break;
			default:
				break;				
		}

		// DEBUG: Print aluResult, input0, input1, aluOp
		printf("ALU: input0 = %d, input1 = %d, aluOp = %d, aluResult = %d, zero = %d\n", (int32_t) input0_ulong, (int32_t) input1_ulong, (int32_t) aluOp.to_ulong(), (int32_t) aluResult.to_ulong(), (int32_t) zero.to_ulong());
	}
};

class Add {
public:
	bitset<32> sum;

	bitset<32> add(bitset<32> i1, bitset<32> i2) {
		bitset<32> result = bitset<32>((int32_t) i1.to_ulong() + (int32_t) i2.to_ulong());
		printf("ADD(%d, %d) = %d\n", (int32_t) i1.to_ulong(), (int32_t) i2.to_ulong(), (int32_t) result.to_ulong());
		return result;
	}

	void update(bitset<32> i1, bitset<32> i2) {
		sum = add(i1, i2);
	}
};

class ShiftLeft {
public:
	bitset<32> result;

	void update(bitset<32> input) {
		result = bitset<32>(input.to_ulong() << 1);
	}
};

class DataMem {
private:
	char dmemory[4096];

	bitset<32> read(bitset<32> addr_bitset) {
		int32_t addr = (int32_t) addr_bitset.to_ulong();
		
		if (addr + 3 > 4096) exit(1);

		int32_t ret = (dmemory[addr + 3] << 24) | (dmemory[addr + 2] << 16) | (dmemory[addr + 1] << 8) | dmemory[addr];

		return bitset<32>(ret);
	}

	void write(bitset<32> addr_bitset, bitset<32> writeData_bitset) {
		int32_t addr = (int32_t) addr_bitset.to_ulong();
		int32_t writeData = (int32_t) writeData_bitset.to_ulong();

		if (addr + 3 > 4096) exit(1);

		dmemory[addr + 3] = (writeData >> 24) & 0xFF;
		dmemory[addr + 2] = (writeData >> 16) & 0xFF;
		dmemory[addr + 1] = (writeData >> 8) & 0xFF;
		dmemory[addr] = writeData & 0xFF;
	}

public:
	DataMem() {
		for (int i = 0; i < 4096; i++) {
			dmemory[i] = 0;
		}
	}

	// Output
	bitset<32> readData;
	
	void update(bitset<32> addr, bitset<32> writeData, bitset<1> memRead, bitset<1> memWrite) {
		// TODO: Verify whether only one of memWrite/memRead can be 1 at the same time
		if (memWrite.to_ulong() == 1) {
			write(addr, writeData);
		}
		if (memRead.to_ulong() == 1) {
			readData = read(addr);
		}
	}
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

	void update(bitset<7> opcode){
		// Generate all signals except for aluOp
		switch (opcode.to_ulong()) {
			case 0b0110011: // R-type
				regWrite.set();
				aluSrc.reset();
				branch.reset();
				memRead.reset();
				memWrite.reset();
				memToReg.reset();

				aluOp = bitset<4>(R);

				break;
			case 0b0010011: // I-type
				regWrite.set();
				aluSrc.set();
				branch.reset();
				memRead.reset();
				memWrite.reset();
				memToReg.reset();

				aluOp = bitset<4>(I);

				break;
			case 0b0000011: // lw or lbu
				regWrite.set();
				aluSrc.set();
				branch.reset();
				memRead.set();
				memWrite.reset();
				memToReg.set();

				aluOp = bitset<4>(ADD);

				break;
			case 0b0100011: // sw 
				regWrite.reset();
				aluSrc.set();
				branch.reset();
				memRead.reset();
				memWrite.set();
				memToReg.reset();

				aluOp = bitset<4>(ADD);

				break;
			case 0b1100011: // bne
				regWrite.reset();
				aluSrc.reset();
				branch.set();
				memRead.reset();
				memWrite.reset();
				memToReg.reset();

				aluOp = bitset<4>(NOTEQUAL);

				break;

			case 0b0110111: // lui
				regWrite.set();
				aluSrc.set();
				branch.reset();
				memRead.reset();
				memWrite.reset();
				memToReg.reset();

				aluOp = bitset<4>(SL);

				break;

			case 0b1100111: // jalr
				regWrite.set();
				aluSrc.set();
				branch.set();
				memRead.reset();
				memWrite.reset();
				memToReg.reset();

				aluOp = bitset<4>(ADD);

				break;
			default:
				break;
		}
		
		// DEBUG print control signals on single line
		cout << "Control Signals:" 
			<< " regWrite: " << regWrite 
			<< " aluSrc: " << aluSrc 
			<< " branch: " << branch 
			<< " memRead: " << memRead 
			<< " memWrite: " << memWrite 
			<< " memToReg: " << memToReg 
			<< " aluOp: " << aluOp 
			<< "\n\n";
	}

};

class ALUController {
public:
	
	// Output
	bitset<4> aluControlOp; // Control signal to be routed to ALU

	void update(bitset<1> bit30, bitset<3> bits14to12, bitset<4> aluOp) {
		switch (aluOp.to_ulong()) {
			case ADD:
				aluControlOp = aluOp;
				break;
			case SUB:
				aluControlOp = aluOp;
				break;
			case SL:
				aluControlOp = aluOp;
				break;
			case NOTEQUAL:
				aluControlOp = aluOp;
				break;
			case R: 
				switch (bits14to12.to_ulong()) {
					case 0b000: // add or sub
						switch (bit30.to_ulong()) {
							case 0: // add
								aluControlOp = bitset<4>(ADD);
								break;
							case 1: // sub
								aluControlOp = bitset<4>(SUB);
								break;
							default:
								break;
						}	
						break;
					case 0b101: // sra
						aluControlOp = bitset<4>(SRA);
						break;
					default:
						break;
				}
				break;
			case I:
				switch (bits14to12.to_ulong()) {
					case 0b000: // addi
						aluControlOp = bitset<4>(ADD);
						break;
					case 0b110: // ori
						aluControlOp = bitset<4>(OR);
						break;
					case 0b011: // sltiu
						aluControlOp = bitset<4>(LESS);
						break;
					default:
						break;
				}		
				break;
			default:
				break;
		}
		// DEBUG
		cout << "aluControlOp: " << aluControlOp << "\n";
	}
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
	
	Mux2 aluMux; // Mux that selects between readData2 and immGen
	
	ALU alu;

	ShiftLeft shiftLeftOne;
	Add addPCImm;
	Mux2 pcMux; // Mux that selects between PC+4 and PC+imm
	
	DataMem dataMem;
	Mux2 wbMux;
	
	CPU(char* imem);

	void update(bool* done) {
		bitset<32> PC_bitset(PC);
		
		// Fetch
		instrMem.update(PC_bitset);
		Instruction cur_instr = instrMem.instr;

		if (cur_instr.opcode.to_ulong() == 0) {
			cout << "Reached OPCODE 0, exiting\n" << endl;
			*done = true;
		}
		
		controller.update(cur_instr.opcode); // Generate new control signals
		aluController.update(cur_instr.bit30, cur_instr.bit14to12, controller.aluOp);
	
		// Decode / Load
		regFile.update_load(cur_instr.rs1, cur_instr.rs2);

		immGen.update(cur_instr.instr, cur_instr.opcode);
		shiftLeftOne.update(immGen.imm);
		addPCImm.update(PC_bitset, shiftLeftOne.result);

		aluMux.update(regFile.readData2, immGen.imm, controller.aluSrc);	
		alu.update(regFile.readData1, aluMux.output, aluController.aluControlOp);
		
		dataMem.update(alu.aluResult, regFile.readData2, controller.memRead, controller.memWrite);
		wbMux.update(alu.aluResult, dataMem.readData, controller.memToReg);

		regFile.update_store(cur_instr.rd, wbMux.output, controller.regWrite);

		// Create nextPC = PC + 4
		addPC4.update(PC_bitset, bitset<32>(4)); 
		
		bitset<1> pcMuxControlSignal = bitset<1>(controller.branch.to_ulong() & alu.zero.to_ulong());

		pcMux.update(addPC4.sum, addPCImm.sum, pcMuxControlSignal);

		nextPC = pcMux.output.to_ulong();	
		printf("cur_PC = %lu, nextPC = %lu\n\n", PC, nextPC);

		regFile.regDump();
	};

};
