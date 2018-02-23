#pragma once
#include "expressions.h"
#include "instructions.h"
#include "util.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <unordered_map>

/// one row of relocation table
struct reloc_table_row {

	// fields
	unsigned int addr;
	char type;
	int num;

	// constructor
	reloc_table_row(unsigned int addr, char type, int num) :
		addr(addr), type(type), num(num) {
	}
};

/// one row of symbol table struct
struct sym_table_row {

	// fields
	std::string type;
	int num;
	std::string name;
	int seg_num;
	int addr_val;
	char flag;

	// segment fields
	int seg_size;
	std::vector<unsigned char> machine_code;
	std::vector<reloc_table_row*> reloc_table;

	// DEF fields
	std::string label;

	// constructor
	sym_table_row(std::string type, int num, std::string name, int seg_num, int addr_val, char flag) :
		type(type), num(num), name(name), seg_num(seg_num), addr_val(addr_val), flag(flag) {
	}
};

/// add global symbols in symbol table
void add_globals(std::ifstream &in_file);

/// update relocation table
std::string update_reloc_table(std::vector<std::string> &postfix, int location_count, int curr_seg_num, char type = 'A');

/// first iteration of assembly code
void first_code_iteration(char* file);

/// second iteration of assembly code
void second_code_iteration(char* file);
