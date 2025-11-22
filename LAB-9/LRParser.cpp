#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>

using namespace std;

// ============================
// Grammar structures
// ============================

struct Production {
    string left;
    vector<string> right;
};

vector<string> VN;           // non-terminals
vector<string> VT;           // terminals
vector<Production> prods;    // P
string startSymbol;

// ACTION[state][terminalID] = "s4", "r3", "acc"
map<int, map<int, string>> ACTION;

// GOTO[state][nonterminalID] = newState
map<int, map<int, int>> GOTO;

// symbol lists (index mapping)
vector<string> termList;     // 0 = "#"
vector<string> nonTermList;

// ============================
// Read grammar
// ============================

void readGrammar(const string &file) {
    ifstream fin(file);
    if (!fin) {
        cerr << "Cannot open grammar file.\n";
        exit(1);
    }

    int vnCount;
    fin >> vnCount;

    VN.resize(vnCount);
    for (int i = 0; i < vnCount; i++)
        fin >> VN[i];

    int vtCount;
    fin >> vtCount;

    VT.resize(vtCount);
    for (int i = 0; i < vtCount; i++)
        fin >> VT[i];

    int pCount;
    fin >> pCount;

    prods.resize(pCount);

    string arrow;
    for (int i = 0; i < pCount; i++) {
        fin >> prods[i].left;
        fin >> arrow; // ->
        string sym;
        string line;
        getline(fin, line);
        stringstream ss(line);
        while (ss >> sym)
            prods[i].right.push_back(sym);
    }

    fin >> startSymbol;
}

// ============================
// Read LR table
// ============================

void readTable(const string &file) {
    ifstream fin(file);
    if (!fin) {
        cerr << "Cannot open LR table.\n";
        exit(1);
    }

    int actCount;
    fin >> actCount;

    // terminals: add "#" at index 0
    termList.push_back("#");
    for (string t : VT)
        termList.push_back(t);

    for (int i = 0; i < actCount; i++) {
        int st, tid;
        string act;
        fin >> st >> tid >> act;
        ACTION[st][tid] = act;
    }

    int goCount;
    fin >> goCount;

    for (string A : VN)
        nonTermList.push_back(A);

    for (int i = 0; i < goCount; i++) {
        int st, nid, nxt;
        fin >> st >> nid >> nxt;
        GOTO[st][nid] = nxt;
    }
}

// ============================
// Read input string
// ============================

vector<string> readInput(const string &file) {
    ifstream fin(file);
    if (!fin) {
        cerr << "Cannot open input file.\n";
        exit(1);
    }

    vector<string> input;
    string a;
    while (fin >> a)
        input.push_back(a); // includes "#"

    return input;
}

// ============================
// Stack to string (for printing)
// ============================

string stackToString(stack<int> s_state, stack<string> s_sym) {
    vector<string> out;

    while (!s_state.empty()) {
        string t = to_string(s_state.top()) + " ";
        s_state.pop();
        out.push_back(t);

        if (!s_sym.empty()) {
            t = s_sym.top() + " ";
            s_sym.pop();
            out.push_back(t);
        }
    }
    reverse(out.begin(), out.end());

    string r = "";
    for (auto &x : out) r += x;
    return r;
}

// ============================
// LR parser
// ============================

void LRparse(const vector<string> &input) {
    cout << " [parsing]\n";
    cout << left << setw(12) << "栈顶"
         << setw(10) << "输入"
         << setw(8)  << "查表"
         << setw(40) << "动作"
         << "注\n";

    cout << "------------+--------+----+-------------------------------------+-----------\n";

    stack<int> S;
    stack<string> X;

    S.push(0);
    X.push("#");

    int ip = 0;

    while (true) {
        int st = S.top();
        string a = input[ip];

        // find terminal index
        int tid = -1;
        for (int i = 0; i < termList.size(); i++)
            if (termList[i] == a) tid = i;

        string act = ACTION[st][tid];

        cout << left << setw(12) << stackToString(S, X)
             << setw(10) << a
             << setw(8)  << act;

        if (act == "") {
            cout << "错误：无动作。\n";
            return;
        }

        if (act[0] == 's') {
            int to = stoi(act.substr(1));
            cout << setw(40) << ("进栈 " + to_string(to) + " " + a) << "\n";

            S.push(to);
            X.push(a);
            ip++;
        }
        else if (act[0] == 'r') {
            int p = stoi(act.substr(1));
            Production &prod = prods[p];
            int len = prod.right.size();

            cout << "出栈 " << len << " 个符号和状态  ";

            for (int i = 0; i < len; i++) {
                S.pop();
                X.pop();
            }

            int st2 = S.top();

            int nid = -1;
            for (int i = 0; i < nonTermList.size(); i++)
                if (nonTermList[i] == prod.left) nid = i;

            int nxt = GOTO[st2][nid];

            S.push(nxt);
            X.push(prod.left);

            cout << "进栈 " << nxt << " " << prod.left
                 << "        " << prod.left << " -> ";

            for (auto &s : prod.right) cout << s << " ";
            cout << "\n";
        }
        else if (act == "acc") {
            cout << setw(40) << "成功接收！" << "\n";
            cout << "------------+--------+----+-------------------------------------+-----------\n";
            cout << " end!\n";
            return;
        }
    }
}

// ============================
// main
// ============================

int main(int argc, char *argv[]) {
    if (argc != 4) {
        cout << "Usage: parser <grammar> <table> <input>\n";
        return 0;
    }

    readGrammar(argv[1]);
    readTable(argv[2]);
    vector<string> input = readInput(argv[3]);

    LRparse(input);
    return 0;
}
