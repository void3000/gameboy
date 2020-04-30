// Dummy constants
const START_ADDRESS = 0x0000;
const NUM_REGISTERS = 0x0008;

// GameBoy memory
var mem = new Uint8Array(0x10000);


// GameBoy CPU components
var PC = 0x0000;

// Registers
var REG = new Uint8Array(0x08);

// Register index
//
// Register identifier has the following notation:
//
// 	0x0XYZ
// where,
// X is used to identify the register type and has the following notation:
//
// 	0b000 - Any single register
// 	0b001 - AF
//	0b010 - BC
//	0b011 - DE
//	0b100 - HL
//	0b101 - SP
//	0b110 - PC
//
// YZ are used for indexing
const A = 0x0001;
const B = 0x0002;
const C = 0x0003;
const D = 0x0004;
const E = 0x0005;
const F = 0x0006;
const H = 0x0007;
const L = 0x0008;

const AF = 0x0000;
const BC = 0x0000;
const DE = 0x0000;
const HL = 0x0000;

// Addressing modes
const IMMEDIATE_TO_REGISTER = 0x0001;
const IMMEDIATE_TO_IMMEDIATE= 0x0002;
const REGISTER_TO_IMMEDIATE = 0X0003;
const REGISTER_TO_REGISTER  = 0X0004;

function ld_nn_n(nn, n) {
	return function() {
		REG[nn] = mem[n];
	}
}

function ld_r1_r2(r1, r2, mode) {
	return function() {
		if (mode == IMMEDIATE_TO_REGISTER) {
			REG[r1] = mem[r2];
		} 
		else if (mode == REGISTER_TO_IMMEDIATE) {
			mem[r1] = REG[r2];
		} 
		else if (mode == REGISTER_TO_REGISTER){
			REG[r1] = REG[r2];
		}
		else if (mode == IMMEDIATE_TO_IMMEDIATE) {
			mem[r1] = mem[r2];
		}
		else {
			REG[r1] = r2;
		}
	}
}

function read_16b_reg(a, b) {
	return ((REG[a] << 8) | REG[b]) & 0xffff;
}

// operation code mapping
var opcode = new Array(256);

for(let i = 0; i < 256; ++i) opcode[i] = function() {
    throw Error("Invalid opcode.");
}; 

opcode[0x06] = ld_nn_n(B, PC);
opcode[0x0e] = ld_nn_n(C, PC);
opcode[0x16] = ld_nn_n(D, PC);
opcode[0x1e] = ld_nn_n(E, PC);
opcode[0x26] = ld_nn_n(H, PC);
opcode[0x2e] = ld_nn_n(L, PC);

