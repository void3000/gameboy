// Dummy constants
const START_ADDRESS = 0x0000;
const NUM_REGISTERS = 0x0008;

// GameBoy memory
var mem = new Uint8Array(0x10000);


// GameBoy CPU components
var PC = 0x0000;
var SP = 0x0000;

// Register flags 
var Z_flag = 0x00;
var N_flag = 0x00;
var H_flag = 0x00;
var C_flag = 0x00;

// Registers
var REG = new Uint8Array(0x08);

// Register index
const A = 0x0000;
const B = 0x0001;
const C = 0x0002;
const D = 0x0003;
const E = 0x0004;
const F = 0x0005;
const H = 0x0006;
const L = 0x0007;

// Addressing modes
const IMMEDIATE_TO_REGISTER = 0x0001;
const IMMEDIATE_TO_IMMEDIATE= 0x0002;
const REGISTER_TO_IMMEDIATE = 0X0003;
const REGISTER_TO_REGISTER  = 0X0004;
const REGISTER 				= 0x0005;
const IMMEDIATE				= 0x0006;

function ld_nn_n(nn, n) {
	return function() {
		REG[nn] = mem_read_8b(n);
	}
}

function ld_r1_r2(r1, r2, mode) {
	return function() {
		if (mode == IMMEDIATE_TO_REGISTER) {
			REG[r1] = mem_read_8b(r2);
		} 
		else if (mode == REGISTER_TO_IMMEDIATE) {
			mem_write_8b(r2, REG[r2]);
		} 
		else if (mode == REGISTER_TO_REGISTER){
			REG[r1] = REG[r2];
		}
		else if (mode == IMMEDIATE_TO_IMMEDIATE) {
			mem_write_8b(r1, mem_read_8b(r2));
		}
	}
}

function ld_rr_nn(h, l, nn) {
	return function() {
		REG[l] = (nn & 0xff);
		REG[h] = ((nn >> 8) & 0xff);
	}
}

function push_nn(h, l) {
	mem_write_16b(SP, read_16b_reg(h, l));
}

function pop_nn(h, l) {
	return function() {
		let nn = mem_read_16b(SP);
		REG[l] = (nn & 0xff);
		REG[h] = ((nn >> 8) & 0xff);
	}
}

function add_r_n(r, n, carry, mode) {
	return function() {
		if (mode == REGISTER_TO_REGISTER) {
			REG[r] += REG[n] + (carry ? C : 0);
		}
		else if (mode == IMMEDIATE_TO_REGISTER) {
			REG[r] += mem_read_8b(n) + (carry ? C : 0);
		}
	}
}	

function sub_r_n(r, n, carry, mode) {
	return function() {
		if (mode == REGISTER_TO_REGISTER) {
			REG[r] -= REG + (carry ? C : 0);
		}
		else if (mode == IMMEDIATE_TO_REGISTER) {
			REG[r] -= mem_read_8b(n) + (carry ? C : 0);
		}
	}
}

function and_r_n(r, n, mode) {
	return function() {
		if (mode == REGISTER_TO_REGISTER) {
			REG[r] &= REG[n];
		}
		else if (mode == IMMEDIATE_TO_REGISTER) {
			REG[r] &= mem_read_8b(n);
		}
	}
}

function or_r_n(r, n, mode) {
	return function() {
		if (mode == REGISTER_TO_REGISTER) {
			REG[r] |= REG[n];
		}
		else if (mode == IMMEDIATE_TO_REGISTER) {
			REG[r] |= mem_read_8b(n);
		}
	}
}

function xor_r_n(r, n, mode) {
	return function() {
		if (mode == REGISTER_TO_REGISTER) {
			REG[r] ^= REG[n];
		}
		else if (mode == IMMEDIATE_TO_REGISTER) {
			REG[r] ^= mem_read_8b(n);
		}
	}
}

function cp_r_n(r, n, mode) {
	return function() {
		let result = 0;
		if (mode == REGISTER_TO_REGISTER) {
			result = REG[r] - REG[n];
		}
		else if (mode == IMMEDIATE_TO_REGISTER) {
			result = REG[r] - mem_read_8b(n);
		}
		// Update the flag registers using `result`.
	}
}

function inc_n(n, mode) {
	return function() {
		if (mode == REGISTER) {
			REG[n] += 1;
		}
		else if (mode == IMMEDIATE) {
			let result = mem_read_8b(n) + 1;
			mem_write_8b(n, result);
		}
	}
}

