import java.util.*;

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
    UNKNOWN // error
}

class Data {
    int isBool;
    Object data;

    Data(int isBool, Object data) {
        this.isBool = isBool;
        this.data = data;
    }

    @Override
    public String toString() {
        return isBool == 1 ? (boolean) data ? "TRUE" : "FALSE" : data.toString();
    }
}

class TokenInfo {
    Types type;
    String value;

    TokenInfo(Types type, String value) {
        this.type = type;
        this.value = value;
    }
}

class Input {
    String buf;
    int idx;
    List<Integer> restorePoints;

    Input(String buf, int idx) {
        this.buf = buf;
        this.idx = idx;
        this.restorePoints = new ArrayList<>();
    }

    void createRestorePoint() {
        restorePoints.add(idx);
    }

    void removeRestorePoint() {
        restorePoints.remove(restorePoints.size() - 1);
    }

    void restore() {
        idx = restorePoints.get(restorePoints.size() - 1);
        removeRestorePoint();
    }

    boolean eof() {
        return buf.length() == idx;
    }

    static boolean isNumeric(String s) {
        return s.chars().allMatch(Character::isDigit);
    }

    void skipBlanks() {
        while (!eof() && buf.charAt(idx) == ' ') idx++;
    }

    String getTokenString() {
        StringBuilder ret = new StringBuilder();
        while (!eof() && buf.charAt(idx) != ' ') {
            ret.append(buf.charAt(idx++));
        }
        skipBlanks();
        return ret.toString();
    }

    TokenInfo getToken() {
        if (eof()) return new TokenInfo(Types.EOF_, "");
        String token = getTokenString();
        if (List.of("==", "!=", "<", ">", "<=", ">=").contains(token))
            return new TokenInfo(Types.OPERATOR_COMP, token);
        else if (token.equals("="))
            return new TokenInfo(Types.ASSIGN, token);
        else if (token.equals(";"))
            return new TokenInfo(Types.SEMICOLON, token);
        else if (isNumeric(token))
            return new TokenInfo(Types.DECIMAL, token);
        else if (token.equals("print"))
            return new TokenInfo(Types.PRINT, token);
        else if (List.of("+", "-").contains(token))
            return new TokenInfo(Types.OPERATOR_PLUS_MINUS, token);
        else if (List.of("*", "/").contains(token))
            return new TokenInfo(Types.OPERATOR_MUL_DIV, token);
        else if (List.of("x", "y", "z").contains(token))
            return new TokenInfo(Types.VARIABLE, token);
        else
            return new TokenInfo(Types.UNKNOWN, token);
    }
}

class Result {
    boolean success;
    Data value;

    Result(boolean success, Data value) {
        this.success = success;
        this.value = value;
    }
}

class Parser {
    Input input;
    Map<String, Data> variableData;

    Parser(Input input) {
        this.input = input;
        this.variableData = new HashMap<>();
    }

    void parse() {
        while (true) {
            System.out.print(">> ");
            String s = new Scanner(System.in).nextLine().trim();

            if (s.equals("terminate")) break;

            input = new Input(s, 0);
            input.skipBlanks();

            Result res = program();
            if (!res.success) System.out.println("Syntax error!!");
            else if (res.value != null) System.out.println(res.value.data);
        }
    }

    Result program() {
        StringBuilder res = new StringBuilder();
        while (!input.eof()) {
            Result a = statement();
            if (!a.success) return new Result(false, null);
            if (a.value != null) res.append(a.value.data.toString().toUpperCase()).append(" ");
        }
        return new Result(true, new Data(0, res));
    }

    Result statement() {
        TokenInfo curr = input.getToken();
        if (curr.type == Types.VARIABLE) {
            TokenInfo a = input.getToken();
            Result b = expr();
            TokenInfo c = input.getToken();

            if (a.type != Types.ASSIGN || !b.success) return new Result(false, null);
            if (c.type != Types.SEMICOLON) return new Result(false, null);

            variableData.put(curr.value, new Data(b.value.isBool, b.value.data));
            return new Result(true, null);
        } else if (curr.type == Types.PRINT) {
            Result a = var();
            TokenInfo b = input.getToken();

            if (!a.success) return new Result(false, null);
            if (b.type != Types.SEMICOLON) return new Result(false, null);

            return new Result(true, variableData.containsKey(a.value.data.toString()) ? variableData.get(a.value.data.toString()) : new Data(0, 0));
        } else {
            return new Result(false, null);
        }
    }

    Result expr() {
        input.createRestorePoint();

        Result res = bexpr();
        if (res.success) {
            input.removeRestorePoint();
            return res;
        } else {
            input.restore();
            return aexpr();
        }
    }

    Result bexpr() {
        TokenInfo a = input.getToken();
        TokenInfo b = input.getToken();
        TokenInfo c = input.getToken();
        if (a.type != Types.DECIMAL || b.type != Types.OPERATOR_COMP || c.type != Types.DECIMAL)
            return new Result(false, null);
        int x = Integer.parseInt(a.value), y = Integer.parseInt(c.value);
        boolean comparisonResult;
        switch (b.value) {
            case "==":
                comparisonResult = x == y;
                break;
            case "!=":
                comparisonResult = x != y;
                break;
            case "<":
                comparisonResult = x < y;
                break;
            case ">":
                comparisonResult = x > y;
                break;
            case "<=":
                comparisonResult = x <= y;
                break;
            case ">=":
                comparisonResult = x >= y;
                break;
            default:
                return new Result(false, null);
        }
        return new Result(true, new Data(1, comparisonResult));
    }

    Result aexpr() {
        Result a = term();

        if (!a.success) return new Result(false, null);

        while (true) {
            input.createRestorePoint();

            boolean fail = false;
            TokenInfo op = input.getToken();
            if (op.type == Types.OPERATOR_PLUS_MINUS) {
                Result res = term();
                if (!res.success) fail = true;
                else {
                    int val = Integer.parseInt(a.value.data.toString());
                    int resVal = Integer.parseInt(res.value.data.toString());
                    if (op.value.equals("+")) a.value.data = val + resVal;
                    else a.value.data = val - resVal;
                }
            } else {
                fail = true;
            }

            if (fail) {
                input.restore();
                break;
            } else {
                input.removeRestorePoint();
            }
        }

        return a;
    }

    Result term() {
        Result a = factor();

        if (!a.success) return new Result(false, null);

        while (true) {
            input.createRestorePoint();
            boolean fail = false;

            TokenInfo op = input.getToken();
            if (op.type == Types.OPERATOR_MUL_DIV) {
                Result res = factor();
                if (!res.success) fail = true;
                else {
                    int val = Integer.parseInt(a.value.data.toString());
                    int resVal = Integer.parseInt(res.value.data.toString());
                    if (op.value.equals("*")) a.value.data = val * resVal;
                    else a.value.data = val / resVal;
                }
            } else {
                fail = true;
            }

            if (fail) {
                input.restore();
                break;
            } else {
                input.removeRestorePoint();
            }
        }

        return a;
    }

    Result factor() {
        return number();
    }

    Result number() {
        TokenInfo a = input.getToken();
        if (a.type != Types.DECIMAL) return new Result(false, null);
        return new Result(true, new Data(0, Integer.parseInt(a.value)));
    }

    Result var() {
        TokenInfo a = input.getToken();
        if (a.type != Types.VARIABLE) return new Result(false, null);
        return new Result(true, new Data(0, a.value));
    }
}

public class Main {
    public static void main(String[] args) {
        Input input = new Input("", 0);
        Parser parser = new Parser(input);
        parser.parse();
    }
}
