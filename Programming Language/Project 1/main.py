from enum import Enum
import re

class Types(Enum):
    OPERATOR_COMP = 0
    OPERATOR_PLUS_MINUS = 1
    OPERATOR_MUL_DIV = 2
    ASSIGN = 3
    SEMICOLON = 4
    DECIMAL = 5
    VARIABLE = 6
    PRINT = 7
    EOF_ = 8
    UNKNOWN = 9

class bool_int:
    def __init__(self, is_bool, data):
        self.is_bool = is_bool
        self.data = data

    def __str__(self):
        return "TRUE" if self.is_bool else str(self.data)

class token_info:
    def __init__(self, type_, value):
        self.type = type_
        self.value = value

class input_:
    def __init__(self, buf, idx):
        self.buf = buf
        self.idx = idx
        self.restore_points = []

    def create_restore_point(self):
        self.restore_points.append(self.idx)

    def remove_restore_point(self):
        self.restore_points.pop()

    def restore(self):
        self.idx = self.restore_points.pop()

    def eof(self):
        return self.idx == len(self.buf)

    @staticmethod
    def is_numeric(s):
        return all(char.isdigit() for char in s)

    def skip_blanks(self):
        while not self.eof() and self.buf[self.idx] == ' ':
            self.idx += 1

    def get_token_string(self):
        ret = ''
        while not self.eof() and self.buf[self.idx] != ' ':
            ret += self.buf[self.idx]
            self.idx += 1
        self.skip_blanks()
        return ret

    def get_token(self):
        if self.eof():
            return token_info(Types.EOF_, '')
        token = self.get_token_string()
        if token in ['==', '!=', '<', '>', '<=', '>=']:
            return token_info(Types.OPERATOR_COMP, token)
        elif token == '=':
            return token_info(Types.ASSIGN, token)
        elif token == ';':
            return token_info(Types.SEMICOLON, token)
        elif self.is_numeric(token):
            return token_info(Types.DECIMAL, token)
        elif token == 'print':
            return token_info(Types.PRINT, token)
        elif token in ['+', '-']:
            return token_info(Types.OPERATOR_PLUS_MINUS, token)
        elif token in ['*', '/']:
            return token_info(Types.OPERATOR_MUL_DIV, token)
        elif token in ['x', 'y', 'z']:
            return token_info(Types.VARIABLE, token)
        else:
            return token_info(Types.UNKNOWN, token)

class parser:
    def __init__(self, _input):
        self._input = _input
        self.variable_data = {}

    def program(self):
        res = ''
        while not self._input.eof():
            a = self.statement()
            if not a[0]:
                return "syntax error!!"
            if a[1]:
                res += a[1] + " "
        return res

    def statement(self):
        curr = self._input.get_token()
        if curr.type == Types.VARIABLE:
            a = self._input.get_token()
            b = self.expr()
            c = self._input.get_token()

            if a.type != Types.ASSIGN or not b[0]:
                return [False, ""]
            if c.type != Types.SEMICOLON:
                return [False, ""]

            self.variable_data[curr.value] = b[1]
            return [True, ""]
        elif curr.type == Types.PRINT:
            a = self.var()
            b = self._input.get_token()

            if not a[0]:
                return [False, ""]
            if b.type != Types.SEMICOLON:
                return [False, ""]

            return [True, str(self.variable_data[a[1]] if a[1] in self.variable_data else 0).upper()]
        else:
            return [False, ""]

    def expr(self):
        self._input.create_restore_point()

        res = self.bexpr()
        if not res[0]:
            self._input.restore()
            res = self.aexpr()
            return res

        self._input.remove_restore_point()
        return res

    def bexpr(self):
        a = self._input.get_token()
        b = self._input.get_token()
        c = self._input.get_token()

        if a.type != Types.DECIMAL or b.type != Types.OPERATOR_COMP or c.type != Types.DECIMAL:
            return [False, '']

        x, y = int(a.value), int(c.value)
        if b.value == '==':
            return [True, x == y]
        elif b.value == '!=':
            return [True, x != y]
        elif b.value == '<':
            return [True, x < y]
        elif b.value == '>':
            return [True, x > y]
        elif b.value == '<=':
            return [True, x <= y]
        elif b.value == '>=':
            return [True, x >= y]
        else:
            return [False, '']

    def aexpr(self):
        a = self.term()

        if not a[0]:
            return [False, 0]

        while True:
            flag = False
            self._input.create_restore_point()
            op = self._input.get_token()
            if op.type == Types.OPERATOR_PLUS_MINUS:
                res = self.term()
                if not res[0]:
                    flag = True
                    a[0] = False
                else:
                    if op.value == '+':
                        a[1] += res[1]
                    else:
                        a[1] -= res[1]
            else:
                flag = True

            if flag:
                self._input.restore()
                break
            else:
                self._input.remove_restore_point()

        return a

    def term(self):
        a = self.factor()

        if not a[0]:
            return [False, 0]

        while True:
            flag = False
            self._input.create_restore_point()
            op = self._input.get_token()
            if op.type == Types.OPERATOR_MUL_DIV:
                res = self.factor()
                if not res[0]:
                    flag = True
                    a[0] = False
                else:
                    if op.value == '*':
                        a[1] *= res[1]
                    else:
                        a[1] //= res[1]
            else:
                flag = True

            if flag:
                self._input.restore()
                break
            else:
                self._input.remove_restore_point()

        return a

    def factor(self):
        return self.number()

    def number(self):
        a = self._input.get_token()
        if a.type != Types.DECIMAL:
            return [False, 0]
        return [True, int(a.value)]

    def var(self):
        a = self._input.get_token()
        if a.type != Types.VARIABLE:
            return [False, ""]
        return [True, a.value]

def main():
    while True:
        print(">> ", end='')
        s = input()

        if s == "terminate":
            break

        parser_ = parser(input_(s, 0))
        parser_._input.skip_blanks()

        res = parser_.program()
        print(res)

main()