opcode[0x7f] = ld_r1_r2(A, A, REGISTER_TO_REGISTER);
opcode[0x78] = ld_r1_r2(A, B, REGISTER_TO_REGISTER);
opcode[0x79] = ld_r1_r2(A, C, REGISTER_TO_REGISTER);
opcode[0x7a] = ld_r1_r2(A, D, REGISTER_TO_REGISTER);
opcode[0x7b] = ld_r1_r2(A, E, REGISTER_TO_REGISTER);
opcode[0x7c] = ld_r1_r2(A, H, REGISTER_TO_REGISTER);
opcode[0x7d] = ld_r1_r2(A, L, REGISTER_TO_REGISTER);
opcode[0x7e] = ld_r1_r2(A, read_16b_reg(H, L), IMMEDIATE_TO_REGISTER);
opcode[0x40] = ld_r1_r2(B, B, REGISTER_TO_REGISTER);
opcode[0x41] = ld_r1_r2(B, C, REGISTER_TO_REGISTER);
opcode[0x42] = ld_r1_r2(B, D, REGISTER_TO_REGISTER);
opcode[0x43] = ld_r1_r2(B, E, REGISTER_TO_REGISTER);
opcode[0x44] = ld_r1_r2(B, H, REGISTER_TO_REGISTER);
opcode[0x45] = ld_r1_r2(B, L, REGISTER_TO_REGISTER);
opcode[0x46] = ld_r1_r2(B, read_16b_reg(H, L), IMMEDIATE_TO_REGISTER);
opcode[0x48] = ld_r1_r2(C, B, REGISTER_TO_REGISTER);
opcode[0x49] = ld_r1_r2(C, C, REGISTER_TO_REGISTER);
opcode[0x4A] = ld_r1_r2(C, D, REGISTER_TO_REGISTER);
opcode[0x4B] = ld_r1_r2(C, E, REGISTER_TO_REGISTER);
opcode[0x4C] = ld_r1_r2(C, H, REGISTER_TO_REGISTER);
opcode[0x4D] = ld_r1_r2(C, L, REGISTER_TO_REGISTER);
opcode[0x4e] = ld_r1_r2(C, read_16b_reg(H, L), IMMEDIATE_TO_REGISTER);
opcode[0x50] = ld_r1_r2(D, B, REGISTER_TO_REGISTER);
opcode[0x51] = ld_r1_r2(D, C, REGISTER_TO_REGISTER);
opcode[0x52] = ld_r1_r2(D, D, REGISTER_TO_REGISTER);
opcode[0x53] = ld_r1_r2(D, E, REGISTER_TO_REGISTER);
opcode[0x54] = ld_r1_r2(D, H, REGISTER_TO_REGISTER);
opcode[0x55] = ld_r1_r2(D, L, REGISTER_TO_REGISTER);
opcode[0x56] = ld_r1_r2(D, read_16b_reg(H, L), IMMEDIATE_TO_REGISTER);
opcode[0x58] = ld_r1_r2(E, B, REGISTER_TO_REGISTER);
opcode[0x59] = ld_r1_r2(E, C, REGISTER_TO_REGISTER);
opcode[0x5a] = ld_r1_r2(E, D, REGISTER_TO_REGISTER);
opcode[0x5b] = ld_r1_r2(E, E, REGISTER_TO_REGISTER);
opcode[0x5c] = ld_r1_r2(E, H, REGISTER_TO_REGISTER);
opcode[0x5d] = ld_r1_r2(E, L, REGISTER_TO_REGISTER);
opcode[0x5e] = ld_r1_r2(E, read_16b_reg(H, L), IMMEDIATE_TO_REGISTER);
opcode[0x60] = ld_r1_r2(H, B, REGISTER_TO_REGISTER);
opcode[0x61] = ld_r1_r2(H, C, REGISTER_TO_REGISTER);
opcode[0x62] = ld_r1_r2(H, D, REGISTER_TO_REGISTER);
opcode[0x63] = ld_r1_r2(H, E, REGISTER_TO_REGISTER);
opcode[0x64] = ld_r1_r2(H, H, REGISTER_TO_REGISTER);
opcode[0x65] = ld_r1_r2(H, L, REGISTER_TO_REGISTER);
opcode[0x66] = ld_r1_r2(H, read_16b_reg(H, L), IMMEDIATE_TO_REGISTER);
opcode[0x68] = ld_r1_r2(L, B, REGISTER_TO_REGISTER);
opcode[0x69] = ld_r1_r2(L, C, REGISTER_TO_REGISTER);
opcode[0x6a] = ld_r1_r2(L, D, REGISTER_TO_REGISTER);
opcode[0x6b] = ld_r1_r2(L, E, REGISTER_TO_REGISTER);
opcode[0x6c] = ld_r1_r2(L, H, REGISTER_TO_REGISTER);
opcode[0x6d] = ld_r1_r2(L, L, REGISTER_TO_REGISTER);
opcode[0x6e] = ld_r1_r2(L, read_16b_reg(H, L), IMMEDIATE_TO_REGISTER);
opcode[0x70] = ld_r1_r2(read_16b_reg(H, L), B, REGISTER_TO_IMMEDIATE);
opcode[0x71] = ld_r1_r2(read_16b_reg(H, L), C, REGISTER_TO_IMMEDIATE);
opcode[0x72] = ld_r1_r2(read_16b_reg(H, L), D, REGISTER_TO_IMMEDIATE);
opcode[0x73] = ld_r1_r2(read_16b_reg(H, L), E, REGISTER_TO_IMMEDIATE);
opcode[0x74] = ld_r1_r2(read_16b_reg(H, L), H, REGISTER_TO_IMMEDIATE);
opcode[0x75] = ld_r1_r2(read_16b_reg(H, L), L, REGISTER_TO_IMMEDIATE);
opcode[0x36] = ld_r1_r2(read_16b_reg(H, L), PC, IMMEDIATE_TO_IMMEDIATE);

function step() {
	let ins = mem[PC];
	opcode[ins]();
	PC++;
}
