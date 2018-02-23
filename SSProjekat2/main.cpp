#include <iostream>
#include <iomanip>
#include <fstream>
#include "instructions.h"
#include "tables.h"
#include "util.h"
#include "expressions.h"

extern std::unordered_map<std::string, instruct*> instructions;
extern std::vector<sym_table_row*> sym_table;

/// output
void output_obj(char* file) {

	std::ofstream out_file(file);

	out_file << "#TabelaSimbola" << std::endl;
	for (int it = 0; it < sym_table.size(); ++it) {
		if (sym_table[it]->type == "SYM") {
			// print row
			out_file <<
				sym_table[it]->type << " " <<
				std::dec << sym_table[it]->num << " " <<
				sym_table[it]->name << " " <<
				std::dec << sym_table[it]->seg_num << " " <<
				"0x" << std::hex << sym_table[it]->addr_val << " " <<
				sym_table[it]->flag << std::endl;
		} else if (sym_table[it]->type == "SEG") {
			// print row
			out_file <<
				sym_table[it]->type << " " <<
				std::dec << sym_table[it]->num << " " <<
				sym_table[it]->name << " " <<
				std::dec << sym_table[it]->seg_num << " " <<
				"0x" << std::hex << sym_table[it]->addr_val << " " <<
				"0x" << std::hex << sym_table[it]->seg_size << " " <<
				sym_table[it]->flag << std::endl;
		}
	}

	for (int it = 0; it < sym_table.size(); ++it) {
		if (sym_table[it]->type == "SEG") { // dont print .bss
			if (sym_table[it]->reloc_table.size() > 0) {
				out_file << "#rel" << sym_table[it]->name << std::endl;

				for (int it2 = 0; it2 < sym_table[it]->reloc_table.size(); ++it2) {
					out_file << "0x" << std::hex << sym_table[it]->reloc_table[it2]->addr << " " << sym_table[it]->reloc_table[it2]->type << " " << std::dec << sym_table[it]->reloc_table[it2]->num << std::endl;
				}
			}

			out_file << sym_table[it]->name << std::endl;

			for (int it2 = 0; it2 < sym_table[it]->machine_code.size(); ++it2) { // check
				if (it2 % 16 == 0 && it2 != 0) {
					out_file << std::endl;
				}

				int first = (sym_table[it]->machine_code[it2] & 0xF0) >> 4;
				int second = sym_table[it]->machine_code[it2] & 0xF;
				out_file << std::hex << first << std::hex << second << " ";
			}

			out_file << std::endl;
		}
	}
	out_file << "#end" << std::endl;
}

/// main
int main(int argc, char** argv) {

	init_instruct_map();

	first_code_iteration(argv[1]);
	second_code_iteration(argv[1]);

	output_obj(argv[2]);

	return 0;
}
