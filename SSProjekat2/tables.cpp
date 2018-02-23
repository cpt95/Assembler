#include "tables.h"

/// BSS DONT MAKE MACHINE CODE
std::vector<sym_table_row*> sym_table;
std::unordered_map<std::string, sym_table_row*> sym_table_unord;
bool comma_flag = false;
bool dup_flag = false;

extern std::unordered_map<std::string, instruct*> instructions;

/// add global symbols in symbol table
void add_globals(std::ifstream &in_file) {

	std::string line;
	std::string word;

	// error if there is only .global in one line // solved // check
	std::getline(in_file, line);
	std::stringstream s_stream(line);

	// takes .global from stream ///////////////// one example with this ///////////////////
	s_stream >> word; // check

	// check if it reads until the end of the input string
	while (s_stream >> word) {
		if (word[word.length() - 1] == ',') {
			word.pop_back();
		}
		// in case global variables are separated with ','
		if (word.length() > 0) {
			if (sym_table_unord.find(word) != sym_table_unord.end()) {
				sym_table_unord[word]->flag = 'G';
			} else {
				// check 0 0
				sym_table.push_back(new sym_table_row("SYM", sym_table.size() - 1, word, 0, 0, 'G'));
				sym_table_unord[word] = sym_table[sym_table.size() - 1];
			}
		}
	}
}

/// update relocation table
std::string update_reloc_table(std::vector<std::string> &postfix, int location_count, int curr_seg_num, char type) { // org section ?????????????? // 'D' try changing that

	int it = postfix.size() - 1;
	std::unordered_map<int, std::string> reloc_map;

	while (it >= 0) {
		int cnt = 0;

		if (postfix[it] == "*" || postfix[it] == "/") {
			cnt = 2;
		} else if (postfix[it] == "+" || postfix[it] == "-") {
			// skip
		} else {
			if (sym_table_unord.find(postfix[it]) != sym_table_unord.end()) {
				if (reloc_map.find(sym_table_unord[postfix[it]]->seg_num) != reloc_map.end()) {
					reloc_map.erase(sym_table_unord[postfix[it]]->seg_num);
				} else {
					reloc_map[sym_table_unord[postfix[it]]->seg_num] = postfix[it];
				}
			}
		}

		for (int i = 0; i < cnt; ++i) {
			if (postfix[it] == "+" || postfix[it] == "-" || postfix[it] == "*" || postfix[it] == "/") {
				++cnt;
			}
			--it;
		}
		--it;
	}

	if (type == 'D') {
		if (reloc_map.size() > 0) {
			return reloc_map.begin()->second;
		} else {
			return "";
		}
	}

	// check for 'R' relative rellocations !!!!!!!!!!!!!!!!!!!!!! ///////////////// one example with this ///////////////////
	if (reloc_map.size() > 0 && sym_table_unord[reloc_map.begin()->second]->seg_num != -1) { // ///////////////// one example with this ///////////////////
		int third_reloc_arg = sym_table_unord[reloc_map.begin()->second]->flag == 'G' ? sym_table_unord[reloc_map.begin()->second]->num : sym_table_unord[reloc_map.begin()->second]->seg_num;

		// u dont need to reloc if its relative and both symbol u have to relocate and the segment where u have to do relocate, u dont have to do relocation
		if (type == 'R' && (sym_table[sym_table_unord[reloc_map.begin()->second]->seg_num - 1]->addr_val == 0 || sym_table[curr_seg_num - 1] == 0)) { // ??????????????????????????
			sym_table[curr_seg_num - 1]->reloc_table.push_back(new reloc_table_row(location_count - sym_table[curr_seg_num]->addr_val, type, third_reloc_arg)); // check
			return "";
		}
		if (sym_table[sym_table_unord[reloc_map.begin()->second]->seg_num - 1]->addr_val == 0) {
			sym_table[curr_seg_num - 1]->reloc_table.push_back(new reloc_table_row(location_count - sym_table[curr_seg_num]->addr_val, type, third_reloc_arg)); // check
		}
	} else if (type == 'R') {
		sym_table[curr_seg_num - 1]->reloc_table.push_back(new reloc_table_row(location_count - sym_table[curr_seg_num]->addr_val, type, 0)); // check
		return "";
	}

	return "";
}

