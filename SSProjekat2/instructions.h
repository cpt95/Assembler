#pragma once
#include "expressions.h"
#include "tables.h"
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <regex>
#include <vector>

/// istruction struct
struct instruct {

	// fields
	unsigned int operand_num;
	unsigned int op_type_mask;
	unsigned int addr_type;

	// constructor
	instruct(unsigned int operand_num, unsigned int op_type_mask, unsigned int addr_type = 0) :
		operand_num(operand_num), op_type_mask(op_type_mask), addr_type(addr_type) {}
};

/// initialise map of instructions
void init_instruct_map(void);

/// get size of instruction "word"
unsigned int get_instruct_size(std::ifstream &in_file, std::string &word);

/// calculate intruction code
int cal_instruct_code(std::ifstream &in_file, std::string &word, std::string &curr_seg, int location_count, int curr_seg_num, bool bss_flag);
