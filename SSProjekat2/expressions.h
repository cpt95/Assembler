#pragma once
#include "tables.h"
#include <string>
#include <vector>
#include <stack>
#include <cstdlib>

struct sym_table_row;

/// return ipriority of operator
int ipr(std::string op);

/// return spriority of operation
int spr(std::string op);

/// swtitch from infix to prefix
std::vector<std::string> in_to_post(std::vector<std::string> &inf);

/// get symbol value
int get_string_val(std::string &op);

/// evaluate expression
int eval_postf(std::vector<std::string> &postf, int location_counter);

/// convert string to word vector
std::vector<std::string> line_to_vec(std::ifstream &in_file);
