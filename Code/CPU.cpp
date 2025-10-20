#include "CPU.h"
#include <bitset>

// Constructor to parse raw 32-bit instruction bitset into fields
Instruction::Instruction(bitset<32> raw_instr) {

	// Field extraction based on RISC-V instruction format
	instr = raw_instr;
	opcode = bitset<7>(instr.to_ulong() & 0x7F);
	rs1 = bitset<5>((instr.to_ulong() >> 15) & 0x1F);
	rs2 = bitset<5>((instr.to_ulong() >> 20) & 0x1F);
	rd = bitset<5>((instr.to_ulong() >> 7) & 0x1F);
	bit30 = bitset<1>((instr.to_ulong() >> 30) & 1);
	bit14to12 = bitset<3>((instr.to_ulong() >> 12) & 7);

	if (raw_instr.to_ulong() == 0) return;

	/* DEBUG
	cout << "Instruction: " << instr << endl;
	cout << "Opcode: " << opcode << endl;
	cout << "rs1: " << rs1 << endl;
	cout << "rs2: " << rs2 << endl;
	cout << "rd: " << rd << endl;
	cout << "bit30: " << bit30 << endl;
	cout << "bit14to12: " << bit14to12 << "\n\n";
	*/
}

// Helper function to convert hex char to 4-bit binary string
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

// Default constructor
InstrMem::InstrMem() {};

// Initialize instruction memory with given imem array
InstrMem::InstrMem(char* imem) {
	this->imem = imem;
}

// Fetch instruction from imem at readAddr
Instruction InstrMem::fetch(bitset<32> readAddr) {
	uint32_t offset = (uint32_t) readAddr.to_ulong();
	
	// printf("Fetching instruction at address %lu\n", readAddr.to_ulong());

	// Check for out-of-bounds
	if (offset + 7 >= IMEMORY_SIZE) {
		throw std::runtime_error("readAddr for imem is out of bounds");
	}
	
	// Construct the 32-bit instruction from 4 contiguous bytes starting from readAddr in little-endian format
	uint32_t instr_0_7 = ((uint32_t) imem[offset]) & 0x000000FF;
	uint32_t instr_8_15 = (((uint32_t) imem[offset + 1]) << 8) & 0x0000FF00;
	uint32_t instr_16_23 = (((uint32_t) imem[offset + 2]) << 16) & 0x00FF0000;
	uint32_t instr_24_31 = (((uint32_t) imem[offset + 3]) << 24) & 0xFF000000;
	uint32_t instr = instr_24_31 | instr_16_23 | instr_8_15 | instr_0_7;	
	
	/* DEBUG
	printf("Instruction in hex: %x\n", instr);
	*/

	bitset<32> ret_instr(instr);
	return Instruction(ret_instr);
}

// Update instr based on fetching readAddr from imem (Fetch stage)
void InstrMem::update(bitset<32> readAddr) {
	instr = fetch(readAddr);
}

// Write val into specified register specified by registerNum
void RegFile::write(bitset<5> registerNum, bitset<32> val){
	if (registerNum.to_ulong() == 0) return;
	regFile[(int32_t) registerNum.to_ulong()] = (int32_t) val.to_ulong();
}

// Initialize all registers to zero
RegFile::RegFile() {
	for (int i = 0; i < 32; i++) {
		regFile[i] = 0;
	}
}

// Read data from specified register specified by registerNum
int32_t RegFile::read(bitset<5> registerNum){
	return regFile[(int32_t) registerNum.to_ulong()];
}

// Debug function to print all registers
void RegFile::regDump() {
	printf(" ");
	for (int i = 0; i < 32; i++) {
		printf("x%d = %x ", i, regFile[i]);
	}
	printf("\n");
}

// Update readData1 and readData2 based on rs1 and rs2 (Decode stage)
void RegFile::update_load(bitset<5> rs1, bitset<5> rs2) {
	readData1 = read(rs1);
	readData2 = read(rs2);
	
	/* DEBUG
	printf("RegFile: readData1 = %d, readData2 = %d\n", (int32_t) readData1.to_ulong(), (int32_t) readData2.to_ulong());
	*/
}

// Store writeData into rd if regWrite is set
void RegFile::update_store(bitset<5> rd, bitset<32> writeData, bitset<1> regWrite) {
	if (regWrite.to_ulong() == 1) write(rd, writeData);
}

