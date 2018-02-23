#include "instructions.h"

std::unordered_map<std::string, instruct*> instructions;

extern std::unordered_map<std::string, sym_table_row*> sym_table_unord;
extern std::vector<sym_table_row*> sym_table;

/// initialise map of instructions
void init_instruct_map(void) {

	// stream control
	instructions["INT"] = new instruct(1, 0x00 << 24, 1);
	instructions["RET"] = new instruct(0, 0x01 << 24);
	instructions["JMP"] = new instruct(1, 0x02 << 24, 1);
	instructions["CALL"] = new instruct(1, 0x03 << 24, 1);
	instructions["JZ"] = new instruct(2, 0x04 << 24, 1);
	instructions["JNZ"] = new instruct(2, 0x05 << 24, 1);
	instructions["JGZ"] = new instruct(2, 0x06 << 24, 1);
	instructions["JGEZ"] = new instruct(2, 0x07 << 24, 1);
	instructions["JLZ"] = new instruct(2, 0x08 << 24, 1);
	instructions["JLEZ"] = new instruct(2, 0x09 << 24, 1);

	// load
	instructions["LOADUB"] = new instruct(2, 0x10 << 24 | 0x03 << 3, 1);
	instructions["LOADSB"] = new instruct(2, 0x10 << 24 | 0x07 << 3, 1);
	instructions["LOADUW"] = new instruct(2, 0x10 << 24 | 0x01 << 3, 1);
	instructions["LOADSW"] = new instruct(2, 0x10 << 24 | 0x05 << 3, 1);
	instructions["LOAD"] = new instruct(2, 0x10 << 24 | 0x00 << 3, 1);

	// store
	instructions["STOREB"] = new instruct(2, 0x11 << 24 | 0x03 << 3, 1);
	instructions["STOREW"] = new instruct(2, 0x11 << 24 | 0x01 << 3, 1);
	instructions["STORE"] = new instruct(2, 0x11 << 24 | 0x00 << 3, 1);

	// stack
	instructions["PUSH"] = new instruct(1, 0x20 << 24);
	instructions["POP"] = new instruct(1, 0x21 << 24);

	// arithmetic and logic
	instructions["ADD"] = new instruct(3, 0x30 << 24);
	instructions["SUB"] = new instruct(3, 0x31 << 24);
	instructions["MUL"] = new instruct(3, 0x32 << 24);
	instructions["DIV"] = new instruct(3, 0x33 << 24);
	instructions["MOD"] = new instruct(3, 0x34 << 24);
	instructions["AND"] = new instruct(3, 0x35 << 24);
	instructions["OR"] = new instruct(3, 0x36 << 24);
	instructions["XOR"] = new instruct(3, 0x37 << 24);
	instructions["NOT"] = new instruct(2, 0x38 << 24);
	instructions["ASL"] = new instruct(3, 0x39 << 24);
	instructions["ASR"] = new instruct(3, 0x3A << 24);
}

/// get size of instruction "word"
unsigned int get_instruct_size(std::ifstream &in_file, std::string &word) {

	unsigned int instruct_size = 4;
	instruct* curr_instruct = instructions[word];

	if (curr_instruct->addr_type > 0) {
		for (int it = 0; it < curr_instruct->operand_num; ++it) {
			in_file >> word;
		}

		if (!(word[0] == '[' && word[word.size() - 1] == ']')
			&& !(word[0] == 'R' || word.compare(0, 2, "SP") == 0 || word.compare(0, 2, "PC") == 0)) {
			instruct_size = 8;
		}
		if (in_file.peek() != '\n') {
			std::string skip_line;
			std::getline(in_file, skip_line);
		}
	} else {
		std::string skip_line;
		std::getline(in_file, skip_line);
	}

	return instruct_size;
}

