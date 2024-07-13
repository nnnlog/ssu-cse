#include <bits/stdc++.h>

using namespace std;

enum Types {
  OPERATOR_COMP, // == != < > <= >=
  OPERATOR_PLUS_MINUS, // + -
  OPERATOR_MUL_DIV, // * /

  ASSIGN, // =

  SEMICOLON, // ;
  DECIMAL, // 수
  VARIABLE, // x y z

  PRINT, // print

  EOF_, // end of file
  UNKNOWN, // error
};

struct bool_int {
  int is_bool;
  union {
	int i;
	bool b;
  } data;

};

string to_string(const bool_int &a) {
	return a.is_bool ? (a.data.b ? "TRUE" : "FALSE") : to_string(a.data.i);
}

struct token_info {
  Types type;
  string value;
};

struct input {
  string buf;
  int idx;

  vector<int> restore_points;

  void create_restore_point() {
	  restore_points.push_back(idx);
  }

  void remove_restore_point() {
	  restore_points.pop_back();
  }

  void restore() {
	  idx = restore_points.back();
	  remove_restore_point();
  }

  bool eof() const {
	  return buf.size() == idx;
  }

  static bool is_numeric(const string &s) {
	  for (char c: s) if (!isdigit(c)) return false;
	  return true;
  }

  void skip_blanks() {
	  while (!eof() && buf[idx] == ' ') idx++;
  }

  string get_token_string() {
	  string ret;
	  while (!eof() && buf[idx] != ' ') {
		  ret.push_back(buf[idx++]);
	  }
	  skip_blanks();
	  return ret;
  }

  token_info get_token() {
	  if (eof()) return {EOF_, ""};
	  string token = get_token_string();
	  token_info ret = {UNKNOWN, token};
	  if (token == "==" || token == "!=" || token == "<" || token == ">" || token == "<=" || token == ">=") ret.type = OPERATOR_COMP;
	  else if (token == "=") ret.type = ASSIGN;
	  else if (token == ";") ret.type = SEMICOLON;
	  else if (is_numeric(token)) ret.type = DECIMAL;
	  else if (token == "print") ret.type = PRINT;
	  else if (token == "+" || token == "-") ret.type = OPERATOR_PLUS_MINUS;
	  else if (token == "*" || token == "/") ret.type = OPERATOR_MUL_DIV;
	  else if (token == "x" || token == "y" || token == "z") ret.type = VARIABLE;
	  return ret;
  }
};

typedef pair<bool, string> result;

struct parser {
  input _input;
  map<string, bool_int> variable_data;

  result program() {
	  result res = {true, ""};
	  while (!_input.eof()) {
		  auto a = statement();
		  if (!a.first) return {false, ""};
		  if (!a.second.empty()) res.second += a.second + " ";
	  }
	  return res;
  }

  result statement() {
	  auto curr = _input.get_token();
	  if (curr.type == VARIABLE) {
		  auto a = _input.get_token();
		  auto b = expr();
		  auto c = _input.get_token();

		  if (a.type != ASSIGN || !b.second.first) return {false, ""};
		  if (c.type != SEMICOLON) return {false, ""};

		  variable_data[curr.value] = b.first;
		  return {true, ""};
	  } else if (curr.type == PRINT) {
		  auto a = var();
		  auto b = _input.get_token();

		  if (!a.second.first) return {false, ""};
		  if (b.type != SEMICOLON) return {false, ""};

		  return {true, to_string(variable_data[a.first])};
	  } else {
		  return {false, ""};
	  }
  }

  pair<bool_int, result> expr() {
	  _input.create_restore_point();

	  auto res = bexpr();
	  if (!res.second.first) {
		  _input.restore();
		  auto res = aexpr();

		  return {
			  bool_int {
				  false,
				  res.first
			  },
			  {true, ""}
		  };
	  } else {
		  _input.remove_restore_point();
		  return {
			  bool_int {
				  	true,
					  res.first
				  },
			  {true, ""}
		  };
	  }
  }

  pair<bool, result> bexpr() {
	  token_info a, b, c;
	  a = _input.get_token();
	  b = _input.get_token();
	  c = _input.get_token();
	  if (a.type != DECIMAL || b.type != OPERATOR_COMP || c.type != DECIMAL) return {false, {false, ""}};
	  pair<bool, result> ret = {true, {true, ""}};
	  auto &v = ret.first;
	  int x = atoi(a.value.c_str()), y = atoi(b.value.c_str());
	  if (b.value == "==") v = x == y;
	  if (b.value == "!=") v = x != y;
	  if (b.value == "<") v = x < y;
	  if (b.value == ">") v = x > y;
	  if (b.value == "<=") v = x <= y;
	  if (b.value == ">=") v = x >= y;
	  return ret;
  }

  pair<int, result> aexpr() {
	  auto a = term();

	  if (!a.second.first) return {0, {false, ""}};

	  while (1) {
		  _input.create_restore_point();

		  int fail = 0;
		  auto op = _input.get_token();
		  if (op.type == OPERATOR_PLUS_MINUS) {
			  auto res = term();
			  if (!res.second.first) fail = true, a.second.first = false;
			  else {
				  if (op.value == "+") a.first += res.first;
				  else a.first -= res.first;
			  }
		  } else {
			  fail = true;
		  }

		  if (fail) {
			  _input.restore();
			  break;
		  } else {
			  _input.remove_restore_point();
		  }
	  }

	  return a;
  }

  pair<int, result> term() {
	  auto a = factor();

	  if (!a.second.first) return {0, {false, ""}};

	  while (1) {
		  _input.create_restore_point();
		  int fail = 0;

		  auto op = _input.get_token();
		  if (op.type == OPERATOR_MUL_DIV) {
			  auto res = factor();
			  if (!res.second.first) fail = true, a.second.first = false;
			  else {
				  if (op.value == "*") a.first *= res.first;
				  else a.first /= res.first;
			  }
		  } else {
			  fail = true;
		  }

		  if (fail) {
			  _input.restore();
			  break;
		  } else {
			  _input.remove_restore_point();
		  }
	  }

	  return a;
  }

  pair<int, result> factor() {
	  return number();
  }

  pair<int, result> number() {
	  auto a = _input.get_token();
	  if (a.type != DECIMAL) return {0, {false, ""}};
	  return {atoi(a.value.c_str()), {true, ""}};
  }

  pair<string, result> var() {
	  auto a = _input.get_token();
	  if (a.type != VARIABLE) return {"", {false, ""}};
	  return {a.value, {true, ""}};
  }
};

int main() {
	while (1) {
		cout << ">> ";

		string s;
		getline(cin, s);

		if (s == "terminate") break;

		parser parser = {
			{
				s,
				0,
				{}
			},
			{}
		};
		parser._input.skip_blanks();

		auto res = parser.program();
		if (!res.first) cout << "syntax error!!";
		else cout << res.second;

		cout << "\n";
	}
}
