#include "expressions.h"

extern std::unordered_map<std::string, sym_table_row*> sym_table_unord;
extern bool comma_flag;
extern bool dup_flag;

/// return ipriority of operator
int ipr(std::string op) {

	if (op == "+" || op == "-") {
		return 2;
	}
	if (op == "*" || op == "/") {
		return 3;
	}
	if (op == "(") {
		return 6;
	}
	if (op == ")") {
		return 1;
	}

	return -1;
}

/// return spriority of operation
int spr(std::string op) {

	if (op == "+" || op == "-") {
		return 2;
	}
	if (op == "*" || op == "/") {
		return 3;
	}
	if (op == "(") {
		return 0;
	}

	return -1;
}

/// swtitch from infix to prefix
std::vector<std::string> in_to_post(std::vector<std::string> &inf) {

	std::stack<std::string> st;
	std::vector<std::string> postf;
	for (int it = 0; it < inf.size(); ++it) {
		std::string next = inf[it];

		// operator
		if (next == "+" || next == "-" ||
			next == "*" || next == "/" ||
			next == ")" || next == "(") {
			while (!st.empty() && ipr(next) <= spr(st.top())) {
				std::string temp = st.top();
				st.pop();
				postf.push_back(temp);
			}

			if (next != ")") {
				st.push(next);
			} else {
				st.pop();
			}
		} else { // operand
			postf.push_back(next);
		}
	}

	while (!st.empty()) {
		std::string temp = st.top();
		st.pop();
		if (temp != "(" && temp != ")") {
			postf.push_back(temp);
		}
	}

	return postf;
}

/// get synbol value
int get_string_val(std::string &op) {

	if (sym_table_unord.find(op) != sym_table_unord.end()) {
		// !!! check if all 'G' are included, or only imported ones
		if (sym_table_unord[op]->flag == 'G') {
			return 0;
		}
		return sym_table_unord[op]->addr_val;
	}

	if (op.length() >= 2) {
		if (op[0] == '0' && op[1] == 'b') {
			op.erase(0, 2);
			
			const char* temp = op.c_str();
			return strtol(temp, NULL, 2);
		}
	}

	const char* temp = op.c_str();
	return strtol(temp, NULL, 0);
}

/// evaluate expression
int eval_postf(std::vector<std::string> &postf, int location_count) {

	if (postf.size() == 0) {
		return 0;
	}

	std::stack<int> st;
	for (int it = 0; it < postf.size(); ++it) {
		std::string next = postf[it];

		if (next == "+" || next == "-" ||
			next == "*" || next == "/") {
			int first_op = st.top();
			st.pop();

			int second_op = st.top();
			st.pop();

			if (next == "+") {
				st.push(first_op + second_op);
			} else if (next == "-") {
				st.push(second_op - first_op);
			} else if (next == "*") {
				st.push(first_op * second_op);
			} else if (next == "/") {
				st.push(second_op / first_op);
			}
		} else {
			if (next == "$") { // dont need this !!!!!!????????????????????????????
				st.push(location_count);
			} else {
				st.push(get_string_val(next));
			}
		}
	}

	return st.top();
}

/// convert string to word vector
std::vector<std::string> line_to_vec(std::ifstream &in_file) {

	std::vector<std::string> infix;
	std::string word;
	bool break_flag = false;

	while (!break_flag) {
		word.clear();
		std::streampos oldpos = in_file.tellg();

		while (1) {
			if (in_file.peek() == ' ') {
				char c;
				in_file.get(c);
			}
			else {
				break;
			}
		}

		while (1) {
			if (in_file.peek() == '\n' || in_file.peek() == '\t' || in_file.peek() == '\r') { // check !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				break_flag = true;
				break;
			}
			if (in_file.peek() == ' ') {
				break;
			}

			char c;
			in_file.get(c);
			word.push_back(c);
		}

		if (word.length() == 0) {
			break;
		}

		if (word[word.length() - 1] == ',') { // reg ind off, and no variable can have ']' as last char || DB, DW, DD
			word.pop_back();
			comma_flag = true;
			break_flag = true;
		}

		if (word[0] == '#' || word[0] == '$') {
			word.erase(word.begin());
		}

		if (word.length() == 0) {
			break;
		}

		if (word[0] == '\'' && word[word.size() - 1] == '\'') {
			if (word.length() == 3) {
				word = std::to_string((int)word[1]);
			}
			else {
				err_exit();
			}
		}

		if (word[0] == ';') {
			in_file.seekg(oldpos);
			break;
		}

		if (word[word.length() - 1] == ']') { // reg ind off, and no variable can have ']' as last char
			word.pop_back();
			break_flag = true;
		}

		if (word == "DUP") {
			dup_flag = true;
			break;
		}

		if (word == "?") { // random value
			infix.push_back("0");
			break;
		}

		if (word.length() > 0) {
			if (sym_table_unord.find(word) != sym_table_unord.end()) {
				// check // should be right // if def we use def which is defed with label, if that label is not in org, we add that label,
				// and that lable will be latter rellocated, if its in the org section, thenwe will add and subtract that labels value
				if (sym_table_unord[word]->seg_num == -1 && sym_table_unord[word]->label != "") {
					infix.push_back("(");

					infix.push_back(sym_table_unord[word]->label);
					infix.push_back("+");
					std::string to_push = std::to_string(sym_table_unord[word]->addr_val - sym_table_unord[sym_table_unord[word]->label]->addr_val);
					infix.push_back(to_push);

					infix.push_back(")");
					continue;
				}
			}
			infix.push_back(word);
		}

		if (break_flag) {
			break;
		}
	}

	return infix;
}
