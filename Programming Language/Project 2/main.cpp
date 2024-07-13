#include <bits/stdc++.h>

using namespace std;

/**
 * Reference: https://stackoverflow.com/questions/25114597/how-to-print-int128-in-g
 *
 * @param dest
 * @param value
 * @return
 */
std::ostream &
operator<<(std::ostream &dest, __int128_t value) {
	std::ostream::sentry s(dest);
	if (s) {
		__uint128_t tmp = value < 0 ? -value : value;
		char buffer[128];
		char *d = std::end(buffer);
		do {
			--d;
			*d = "0123456789"[tmp % 10];
			tmp /= 10;
		} while (tmp != 0);
		if (value < 0) {
			--d;
			*d = '-';
		}
		int len = std::end(buffer) - d;
		if (dest.rdbuf()->sputn(d, len) != len) {
			dest.setstate(std::ios_base::badbit);
		}
	}
	return dest;
}

enum Types {
	OPERATOR_COMP, // == != < > <= >=
	OPERATOR_ARITHMETIC, // + - * /

	ASSIGN, // =

	BRACKET_MID_OPEN, // {
	BRACKET_MID_CLOSE, // }
	BRACKET_SMALL_OPEN, // (
	BRACKET_SMALL_CLOSE, // )

	LOOP_DO, // do
	LOOP_WHILE, // while

	SEMICOLON, // ;

	DECIMAL, // 수

	STRING, // 문자열 (int 포함)

	PRINT, // print

	EOF_, // end of file
	UNKNOWN, // error
};

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

	static bool is_alphabetic(const string &s) {
		for (char c: s) if (!isalpha(c)) return false;
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
		else if (token == "{") ret.type = BRACKET_MID_OPEN;
		else if (token == "}") ret.type = BRACKET_MID_CLOSE;
		else if (token == "(") ret.type = BRACKET_SMALL_OPEN;
		else if (token == ")") ret.type = BRACKET_SMALL_CLOSE;
		else if (token == "do") ret.type = LOOP_DO;
		else if (token == "while") ret.type = LOOP_WHILE;
		else if (token == "=") ret.type = ASSIGN;
		else if (token == ";") ret.type = SEMICOLON;
		else if (is_numeric(token) && token.size() <= 10) ret.type = DECIMAL;
		else if (token == "print") ret.type = PRINT;
		else if (token == "+" || token == "-" || token == "*" || token == "/") ret.type = OPERATOR_ARITHMETIC;
		else if (is_alphabetic(token) && token.size() <= 10) ret.type = STRING;
		return ret;
	}
};

struct parser {
	input _input;
	map<string, __int128> variable_data;
//	stringstream output;

	bool check_grammar_only = true;

	void clear() {
//		output.clear();
		variable_data.clear();
	}

	bool program() {
		_input.create_restore_point();
		while (!_input.eof() && declaration());
		while (!_input.eof() && statement());
		if (_input.eof()) {
			_input.remove_restore_point();
			return true;
		}
		_input.restore();
		return false;
	}

	bool declaration() {
		_input.create_restore_point();
		auto a = _input.get_token();
		auto b = _input.get_token();
		auto c = _input.get_token();
		if (a.type == STRING && a.value == "int" && b.type == STRING && c.type == SEMICOLON) {
			_input.remove_restore_point();
			variable_data[b.value] = 0;
			return true;
		}
		_input.restore();
		return false;
	}