/// first iteration of assembly code
void first_code_iteration(char* file) {

	// open file
	std::ifstream in_file(file);

	// check if file is successfully opened
	if (!in_file.is_open()) {
		err_exit();
	}

	// define varables
	unsigned int sym_num = 1;
	int location_count = 0;
	bool org_flag = false;
	std::string word;
	std::string curr_seg = "";

	// first iteration loop
	while (!in_file.eof()) {
		std::streampos oldpos = in_file.tellg();
		in_file >> word;
		
		if (word == ".end") { // break after directive .end
			if (curr_seg != "") {
				sym_table_unord[curr_seg]->seg_size = location_count - sym_table_unord[curr_seg]->addr_val;
			}
			break;
		} else if (word == ".global") { // ignore directive .global
			std::string skip_line;
			std::getline(in_file, skip_line);
			continue;
		} else if (word[0] == '.') { // add section in symbol table
			if (org_flag) {
				org_flag = false;
			} else {
				if (curr_seg != "") {
					sym_table_unord[curr_seg]->seg_size = location_count - sym_table_unord[curr_seg]->addr_val;
				}
				location_count = 0;
			}

			sym_table.push_back(new sym_table_row("SEG", sym_num, word, sym_num, location_count, 'L'));
			sym_table_unord[word] = sym_table[sym_table.size() - 1];
			++sym_num;

			curr_seg = word;
		} else if (word[word.length() - 1] == ':') { // add label in symbol table
			word.pop_back();
			sym_table.push_back(new sym_table_row("SYM", sym_num, word, sym_table_unord[curr_seg]->seg_num, location_count, 'L'));
			sym_table_unord[word] = sym_table[sym_table.size() - 1];
			++sym_num;
		} else if (word[0] == ';') { // ignore comment
			// problem if there is nothing after ';' !!!
			in_file.seekg(oldpos);
			std::string skip_line;
			std::getline(in_file, skip_line);
		} else if (instructions.find(word) != instructions.end()) { // increment location counter according to instruction size
			location_count += get_instruct_size(in_file, word);
		} else if (word == "ORG") { // increment location counter with argument
			if (curr_seg != "") {
				sym_table_unord[curr_seg]->seg_size = location_count - sym_table_unord[curr_seg]->addr_val;
			}
			std::vector<std::string> vect = line_to_vec(in_file);
			std::vector<std::string> post = in_to_post(vect);
			location_count = eval_postf(post, location_count);
			org_flag = true;
		} else if (word == "DB" || word == "DW" || word == "DD") { // increment location counter according to directives below
			int size;

			if (word == "DB") {
				size = 1;
			} else if (word == "DW") {
				size = 2;
			} else if (word == "DD") {
				size = 4;
			}

			// this expression has to be calculateable in first iteration
			std::vector<std::string> vect = line_to_vec(in_file);
			std::vector<std::string> postf = in_to_post(vect);
			int first = eval_postf(postf, location_count);

			if (dup_flag) {
				std::string skip_line;
				std::getline(in_file, skip_line);

				dup_flag = false;

				size *= first;
			} else {
				int count = 1;

				while (comma_flag) {
					comma_flag = false;
					line_to_vec(in_file);
					++count;
				}

				size *= count;
			}
			location_count += size;
		} else { // DEF directive, add defined symbol in symbol table
			sym_table_row* def_row = new sym_table_row("SYM", sym_num, word, -1, 0, 'L');
			++sym_num;

			std::string def;
			in_file >> def;

			if (def != "DEF") {
				err_exit();
			}

			std::vector<std::string> vect = line_to_vec(in_file);
			std::vector<std::string> postf = in_to_post(vect);
			def_row->addr_val = eval_postf(postf, location_count);
			def_row->label = update_reloc_table(postf, location_count, -1, 'D'); // ?????????????????????????? // clarification in expressions.cpp

			sym_table.push_back(def_row);
			sym_table_unord[word] = sym_table[sym_table.size() - 1];
		}
	}

	// close file
	in_file.close();
}