// Update instruction based on opcode and instruction
void ImmGen::update(bitset<32> instr, bitset<7> opcode) {
	// R-type: no immediate
	// I-type: imm[11:0] <-> bits 31:20
	// S-type: imm[11:5] <-> bits 31:25, imm[4:0] <-> bits 11:7
	// B-type: imm[12|10:5] <-> bits 31:25, imm[4:1|11] <-> 11:7
	// U-type: imm[31:12] <-> bits 31:12
	// J-type: imm[20|10:1|11|19:12] <-> bits 31:12
	int32_t instr_signed = (int32_t) instr.to_ulong();
	switch (opcode.to_ulong()) {
		case 0b0110011: // R-type
			imm = bitset<32>(0);
			break;
		case 0b0010011: // I-type (not lw/lbu)	
		case 0b1100111: // jalr
		case 0b0000011: // lw / lbu
			imm = bitset<32>(((int32_t) (instr_signed & 0xFFF00000)) >> 20);
			break;
		case 0b0100011: { // sw / sh
			int32_t imm_11_5 = ((int32_t) (instr_signed & 0xFE000000)) >> 20;
			int32_t imm_4_0 = ((int32_t) (instr_signed & 0x00000F80)) >> 7;
			imm = bitset<32>(imm_11_5 | imm_4_0);	

			break;
		}
		case 0b1100011: { // bne
			int32_t imm_12 = ((int32_t) (instr_signed & 0x80000000)) >> 19; // instr[31]
			int32_t imm_10_5 = ((int32_t) (instr_signed & 0x7E000000)) >> 20; // instr[30:25]
			int32_t imm_4_1 = ((int32_t) (instr_signed & 0x00000F00)) >> 7; // instr[11:8]
			int32_t imm_11 = ((int32_t) (instr_signed & 0x00000080)) << 4; // instr[7]	
			imm = bitset<32>((imm_12 | imm_10_5 | imm_4_1 | imm_11) >> 1);

			/* DEBUG
			cout << "imm parts: imm_12=" << bitset<32>(imm_12) << ", imm_10_5=" << bitset<32>(imm_10_5) << ", imm_4_1=" << bitset<32>(imm_4_1) << ", imm_11=" << bitset<32>(imm_11) << "\n";
			*/

			break;
		}
		case 0b0110111: { // lui
			int32_t imm_31_12 = instr_signed & 0xFFFFF000;	
			// int32_t imm_final = imm_31_12 << 12; // Perform the left shift for the lui instr
			imm = bitset<32>(imm_31_12);

			break;
		}
		default:
			break;
	}
	/* DEBUG
	cout << "immGen: " << imm << "\n";
	*/
}

// Select between two inputs based on sel bit
void Mux2::update(bitset<32> input0, bitset<32> input1, bitset<1> sel) {
	if ((sel).to_ulong() == 1) output = input1;
	else if ((sel).to_ulong() == 0) output = input0;
}

// Select between three inputs based on two sel bits. Made for writeback Mux
void Mux3::update(bitset<32> input0, bitset<32> input1, bitset<32> input2, bitset<1> sel0, bitset<1> sel1) {
	/* DEBUG: Print inputs and selects
	printf("Mux3: input0 = %d, input1 = %d, input2 = %d, sel0 = %d, sel1 = %d\n", (int32_t) input0.to_ulong(), (int32_t) input1.to_ulong(), (int32_t) input2.to_ulong(), (int32_t) sel0.to_ulong(), (int32_t) sel1.to_ulong());
	*/
	if (sel0.to_ulong() == 0 && sel1.to_ulong() == 0) { // If sel0 = 0 and sel1 = 0, select input0
		output = input0;
	} else if (sel0.to_ulong() == 1 && sel1.to_ulong() == 0) { // If sel0 = 1 and sel1 = 0, select input1
		output = input1;
	} else if (sel0.to_ulong() == 0 && sel1.to_ulong() == 1) { // If sel0 = 0 and sel1 = 1, select input2
		output = input2;
	}
	/*
	printf("Mux3 output: %d\n", (int32_t) output.to_ulong());
	*/
}