function dec_n(n, mode) {
	return function() {
		if (mode == REGISTER) {
			REG[n] += 1;
		}
		else if (mode == IMMEDIATE) {
			let result = mem_read_8b(n) + 1;
			mem_write_8b(n, result);
		}
	}
}

function add_rr_n(r1h, r1l, r2h, r2l, mode) {
	return function() {
		let ll = REG[r1l] + REG[r2l];
		let hh = REG[r1h] + REG[r2h] + (ll > 0xff ? 1 : 0);
		
		REG[r1l] = ll & 0xff;
		REG[r1h] = hh & 0xff;
	}
}

function inc_nn(h, l) {
	return function() {
		let ll = REG[l] + 1;
		let hh = REG[h] + (ll > 0xff ? 1 : 0);

		REG[L] = ll & 0xff;
		REG[H] = hh & 0xff;
	}
}

// Util functions
function read_8b_reg(a) {
	return REG[a];
}

function read_16b_reg(a, b) {
	return ((REG[a] << 8) | REG[b]) & 0xffff;
}

function mem_read_8b(addr) {
	return mem[addr] & 0xff;
}

function mem_write_8b(addr, data) {
	mem[addr] = data & 0xff;
}

function mem_read_16b(addr) {
	return ((mem[(++addr)] << 8) | mem[addr]) & 0xffff;
}

