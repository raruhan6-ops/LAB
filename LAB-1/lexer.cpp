#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <map>
#include <vector>
using namespace std;

// Token structure
struct Token {
    int syn;           // Category code
    string token;      // Lexeme
    int sum;           // Numeric value (for NUM)
};

// Define keywords
map<string, int> keywords = {
    {"main", 1}, {"if", 2}, {"then", 3}, {"else", 4}, {"while", 5}, {"do", 6},
    {"repeat", 7}, {"until", 8}, {"for", 9}, {"from", 10}, {"to", 11}, {"step", 12},
    {"switch", 13}, {"of", 14}, {"case", 15}, {"default", 16}, {"return", 17},
    {"integer", 18}, {"real", 19}, {"char", 20}, {"bool", 21}, {"and", 22},
    {"or", 23}, {"not", 24}, {"mod", 25}, {"read", 26}, {"write", 27}
};

// Operators and delimiters
string operators[] = {"=", "+", "-", "*", "/", "<", "<=", ">", ">=", "!=", "," , ";", ":", "{", "}", "[", "]", "(", ")"};

// Helper functions
bool isDelimiter(char c) {
    string delims = ",;:{}[]()";
    return delims.find(c) != string::npos;
}

bool isOperatorChar(char c) {
    string ops = "=+-*/<>!";
    return ops.find(c) != string::npos;
}

bool isKeyword(const string& s) {
    return keywords.find(s) != keywords.end();
}

bool isIdentifierStart(char c) {
    return isalpha(c) || c == '_';
}

bool isIdentifierChar(char c) {
    return isalnum(c) || c == '_';
}

bool isNumChar(char c) {
    return isdigit(c);
}

void skipComment(ifstream& fin, char first, char next) {
    if (first == '/' && next == '/') {
        // single line comment
        char c;
        while (fin.get(c)) {
            if (c == '\n') break;
        }
    } else if (first == '/' && next == '*') {
        // multi-line comment
        char c, prev = 0;
        while (fin.get(c)) {
            if (prev == '*' && c == '/') break;
            prev = c;
        }
    }
}

// Main scanning function
vector<Token> scanTokens(const string& filename) {
    ifstream fin(filename);
    if (!fin.is_open()) {
        cerr << "Error: Cannot open file.\n";
        exit(1);
    }

    vector<Token> tokens;
    char ch;
    while (fin.get(ch)) {
        if (isspace(ch)) continue;

        // Handle identifiers and keywords
        if (isIdentifierStart(ch)) {
            string word;
            word += ch;
            while (fin.peek() != EOF && isIdentifierChar(fin.peek())) {
                word += fin.get();
            }
            Token t;
            if (isKeyword(word)) {
                t.syn = keywords[word];
                t.token = word;
                t.sum = -1;
            } else {
                t.syn = 100;  // Identifier code
                t.token = word;
                t.sum = -1;
            }
            tokens.push_back(t);
        }

        // Handle numbers
        else if (isNumChar(ch)) {
            string num;
            num += ch;
            while (fin.peek() != EOF && isNumChar(fin.peek())) {
                num += fin.get();
            }
            Token t;
            t.syn = 101;  // Number code
            t.token = num;
            t.sum = stoi(num);
            tokens.push_back(t);
        }

        // Handle comments and operators
        else if (isOperatorChar(ch)) {
            char next = fin.peek();
            if (ch == '/' && (next == '/' || next == '*')) {
                fin.get(); // consume next
                skipComment(fin, ch, next);
                continue;
            }
            string op(1, ch);
            if ((ch == '<' || ch == '>' || ch == '=' || ch == '!') && next == '=') {
                op += fin.get(); // consume '='
            }
            Token t;
            t.syn = 200; // Operator code
            t.token = op;
            t.sum = -1;
            tokens.push_back(t);
        }

        // Handle delimiters
        else if (isDelimiter(ch)) {
            Token t;
            t.syn = 300; // Delimiter code
            t.token = string(1, ch);
            t.sum = -1;
            tokens.push_back(t);
        }

        // Unknown characters
        else {
            cout << "Warning: Unknown symbol '" << ch << "' ignored.\n";
        }
    }

    fin.close();
    return tokens;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: lexer <sourcefile>\n";
        return 1;
    }

    vector<Token> tokens = scanTokens(argv[1]);
    cout << "\n=========== TOKEN SEQUENCE ===========\n";
    for (auto &t : tokens) {
        cout << "<syn:" << t.syn << ", token:'" << t.token << "', sum:" << t.sum << ">\n";
    }
    cout << "=====================================\n";
    return 0;
}
