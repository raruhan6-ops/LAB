#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <map>
#include <string>
#include <algorithm>

using namespace std;

// ===========================
// Structures for grammar
// ===========================
struct Production {
    string left;
    vector<string> right;
};

vector<string> VN;             // Non-terminals
vector<string> VT;             // Terminals
vector<Production> prods;      // Productions
string startSymbol;

// LR(1) table (already built offline, read from file)
map<int, map<int, string>> ACTION;   // ACTION[state][terminalId] = "s4" / "r3" / "acc"
map<int, map<int, int>>    GOTO;     // GOTO[state][nonterminalId] = state

// terminalId mapping (for this small grammar)
// 0: #, 1: =, 2: *, 3: i
int terminalId(const string &tok) {
    if (tok == "#") return 0;
    if (tok == "=") return 1;
    if (tok == "*") return 2;
    if (tok == "i") return 3;
    return -1;
}

// NonterminalId mapping for this grammar:
// 1: S, 2: L, 3: R
int nonterminalId(const string &nt) {
    if (nt == "S") return 1;
    if (nt == "L") return 2;
    if (nt == "R") return 3;
    return -1;
}

// ===========================
// Semantic structures
// ===========================

struct SemVal {
    string place;  // name of variable or temporary
};

struct Quad {
    string op;
    string arg1;
    string arg2;
    string result;
};

vector<Quad> quads;
int tempCount = 0;

string newTemp() {
    return "t" + to_string(tempCount++);
}

// For this tiny example, we don't build a full symbol table,
// but you could extend this easily.
map<string, string> symType; // id -> type (optional)

// ===========================
// File readers
// ===========================

void readGrammar(const string &filename) {
    ifstream fin(filename);
    if (!fin) {
        cerr << "Cannot open grammar file: " << filename << endl;
        exit(1);
    }

    int nVN;
    fin >> nVN;
    VN.resize(nVN);
    for (int i = 0; i < nVN; ++i) fin >> VN[i];

    int nVT;
    fin >> nVT;
    VT.resize(nVT);
    for (int i = 0; i < nVT; ++i) fin >> VT[i];

    int nP;
    fin >> nP;
    prods.clear();
    prods.resize(nP);

    string arrow;
    string line;
    getline(fin, line); // consume end of line

    for (int i = 0; i < nP; ++i) {
        // example: S' -> S
        getline(fin, line);
        if (line.empty()) {
            i--;
            continue;
        }
        stringstream ss(line);
        ss >> prods[i].left >> arrow;
        string sym;
        while (ss >> sym) {
            prods[i].right.push_back(sym);
        }
    }

    fin >> startSymbol;
}

void readTable(const string &filename) {
    ifstream fin(filename);
    if (!fin) {
        cerr << "Cannot open LR table file: " << filename << endl;
        exit(1);
    }

    int nAction;
    fin >> nAction;
    ACTION.clear();

    for (int i = 0; i < nAction; ++i) {
        int state, tid;
        string act;
        fin >> state >> tid >> act;
        ACTION[state][tid] = act;
    }

    int nGoto;
    fin >> nGoto;
    GOTO.clear();

    for (int i = 0; i < nGoto; ++i) {
        int state, nid, nxt;
        fin >> state >> nid >> nxt;
        GOTO[state][nid] = nxt;
    }
}

vector<string> readInputTokens(const string &filename) {
    ifstream fin(filename);
    if (!fin) {
        cerr << "Cannot open input file: " << filename << endl;
        exit(1);
    }

    vector<string> tokens;
    string tok;
    while (fin >> tok) {
        tokens.push_back(tok);
    }
    return tokens;
}

// ===========================
// Stack pretty printer (optional)
// ===========================

string stackToString(stack<int> stateSt, stack<string> symSt) {
    // We print like: "# 0 * 4 i 5"
    vector<string> pieces;
    while (!stateSt.empty() || !symSt.empty()) {
        if (!symSt.empty()) {
            pieces.push_back(symSt.top());
            symSt.pop();
        }
        if (!stateSt.empty()) {
            pieces.push_back(to_string(stateSt.top()));
            stateSt.pop();
        }
    }
    reverse(pieces.begin(), pieces.end());
    string r;
    for (auto &s : pieces) {
        r += s + " ";
    }
    return r;
}

// ===========================
// LR-based translator
// ===========================