function mem_write_16b(addr, data) {
	mem[addr] = (data & 0xff);
	mem[++addr] = ((data >> 8) & 0xff);
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
opcode[0x0a] = ld_r1_r2(A, read_16b_reg(B, C), IMMEDIATE_TO_REGISTER);
opcode[0x1a] = ld_r1_r2(A, read_16b_reg(D, E), IMMEDIATE_TO_REGISTER);
opcode[0xfa] = ld_r1_r2(A, mem_read_16b(PC), IMMEDIATE_TO_REGISTER);
opcode[0x3e] = ld_r1_r2(A, PC, IMMEDIATE_TO_REGISTER);
opcode[0x47] = ld_r1_r2(B, A, REGISTER_TO_REGISTER);
opcode[0x4f] = ld_r1_r2(C, A, REGISTER_TO_REGISTER);
opcode[0x57] = ld_r1_r2(D, A, REGISTER_TO_REGISTER);
opcode[0x5f] = ld_r1_r2(E, A, REGISTER_TO_REGISTER);
opcode[0x67] = ld_r1_r2(H, A, REGISTER_TO_REGISTER);
opcode[0x6f] = ld_r1_r2(L, A, REGISTER_TO_REGISTER);
opcode[0x02] = ld_r1_r2(read_16b_reg(B, C), A, REGISTER_TO_IMMEDIATE);
opcode[0x12] = ld_r1_r2(read_16b_reg(D, E), A, REGISTER_TO_IMMEDIATE);
opcode[0x77] = ld_r1_r2(read_16b_reg(H, L), A, REGISTER_TO_IMMEDIATE);
opcode[0x77] = ld_r1_r2(read_16b_reg(H, L), A, REGISTER_TO_IMMEDIATE);
opcode[0xea] = ld_r1_r2(mem_read_16b(PC), A, REGISTER_TO_IMMEDIATE);
opcode[0xf2] = ld_r1_r2(A, ( 0xff00 + read_8b_reg(C)), IMMEDIATE_TO_REGISTER);
opcode[0xe2] = ld_r1_r2((0xff00 + read_8b_reg(C)), A, REGISTER_TO_IMMEDIATE);
opcode[0xe0] = ld_r1_r2((0xff00 + mem_read_8b(PC)), A, REGISTER_TO_IMMEDIATE);
opcode[0xf0] = ld_r1_r2(A, (0xff00 + mem_read_8b(PC)), IMMEDIATE_TO_REGISTER);
opcode[0x01] = ld_rr_nn(B, C, mem_read_16b(PC), IMMEDIATE_TO_REGISTER);
opcode[0x11] = ld_r1_r2(D, E, mem_read_16b(PC), IMMEDIATE_TO_REGISTER);
opcode[0x21] = ld_r1_r2(H, L, mem_read_16b(PC), IMMEDIATE_TO_REGISTER);
opcode[0x31] = function() { SP = mem_read_16b(PC); }
opcode[0xf9] = function() { SP = read_16b_reg(H, L); }
opcode[0xf8] = function() { let nn = SP + mem_read_8b(PC); ld_rr_nn(H, L, nn); }
opcode[0x08] = function() { mem_write_16b(PC, SP); }
opcode[0xf5] = push_nn(A, F);
opcode[0xc5] = push_nn(B, C);
opcode[0xd5] = push_nn(D, E);
opcode[0xe5] = push_nn(H, L);
opcode[0xf1] = pop_nn(A, F);
opcode[0xc1] = pop_nn(B, C);
opcode[0xd1] = pop_nn(D, E);
opcode[0xe1] = pop_nn(H, L);
opcode[0x87] = add_r_n(A, A, false, REGISTER_TO_REGISTER);
opcode[0x80] = add_r_n(A, B, false, REGISTER_TO_REGISTER);
opcode[0x81] = add_r_n(A, C, false, REGISTER_TO_REGISTER);
opcode[0x82] = add_r_n(A, D, false, REGISTER_TO_REGISTER);
opcode[0x83] = add_r_n(A, E, false, REGISTER_TO_REGISTER);
opcode[0x84] = add_r_n(A, H, false, REGISTER_TO_REGISTER);
opcode[0x85] = add_r_n(A, L, false, REGISTER_TO_REGISTER);
opcode[0x86] = add_r_n(A, read_16b_reg(H, L), false, IMMEDIATE_TO_REGISTER);
opcode[0xc6] = add_r_n(A, PC, false, IMMEDIATE_TO_REGISTER);
opcode[0x8f] = add_r_n(A, A, true, REGISTER_TO_REGISTER);
opcode[0x88] = add_r_n(A, B, true, REGISTER_TO_REGISTER);
opcode[0x89] = add_r_n(A, C, true, REGISTER_TO_REGISTER);
opcode[0x8a] = add_r_n(A, D, true, REGISTER_TO_REGISTER);
opcode[0x8b] = add_r_n(A, E, true, REGISTER_TO_REGISTER);
opcode[0x8c] = add_r_n(A, H, true, REGISTER_TO_REGISTER);
opcode[0x8d] = add_r_n(A, L, true, REGISTER_TO_REGISTER);
opcode[0x8e] = add_r_n(A, read_16b_reg(H, L), true, IMMEDIATE_TO_REGISTER);
opcode[0xce] = add_r_n(A, PC, true, IMMEDIATE_TO_REGISTER);
opcode[0x97] = sub_r_n(A, A, false, REGISTER_TO_REGISTER);
opcode[0x90] = sub_r_n(A, B, false, REGISTER_TO_REGISTER);
opcode[0x91] = sub_r_n(A, C, false, REGISTER_TO_REGISTER);
opcode[0x92] = sub_r_n(A, D, false, REGISTER_TO_REGISTER);
opcode[0x93] = sub_r_n(A, E, false, REGISTER_TO_REGISTER);
opcode[0x94] = sub_r_n(A, H, false, REGISTER_TO_REGISTER);
opcode[0x95] = sub_r_n(A, L, false, REGISTER_TO_REGISTER);
opcode[0x96] = sub_r_n(A, read_16b_reg(H, L), false, IMMEDIATE_TO_REGISTER);
opcode[0xd6] = sub_r_n(A, PC, false, IMMEDIATE_TO_REGISTER);
opcode[0x9f] = sub_r_n(A, A, true, REGISTER_TO_REGISTER);
opcode[0x98] = sub_r_n(A, B, true, REGISTER_TO_REGISTER);
opcode[0x99] = sub_r_n(A, C, true, REGISTER_TO_REGISTER);
opcode[0x9a] = sub_r_n(A, D, true, REGISTER_TO_REGISTER);
opcode[0x9b] = sub_r_n(A, E, true, REGISTER_TO_REGISTER);
opcode[0x9c] = sub_r_n(A, H, true, REGISTER_TO_REGISTER);
opcode[0x9d] = sub_r_n(A, L, true, REGISTER_TO_REGISTER);
opcode[0x9e] = sub_r_n(A, read_16b_reg(H, L), true, IMMEDIATE_TO_REGISTER);
opcode[0xa7] = and_r_n(A, A, REGISTER_TO_REGISTER);
opcode[0xa0] = and_r_n(A, B, REGISTER_TO_REGISTER);
opcode[0xa1] = and_r_n(A, C, REGISTER_TO_REGISTER);
opcode[0xa2] = and_r_n(A, D, REGISTER_TO_REGISTER);
opcode[0xa3] = and_r_n(A, E, REGISTER_TO_REGISTER);
opcode[0xa4] = and_r_n(A, H, REGISTER_TO_REGISTER);
opcode[0xa5] = and_r_n(A, L, REGISTER_TO_REGISTER);
opcode[0xa6] = and_r_n(A, read_16b_reg(H, L), IMMEDIATE_TO_REGISTER);
opcode[0xe6] = and_r_n(A, PC, IMMEDIATE_TO_REGISTER);
opcode[0xb7] = or_r_n(A, A, REGISTER_TO_REGISTER);
opcode[0xb0] = or_r_n(A, B, REGISTER_TO_REGISTER);
opcode[0xb1] = or_r_n(A, C, REGISTER_TO_REGISTER);
opcode[0xb2] = or_r_n(A, D, REGISTER_TO_REGISTER);
opcode[0xb3] = or_r_n(A, E, REGISTER_TO_REGISTER);
opcode[0xb4] = or_r_n(A, H, REGISTER_TO_REGISTER);
opcode[0xb5] = or_r_n(A, L, REGISTER_TO_REGISTER);
opcode[0xb6] = or_r_n(A, read_16b_reg(H, L), IMMEDIATE_TO_REGISTER);
opcode[0xf6] = or_r_n(A, PC, IMMEDIATE_TO_REGISTER);
opcode[0xaf] = xor_r_n(A, A, REGISTER_TO_REGISTER);
opcode[0xa8] = xor_r_n(A, B, REGISTER_TO_REGISTER);
opcode[0xa9] = xor_r_n(A, C, REGISTER_TO_REGISTER);
opcode[0xaa] = xor_r_n(A, D, REGISTER_TO_REGISTER);
opcode[0xab] = xor_r_n(A, E, REGISTER_TO_REGISTER);
opcode[0xac] = xor_r_n(A, H, REGISTER_TO_REGISTER);
opcode[0xad] = xor_r_n(A, L, REGISTER_TO_REGISTER);
opcode[0xae] = xor_r_n(A, read_16b_reg(H, L), IMMEDIATE_TO_REGISTER);
opcode[0xee] = xor_r_n(A, PC, IMMEDIATE_TO_REGISTER);
opcode[0xbf] = cp_r_n(A, A, REGISTER_TO_REGISTER);
opcode[0xb8] = cp_r_n(A, B, REGISTER_TO_REGISTER);
opcode[0xb9] = cp_r_n(A, C, REGISTER_TO_REGISTER);
opcode[0xba] = cp_r_n(A, D, REGISTER_TO_REGISTER);
opcode[0xbb] = cp_r_n(A, E, REGISTER_TO_REGISTER);
opcode[0xbc] = cp_r_n(A, H, REGISTER_TO_REGISTER);
opcode[0xbd] = cp_r_n(A, L, REGISTER_TO_REGISTER);
opcode[0xbe] = cp_r_n(A, read_16b_reg(H, L), IMMEDIATE_TO_REGISTER);
opcode[0xfe] = cp_r_n(A, PC, IMMEDIATE_TO_REGISTER);
opcode[0x3c] = inc_n(A, REGISTER);
opcode[0x04] = inc_n(B, REGISTER);
opcode[0x0c] = inc_n(C, REGISTER);
opcode[0x14] = inc_n(D, REGISTER);
opcode[0x1c] = inc_n(E, REGISTER);
opcode[0x24] = inc_n(H, REGISTER);
opcode[0x2c] = inc_n(L, REGISTER);
opcode[0x34] = inc_n(read_16b_reg(H, L), IMMEDIATE);
opcode[0x3d] = dec_n(A, REGISTER);
opcode[0x05] = dec_n(B, REGISTER);
opcode[0x0d] = dec_n(C, REGISTER);
opcode[0x15] = dec_n(D, REGISTER);
opcode[0x1d] = dec_n(E, REGISTER);
opcode[0x25] = dec_n(H, REGISTER);
opcode[0x2d] = dec_n(L, REGISTER);
opcode[0x35] = dec_n(read_16b_reg(H, L), IMMEDIATE);
opcode[0x09] = add_rr_n(H, L, B, C, REGISTER_TO_REGISTER);
opcode[0x19] = add_rr_n(H, L, D, E, REGISTER_TO_REGISTER);
opcode[0x29] = add_rr_n(H, L, H, L, REGISTER_TO_REGISTER);
opcode[0x39] = function() {
	let ll = REG[L] + (SP & 0xff);
	let hh = REG[H] + ((SP >> 8) & 0xff) + (ll > 0xff ? 1 : 0);
	
	REG[L] = ll & 0xff;
	REG[H] = hh & 0xff;
 };
 opcode[0xe8] = function() { SP += mem_read_8b(PC); };
 opcode[0x03] = inc_nn(B, C);
 opcode[0x13] = inc_nn(D, E);
 opcode[0x23] = inc_nn(H, L);
 opcode[0x33] = function() { SP += 1; }

function step() {
	let ins = mem[PC];
	opcode[ins]();
	PC++;
}