// Update ALU based on two inputs and specified aluOp
void ALU::update(bitset<32> input0, bitset<32> input1, bitset<4> aluOp) {
	int32_t input0_ulong = (int32_t) input0.to_ulong();
	int32_t input1_ulong = (int32_t) input1.to_ulong();
	
	zero = bitset<32>(!((input0_ulong - input1_ulong) == 0)); // Calculate zero signal for bne, to be used with branch control signal
	
	// Perform ALU operation based on aluOp
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
			aluResult = bitset<32>(input0_ulong >> (uint32_t) input1_ulong);
			break;
		case AND:
			aluResult = bitset<32>(input0_ulong & input1_ulong);
			break;
		case OR:
			aluResult = bitset<32>(input0_ulong | input1_ulong);
			break;
		case LESS:
			aluResult = bitset<32>(input0_ulong < input1_ulong ? 1 : 0);
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

	/* DEBUG: Print aluResult, input0, input1, aluOp
	printf("ALU: input0 = %d, input1 = %d, aluOp = %d, aluResult = %d, zero = %d\n", (int32_t) input0_ulong, (int32_t) input1_ulong, (int32_t) aluOp.to_ulong(), (int32_t) aluResult.to_ulong(), (int32_t) zero.to_ulong());
	*/
}

// Helper function to add two bitsets
bitset<32> Add::add(bitset<32> i1, bitset<32> i2) {
	bitset<32> result = bitset<32>((int32_t) i1.to_ulong() + (int32_t) i2.to_ulong());
	// printf("ADD(%d, %d) = %d\n", (int32_t) i1.to_ulong(), (int32_t) i2.to_ulong(), (int32_t) result.to_ulong());
	return result;
}

// Add two inputs, store in sum (a class member)
void Add::update(bitset<32> i1, bitset<32> i2) {
	sum = add(i1, i2);
}

// Shift input left by 1, store in result (a class member)
void ShiftLeft::update(bitset<32> input) {
	result = bitset<32>(input.to_ulong() << 1);
}

// Read data from dmemory at given address, with read size specified by rw_size
bitset<32> DataMem::read(bitset<32> addr_bitset, bitset<2> rw_size) {
	uint32_t addr = (uint32_t) addr_bitset.to_ulong(); // addr should be unsigned because it can't be negative
	uint32_t size = rw_size.to_ulong();
	
	// Check for out-of-bounds
	if (addr < 0) {
		printf("ERROR DataMem::read: addr < 0\n");
		exit(1);
	}
	if (addr + 3 > DMEMORY_SIZE) {
		printf("ERROR Datamem::read: addr + 3 > DMEMORY_SIZE\n");
		exit(1);
	}

	int32_t ret;

	if (size == WORD) { // Read 4 bytes (remember little-endian). Construct the 32-bit word from 4 extracted bytes
		ret = (((uint32_t) dmemory[addr + 3] << 24) & 0xFF000000) | ((((uint32_t) dmemory[addr + 2]) << 16) & 0x00FF0000) | ((((uint32_t) dmemory[addr + 1]) << 8) & 0x0000FF00) | (((uint32_t) dmemory[addr]) & 0xFF);
		/* DEBUG: Print part of memory that got read with address and data
		printf("DMEM READ, WORD: Address %d to %d, Data %x\n", addr, addr + 3, ret);
		*/
	}
	else if (size == BYTE) { // Read 1 byte
		ret = (uint8_t) dmemory[addr]; // Should zero-extend
		/* DEBUG
		printf("DMEM READ, BYTE: Address %d, Data %hhx\n", addr, dmemory[addr]);
		*/
	}
	
	return bitset<32>(ret);
}

// Write data to dmemory at given address, with size specified by rw_size
void DataMem::write(bitset<32> addr_bitset, bitset<32> writeData_bitset, bitset<2> rw_size) {
	uint32_t addr = (uint32_t) addr_bitset.to_ulong();
	int32_t writeData = (int32_t) writeData_bitset.to_ulong();
	uint32_t size = rw_size.to_ulong();
	
	// Check for out-of-bounds
	if (addr < 0) {
		// printf("ERROR DataMem::read: addr < 0\n");
		exit(1);
	}
	if (addr + 3 > DMEMORY_SIZE) {
		// printf("ERROR Datamem::read: addr + 3 > DMEMORY_SIZE\n");
		exit(1);
	}
	
	if (size == WORD) { // Write the data (4 bytes) in little endian
		dmemory[addr + 3] = (writeData >> 24) & 0xFF; // Most significant byte
		dmemory[addr + 2] = (writeData >> 16) & 0xFF;
		dmemory[addr + 1] = (writeData >> 8) & 0xFF; 
		dmemory[addr] = writeData & 0xFF; // Least significant byte
		
		/* DEBUG: Print part of memory that got written to with address and data
		for (int i = 0; i < 4; i++) {
			printf("DMEM WRITE, WORD: Address %d, Data %hhx\n", addr + i, (int8_t) dmemory[addr + i]);
		}
		*/
	} else if (size == HALF) { // Write the data (2 bytes) in little endian
		dmemory[addr + 1] = (uint8_t) ((writeData >> 8) & 0xFF);
		dmemory[addr] = (uint8_t) (writeData & 0xFF);
		
		/* DEBUG: Print part of memory that got written to with address and data
		for (int i = 0; i < 2; i++) {
			printf("DMEM WRITE, HALF: Address %d, Data %hhx\n", addr + i, (int8_t) dmemory[addr + i]);
		}
		*/
	}


}