	bool statement() {
		_input.create_restore_point();
		auto a = _input.get_token();
		if (a.type == PRINT) {
			_input.create_restore_point();
			auto b = bexpr();
			auto c = _input.get_token();
			if (b.first && c.type == SEMICOLON) {
				_input.remove_restore_point();
				if (!check_grammar_only) cout << (b.second ? "TRUE" : "FALSE") << " ", cout.flush();
			} else {
				_input.restore();
				auto b = aexpr();
				auto c = _input.get_token();
				if (b.first && c.type == SEMICOLON) {
					if (!check_grammar_only) cout << b.second << " ", cout.flush();
				} else {
					_input.restore();
					return false;
				}
			}
		} else if (a.type == LOOP_DO) {
			while (true) {
				_input.create_restore_point();
				if (_input.get_token().type != BRACKET_MID_OPEN) {
					_input.restore();
					_input.restore();
					return false;
				}
				while (statement());
				if (_input.get_token().type != BRACKET_MID_CLOSE) {
					_input.restore();
					_input.restore();
					return false;
				}
				if (_input.get_token().type != LOOP_WHILE) {
					_input.restore();
					_input.restore();
					return false;
				}
				if (_input.get_token().type != BRACKET_SMALL_OPEN) {
					_input.restore();
					_input.restore();
					return false;
				}
				auto res = bexpr();
				if (!res.first) {
					_input.restore();
					_input.restore();
					return false;
				}
				if (!res.second || check_grammar_only) {
					_input.remove_restore_point();
					break;
				}
				_input.restore();
			}

			if (_input.get_token().type != BRACKET_SMALL_CLOSE) {
				_input.restore();
				return false;
			}
			if (_input.get_token().type != SEMICOLON) {
				_input.restore();
				return false;
			}
		} else if (a.type == STRING && variable_data.count(a.value)) {
			auto b = _input.get_token();
			if (b.type != ASSIGN) {
				_input.restore();
				return false;
			}
			auto c = aexpr();
			auto d = _input.get_token();
			if (!c.first || d.type != SEMICOLON) {
				_input.restore();
				return false;
			}
			variable_data[a.value] = c.second;
		} else {
			_input.restore();
			return false;
		}

		_input.remove_restore_point();
		return true;
	}

	pair<bool, bool> bexpr() {
		_input.create_restore_point();

		auto a = _input.get_token();
		auto b = aexpr();
		auto c = aexpr();

		if (a.type == OPERATOR_COMP && b.first && c.first) {
			pair<bool, bool> ret = {true, true};
			if (a.value == "==") ret.second = b.second == c.second;
			if (a.value == "!=") ret.second = b.second != c.second;
			if (a.value == "<") ret.second = b.second < c.second;
			if (a.value == ">") ret.second = b.second > c.second;
			if (a.value == "<=") ret.second = b.second <= c.second;
			if (a.value == ">=") ret.second = b.second >= c.second;
			_input.remove_restore_point();
			return ret;
		}
		_input.restore();
		return {false, false};
	}

	pair<bool, __int128> aexpr() {
		_input.create_restore_point();

		auto a = term();

		if (!a.first) {
			_input.restore();
			return {false, 0};
		}

		__int128 ret = a.second;
		while (true) {
			_input.create_restore_point();
			auto b = _input.get_token();
			auto c = term();
			if (b.type == OPERATOR_ARITHMETIC && c.first) {
				if (b.value == "+") ret += c.second;
				if (b.value == "-") ret -= c.second;
				if (b.value == "*") ret *= c.second;
				if (b.value == "/") ret /= c.second;

				_input.remove_restore_point();
			} else {
				_input.restore();
				break;
			}
		}

		_input.remove_restore_point();

		return {true, ret};
	}

	pair<bool, __int128> term() {
		_input.create_restore_point();

		auto a = _input.get_token();
		if (a.type == DECIMAL) {
			_input.remove_restore_point();
			return {true, atoll(a.value.c_str())};
		}
		if (a.type == STRING && variable_data.count(a.value)) {
			_input.remove_restore_point();
			return {true, variable_data[a.value]};
		}
		if (a.type == BRACKET_SMALL_OPEN) {
			auto b = aexpr();
			auto c = _input.get_token();
			if (b.first && c.type == BRACKET_SMALL_CLOSE) {
				_input.remove_restore_point();
				return {true, b.second};
			}
		}

		_input.restore();
		return {false, 0};
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
		};
		parser.clear();
		parser._input.skip_blanks();

		parser.check_grammar_only = true;

		auto res = parser.program();
		if (!res) cout << "Syntax Error!!";
		else {
			parser.check_grammar_only = false;
			parser.clear();
			parser._input.idx = 0;

			parser.program();
		}
		cout << "\n";
	}
}