void runLRTranslator(const vector<string> &inputTokens) {
    stack<int>    stateSt;
    stack<string> symSt;
    stack<SemVal> valSt;

    stateSt.push(0);
    symSt.push("#");

    int ip = 0;
    vector<string> tokens = inputTokens;

    cout << " [parsing]\n";
    cout << "Stack                Input      Action        Note\n";
    cout << "-------------------+----------+-------------+--------------------------\n";

    while (true) {
        int s = stateSt.top();
        string a = tokens[ip];
        int tid = terminalId(a);

        if (tid < 0) {
            cerr << "Unknown token: " << a << endl;
            return;
        }

        string act = ACTION[s][tid];

        cout << stackToString(stateSt, symSt);
        int pad = 20 - (int)stackToString(stateSt, symSt).size();
        if (pad < 0) pad = 0;
        cout << string(pad, ' ') << " | ";
        cout << a << "        | ";
        cout << act << "        | ";

        if (act.empty()) {
            cout << "ERROR (no ACTION entry)\n";
            break;
        }

        if (act[0] == 's') {
            int to = stoi(act.substr(1));
            cout << "shift to " << to << "\n";

            symSt.push(a);
            SemVal v;
            if (a == "i") {
                // In a real translator, you would use actual identifier names.
                v.place = "i";
            } else {
                v.place = "";
            }
            valSt.push(v);
            stateSt.push(to);
            ip++;
        } else if (act[0] == 'r') {
            int pid = stoi(act.substr(1));
            Production &p = prods[pid];
            int len = p.right.size();

            cout << "reduce by P" << pid << ": " << p.left << " -> ";
            for (auto &x : p.right) cout << x << " ";
            cout << " | ";

            // Pop RHS
            vector<SemVal> rhsVals;
            for (int i = 0; i < len; ++i) {
                if (!valSt.empty()) {
                    rhsVals.push_back(valSt.top());
                    valSt.pop();
                }
            }
            reverse(rhsVals.begin(), rhsVals.end());

            for (int i = 0; i < len; ++i) {
                if (!symSt.empty()) symSt.pop();
                if (!stateSt.empty()) stateSt.pop();
            }

            // Semantic action
            SemVal lhsVal;
            lhsVal.place = "";

            switch (pid) {
                case 0:
                    // 0: S' -> S
                    // just propagate S
                    if (!rhsVals.empty())
                        lhsVal = rhsVals[0];
                    break;

                case 1:
                    // 1: S -> L = R
                    if (rhsVals.size() == 3) {
                        // L = R
                        string Lp = rhsVals[0].place;
                        string Rp = rhsVals[2].place;
                        quads.push_back({"=", Rp, "", Lp});
                    }
                    break;

                case 2:
                    // 2: S -> R
                    if (!rhsVals.empty()) lhsVal = rhsVals[0];
                    break;

                case 3:
                    // 3: L -> * R
                    if (rhsVals.size() == 2) {
                        string Rp = rhsVals[1].place;
                        string t = newTemp();
                        quads.push_back({"*", Rp, "", t});
                        lhsVal.place = t;
                    }
                    break;

                case 4:
                    // 4: L -> i
                    if (!rhsVals.empty())
                        lhsVal.place = rhsVals[0].place; // "i"
                    break;

                case 5:
                    // 5: R -> L
                    if (!rhsVals.empty())
                        lhsVal = rhsVals[0];
                    break;

                default:
                    break;
            }

            // Push LHS
            symSt.push(p.left);
            valSt.push(lhsVal);

            int s2 = stateSt.top();
            int nid = nonterminalId(p.left);
            int to = GOTO[s2][nid];
            stateSt.push(to);
        } else if (act == "acc") {
            cout << "ACCEPT\n";
            cout << "-------------------+----------+-------------+--------------------------\n";
            cout << "Input is valid under the grammar.\n";
            break;
        } else {
            cout << "ERROR (unknown action)\n";
            break;
        }
    }

    cout << "\nGenerated quadruples:\n";
    for (int i = 0; i < (int)quads.size(); ++i) {
        cout << i << ": ("
             << quads[i].op << ", "
             << quads[i].arg1 << ", "
             << quads[i].arg2 << ", "
             << quads[i].result << ")\n";
    }
}

// ===========================
// main
// ===========================

int main(int argc, char *argv[]) {
    // We will just use fixed filenames to keep it simple.
    string grammarFile = "grammar.txt";
    string tableFile   = "table.lrtbl";
    string inputFile   = "input.txt";

    readGrammar(grammarFile);
    readTable(tableFile);
    vector<string> tokens = readInputTokens(inputFile);

    runLRTranslator(tokens);

    return 0;
}