// Initialize dmemory to zero
DataMem::DataMem() {
	for (int i = 0; i < DMEMORY_SIZE; i++) {
		dmemory[i] = 0;
	}
}

// Update DataMem based on control signals and either read or write data
void DataMem::update(bitset<32> addr, bitset<32> writeData, bitset<1> memRead, bitset<1> memWrite, bitset<2> rw_size) {
	// TODO: Verify whether only one of memWrite/memRead can be 1 at the same time
	if (memWrite.to_ulong() == 1) { // If memWrite is set, perform write to given addr
		write(addr, writeData, rw_size);
	}
	if (memRead.to_ulong() == 1) { // If memRead is set, perform read from given addr
		readData = read(addr, rw_size);
	}
}

// Update control signals based on opcode of current instruction
void Controller::update(bitset<7> opcode){
	// Generate all signals except for aluOp. Report attached to code documents which control signals are set for which instructions
	switch (opcode.to_ulong()) {
		case 0b0110011: // R-type
			regWrite.set();
			aluSrc.reset();
			branch.reset();
			memRead.reset();
			memWrite.reset();
			memToReg.reset();

			lui.reset();
			jalr.reset();

			aluOp = bitset<4>(R); // Need funct3 and funct7 to distinguish which specific R-type instruction it is

			// printf("Controller: R-type case\n");

			break;
		case 0b0010011: // I-type
			regWrite.set();
			aluSrc.set();
			branch.reset();
			memRead.reset();
			memWrite.reset();
			memToReg.reset();

			lui.reset();
			jalr.reset();

			aluOp = bitset<4>(I); // Need funct3 to distinguish which specific I-type instruction it is

			// printf("Controller: I-type case\n");

			break;
		case 0b0000011: // lw or lbu
			regWrite.set();
			aluSrc.set();
			branch.reset();
			memRead.set();
			memWrite.reset();
			memToReg.set();

			lui.reset();
			jalr.reset();

			aluOp = bitset<4>(L); // Need funct3 to distinguish lw vs lbu, in order to set rw_size control bit
			// Otherwise, both lw and lbu use aluOp = ADD (rs1 + imm)

			// printf("Controller: lw/lbu case\n");

			break;
		case 0b0100011: // sw or sh
			regWrite.reset();
			aluSrc.set();
			branch.reset();
			memRead.reset();
			memWrite.set();
			memToReg.reset();

			lui.reset();
			jalr.reset();

			aluOp = bitset<4>(S); // Need further distinction based on funct3 to distinguish sw vs sh, in order to set rw_size control bit
			// Otherwise, both sw and sh use aluOp = ADD (rs1 + imm) to compute address, which we set in the ALUController

			// printf("Controller: sw case\n");

			break;
		case 0b1100011: // bne
			regWrite.reset();
			aluSrc.reset();
			branch.set();
			memRead.reset();
			memWrite.reset();
			memToReg.reset();

			lui.reset();
			jalr.reset();

			aluOp = bitset<4>(NOTEQUAL); // For rs1 != rs2

			// printf("Controller: bne case\n");

			break;

		case 0b0110111: // lui
			regWrite.set();
			aluSrc.set();
			branch.reset();
			memRead.reset();
			memWrite.reset();
			memToReg.reset();

			lui.set();
			jalr.reset();

			aluOp = bitset<4>(ADD); // For 0 + (imm << 12)

			// printf("Controller: lui case\n");

			break;

		case 0b1100111: // jalr
			regWrite.set();
			aluSrc.set();
			branch.reset();
			memRead.reset();
			memWrite.reset();
			memToReg.reset();

			lui.reset();
			jalr.set();

			aluOp = bitset<4>(ADD); // For rs1 + imm

			// printf("Controller: jalr case\n");

			break;
		default:
			break;
	}
	
	/* DEBUG print control signals on single line
	cout << "Control Signals:" 
		<< " regWrite: " << regWrite 
		<< " aluSrc: " << aluSrc 
		<< " branch: " << branch 
		<< " memRead: " << memRead 
		<< " memWrite: " << memWrite 
		<< " memToReg: " << memToReg 
		<< " aluOp: " << aluOp 
		<< " lui: " << lui
		<< " jalr: " << jalr
		<< "\n\n";
	*/
}