/// calculate intruction code
int cal_instruct_code(std::ifstream &in_file, std::string &word, std::string &curr_seg, int location_count, int curr_seg_num, bool bss_flag) {

	instruct* instr = instructions[word];
	unsigned int instr_code = instr->op_type_mask;
	unsigned int reg_num = instr->operand_num - instr->addr_type;
	for (int it = 0; it < reg_num; ++it) {
		in_file >> word;

		// erase 'R' from register word
		word.erase(word.begin());

		const char* temp = word.c_str();
		long conv = strtol(temp, NULL, 0);
		unsigned int reg_mask = conv << (16 - it * 5);

		instr_code |= reg_mask;
	}

	bool second_word_flag = false;
	int second_word;
	if (instr->addr_type > 0) {
		std::streampos oldpos = in_file.tellg();
		in_file >> word;

		unsigned int addr_mask;
		if (word[0] == '#') { // immed
			addr_mask = 4;
			in_file.seekg(oldpos);
			std::vector<std::string> vect = line_to_vec(in_file);
			std::vector<std::string> postfix = in_to_post(vect);
			update_reloc_table(postfix, location_count + 4, curr_seg_num);
			second_word = eval_postf(postfix, location_count);
			second_word_flag = true;
		} else if (word[0] == 'R') { // reg dir
			addr_mask = 0;
			// push reg
			word.erase(word.begin());
			const char* temp = word.c_str();
			long conv = strtol(temp, NULL, 0);
			unsigned int reg_mask = conv << (16 - (instr->operand_num - 1) * 5);
			instr_code |= reg_mask;
		} else if (word.compare(0, 2, "PC") == 0) { // reg dir
			addr_mask = 0;
			// push reg
			unsigned int reg_mask = 0x11 << (16 - (instr->operand_num - 1) * 5);
			instr_code |= reg_mask;
		} else if (word.compare(0, 2, "SP") == 0) { // reg dir
			addr_mask = 0;
			// push reg
			unsigned int reg_mask = 0x10 << (16 - (instr->operand_num - 1) * 5);
			instr_code |= reg_mask;
		}
		else if (word.compare(0, 2, "[R") == 0 && word[word.size() - 1] == ']') { // reg ind
			addr_mask = 2;
			// push reg
			word.erase(word.begin());
			word.erase(word.begin());
			const char* temp = word.c_str();
			long conv = strtol(temp, NULL, 0);
			unsigned int reg_mask = conv << (16 - (instr->operand_num - 1) * 5);
			instr_code |= reg_mask;
		} else if (word.compare(0, 3, "[PC") == 0 && word[word.size() - 1] == ']') { // reg ind
			addr_mask = 2;
			// push reg
			unsigned int reg_mask = 0x11 << (16 - (instr->operand_num - 1) * 5);
			instr_code |= reg_mask;
		} else if (word.compare(0, 3, "[SP") == 0 && word[word.size() - 1] == ']') { // reg ind
			addr_mask = 2;
			// push reg
			unsigned int reg_mask = 0x10 << (16 - (instr->operand_num - 1) * 5);
			instr_code |= reg_mask;
		} else if (word.compare(0, 2, "[R") == 0) { // reg ind off ///////////////// one example with this ///////////////////
			addr_mask = 7;
			// push reg
			word.erase(word.begin());
			word.erase(word.begin());
			const char* temp = word.c_str();
			long conv = strtol(temp, NULL, 0);
			unsigned int reg_mask = conv << (16 - (instr->operand_num - 1) * 5);
			instr_code |= reg_mask;
			// skip '+' char
			in_file >> word;
			std::vector<std::string> vect = line_to_vec(in_file);
			std::vector<std::string> postfix = in_to_post(vect);
			update_reloc_table(postfix, location_count + 4, curr_seg_num);
			second_word = eval_postf(postfix, location_count);
			second_word_flag = true;
		} else if (word.compare(0, 3, "[PC") == 0) { // reg ind off ///////////////// one example with this ///////////////////
			addr_mask = 7;
			// push reg
			unsigned int reg_mask = 0x11 << (16 - (instr->operand_num - 1) * 5);
			instr_code |= reg_mask;
			// skip '+' char
			in_file >> word;
			std::vector<std::string> vect = line_to_vec(in_file);
			std::vector<std::string> postfix = in_to_post(vect);
			update_reloc_table(postfix, location_count + 4, curr_seg_num); // ????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
			second_word = eval_postf(postfix, location_count);
			second_word_flag = true;
		} else if (word.compare(0, 3, "[SP") == 0) { // reg ind off
			addr_mask = 7;
			// push reg
			unsigned int reg_mask = 0x10 << (16 - (instr->operand_num - 1) * 5);
			instr_code |= reg_mask;
			// skip '+' char
			in_file >> word;
			std::vector<std::string> vect = line_to_vec(in_file);
			std::vector<std::string> postfix = in_to_post(vect);
			update_reloc_table(postfix, location_count + 4, curr_seg_num);
			second_word = eval_postf(postfix, location_count);
			second_word_flag = true;
		} else if (word[0] == '$') { // pc rel off ///////////////// one example with this ///////////////////
			addr_mask = 7;
			// push reg
			unsigned int reg_mask = 0x11 << (16 - (instr->operand_num - 1) * 5);
			instr_code |= reg_mask;
			in_file.seekg(oldpos);
			std::vector<std::string> vect = line_to_vec(in_file);
			std::vector<std::string> postfix = in_to_post(vect);
			update_reloc_table(postfix, location_count + 4, curr_seg_num, 'R'); // add curr_seg field, 'R' to relloc table !!!!!!!!!!!!!!!! ??????????????????????????????????????????????????????????????????
			second_word = eval_postf(postfix, location_count) - (location_count + 4 - sym_table[curr_seg_num]->addr_val); // ????????????????????????????? does relloc has to put something here ?!!!!!!!!!!??????????????? ////////////////////////////////////
			second_word_flag = true;
		} else { // mem dir
			addr_mask = 6;
			in_file.seekg(oldpos);
			std::vector<std::string> vect = line_to_vec(in_file);
			std::vector<std::string> postfix = in_to_post(vect);
			update_reloc_table(postfix, location_count + 4, curr_seg_num);
			second_word = eval_postf(postfix, location_count);
			second_word_flag = true;
		}

		addr_mask <<= 21;
		instr_code |= addr_mask;
	}

	if (!bss_flag) {
		for (int it = 3; it >= 0; --it) { // add instruction code of current intruction
			// check if it actually cuts the rest
			unsigned char temp = instr_code >> (8 * it);
			sym_table_unord[curr_seg]->machine_code.push_back(temp);
		}

		if (second_word_flag) { // if instruction is 64b long, add second word, little endian !!!!!!!!!!!!
			for (int it = 0; it < 4; ++it) {
				// check if it actually cuts the rest
				unsigned char temp = second_word >> (8 * it);
				sym_table_unord[curr_seg]->machine_code.push_back(temp);
			}
		}
	}

	return second_word_flag ? 8 : 4;
}