/// second iteration of assembly code
void second_code_iteration(char* file) {

	// open file
	std::ifstream in_file(file);

	// check if file is successfully opened
	if (!in_file.is_open()) {
		err_exit();
	}

	// define varables
	int location_count = 0;
	bool org_flag = false;
	bool bss_flag = false;
	std::string word;
	std::string curr_seg;

	// second iteration loop
	while (!in_file.eof()) {
		std::streampos oldpos = in_file.tellg();
		in_file >> word;

		if (word == ".end") { // break after directive .end
			break;
		} else if (word == ".global") { // add global variables
			in_file.seekg(oldpos); // if the .global alone is in one line, it will work ///////////////// one example with this ///////////////////
			add_globals(in_file);
			continue;
		} else if (word[0] == '.') { // segment
			if (word.compare(0, 4, ".bss") == 0) {
				bss_flag = true;
			} else {
				bss_flag = false;
			}

			if (org_flag) {
				org_flag = false;
			} else {
				location_count = 0;
			}

			curr_seg = word;
		} else if (word[word.length() - 1] == ':') { // label
			continue;
		} else if (word[0] == ';') { // ignore comment
			std::string skip_line;
			std::getline(in_file, skip_line);
		} else if (instructions.find(word) != instructions.end()) { // calculate instruction code
			std::string instruction = word;
			location_count += cal_instruct_code(in_file, instruction, curr_seg, location_count, sym_table_unord[curr_seg]->seg_num, bss_flag);
		} else if (word == "ORG") { // ORG
			std::vector<std::string> vect = line_to_vec(in_file);
			std::vector<std::string> postfix = in_to_post(vect);
			location_count = eval_postf(postfix, location_count);
			org_flag = true;
		} else if (word == "DB" || word == "DW" || word == "DD") { // DB, DW, DD
			int size;

			if (word == "DB") {
				size = 1;
			} else if (word == "DW") {
				size = 2;
			} else if (word == "DD") {
				size = 4;
			}

			// this expression has to be calculateable in first iteration
			std::vector<std::string> vect = line_to_vec(in_file);
			std::vector<std::string> postfix = in_to_post(vect);
			update_reloc_table(postfix, location_count, sym_table_unord[curr_seg]->seg_num);
			int first = eval_postf(postfix, location_count);

			if (dup_flag) {
				std::vector<std::string> vect2 = line_to_vec(in_file);
				std::vector<std::string> postf = in_to_post(vect2);
				//update_reloc_table(postf, location_count, sym_table_unord[curr_seg]->seg_num);
				int second = eval_postf(postf, location_count);

				dup_flag = false;

				for (int it1 = 0; it1 < first; ++it1) {
					update_reloc_table(postf, location_count + it1 * size, sym_table_unord[curr_seg]->seg_num);
					// little endian, check for DB and DW
					for (int it2 = 0; it2 < size; ++it2) {
						unsigned char temp = second >> (8 * it2);
						sym_table_unord[curr_seg]->machine_code.push_back(temp);
					}
				}

				size *= first;
			} else {
				std::vector<int> args;
				args.push_back(first);

				while (comma_flag) {
					comma_flag = false;
					std::vector<std::string> vect2 = line_to_vec(in_file);
					std::vector<std::string> postf = in_to_post(vect2);
					update_reloc_table(postf, location_count, sym_table_unord[curr_seg]->seg_num);
					args.push_back(eval_postf(postf, location_count));
				}

				for (int it1 = 0; it1 < args.size(); ++it1) {
					for (int it2 = 0; it2 < size; ++it2) {
						unsigned char temp = args[it1] >> (8 * it2);
						sym_table_unord[curr_seg]->machine_code.push_back(temp);
					}
				}
				size *= args.size();
			}
			location_count += size;
		} else { // DEF directive
			std::string skip_line;
			std::getline(in_file, skip_line);
			continue;
		}
	}

	// close file
	in_file.close();
}