// Update the final aluControlOp based on the aluOp from Controller and funct3/funct7 bits from instruction
void ALUController::update(bitset<1> bit30, bitset<3> bits14to12, bitset<4> aluOp) {
	switch (aluOp.to_ulong()) {
		// For these cases where aluOp is decided by the opcode alone, set aluControlOp directly
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
			switch (bits14to12.to_ulong()) { // funct3
				case 0b000: // add or sub
					switch (bit30.to_ulong()) { // funct7[5]
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
				case 0b111: // and
					aluControlOp = bitset<4>(AND);
					break;
				default:
					break;
			}
			break;
		case I:
			switch (bits14to12.to_ulong()) { // funct3
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
		case L:
			switch (bits14to12.to_ulong()) { // funct3
				case 0b010: // lw
					aluControlOp = bitset<4>(ADD);
					rw_size = bitset<2>(WORD);
					break;
				case 0b100: // lbu
					aluControlOp = bitset<4>(ADD);
					rw_size = bitset<2>(BYTE);
					break;
			}
			break;
		case S:
			switch (bits14to12.to_ulong()) { // funct3
				case 0b010: // sw
					aluControlOp = bitset<4>(ADD);
					rw_size = bitset<2>(WORD);
					break;
				case 0b001: // sh
					aluControlOp = bitset<4>(ADD);
					rw_size = bitset<2>(HALF);
					break;
			}
			break;
		default:
			break;
	}
	/* DEBUG
	cout << "aluControlOp: " << aluControlOp << "\n";
	*/
}

// CPU initializes PC and instruction memory
CPU::CPU(char* imem)
{
	PC = 0; //set PC to 0
	this->instrMem = InstrMem(imem);
}

// Returns current PC
unsigned long CPU::readPC()
{
	return PC;
}

// Sets PC to nextPC
void CPU::incPC()
{
	// printf("Incrementing PC from %lu to %lu\n\n", PC, nextPC);
	PC = nextPC;
}

// Runs one cycle of the CPU
void CPU::update(bool* done) {
	bitset<32> PC_bitset(PC);
	
	// Fetch Stage
	instrMem.update(PC_bitset);
	Instruction cur_instr = instrMem.instr;
	
	// Check for opcode 0 to halt
	if (cur_instr.opcode.to_ulong() == 0) {
		// cout << "Reached OPCODE 0, exiting\n" << endl;
		*done = true;
		return;
	}
	
	// Update control signals based on opcode, funct3, funct7
	controller.update(cur_instr.opcode); // Generate new control signals
	aluController.update(cur_instr.bit30, cur_instr.bit14to12, controller.aluOp);

	// Decode / Load Stage
	regFile.update_load(cur_instr.rs1, cur_instr.rs2);
	
	immGen.update(cur_instr.instr, cur_instr.opcode); // Generate immediate
	shiftLeftOne.update(immGen.imm); // Shift imm left by 1
	addPCImm.update(PC_bitset, shiftLeftOne.result); // add PC to imm
	
	// Execute Stage
	luiMux.update(regFile.readData1, bitset<32>(0), controller.lui); // Mux that selects between 0 (if lui control bit is set) and rs1 (if lui control bit is not set)
	aluMux.update(regFile.readData2, immGen.imm, controller.aluSrc); // Mux that selects between readData2 and immGen depending on aluSrc control signal
	alu.update(luiMux.output, aluMux.output, aluController.aluControlOp); // ALU computation

	// For nextPC
	addPC4.update(PC_bitset, bitset<32>(4)); // PC + 4
	jalrPCMux.update(addPC4.sum, alu.aluResult, controller.jalr); // Mux selects between PC+4 and rs1+imm
	
	// Memory Stage
	dataMem.update(alu.aluResult, regFile.readData2, controller.memRead, controller.memWrite, aluController.rw_size);

	// Writeback Stage
	wbMux.update(alu.aluResult, dataMem.readData, addPC4.sum, controller.memToReg, controller.jalr);
	regFile.update_store(cur_instr.rd, wbMux.output, controller.regWrite);

	bitset<1> pcMuxControlSignal = bitset<1>(controller.branch.to_ulong() & alu.zero.to_ulong());
	
	// PC Update
	pcMux.update(jalrPCMux.output, addPCImm.sum, pcMuxControlSignal);

	nextPC = pcMux.output.to_ulong();	
	// printf("cur_PC = %lu, nextPC = %lu\n\n", PC, nextPC);

	// regFile.regDump();
};
