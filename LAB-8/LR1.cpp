#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
using namespace std;

// =============================
// Data structures
// =============================

struct Production {
    string left;
    vector<string> right;
};

struct LR1Item {
    int prodId;         // index in prods
    int dotPos;         // position of dot
    string lookahead;   // terminal or "#"

    bool operator<(const LR1Item& o) const {
        if (prodId != o.prodId) return prodId < o.prodId;
        if (dotPos != o.dotPos) return dotPos < o.dotPos;
        return lookahead < o.lookahead;
    }

    bool operator==(const LR1Item& o) const {
        return prodId == o.prodId &&
               dotPos == o.dotPos &&
               lookahead == o.lookahead;
    }
};

struct ItemSet {
    set<LR1Item> items;
    int id;
};

// =============================
// Global variables
// =============================

vector<string> VN;               // nonterminals
vector<string> VT;               // terminals
vector<Production> prods;        // productions
string startSymbol;              // start symbol S'
string augmentedStart;           // usually same as VN[0]

vector<ItemSet> C;               // canonical LR(1) item sets
map<pair<int,string>, int> dfaTran;  // (state, symbol) -> next state

map<string, set<string>> FIRST;  // FIRST sets
map<int, map<string,string>> ACTION; // ACTION[state][terminal] = "s4", "r3", "acc"
map<int, map<string,int>> GOTO;      // GOTO[state][nonterminal] = state

bool isLR1 = true;

// =============================
// Helper: symbol type
// =============================

bool isTerminal(const string& s) {
    return find(VT.begin(), VT.end(), s) != VT.end();
}

bool isNonTerminal(const string& s) {
    return find(VN.begin(), VN.end(), s) != VN.end();
}

// =============================
// Read grammar from file
// =============================

void readGrammar(const string& filename) {
    ifstream fin(filename.c_str());
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
    fin.ignore();  // skip end of line

    prods.clear();
    for (int i = 0; i < nP; ++i) {
        string line;
        getline(fin, line);
        if (line.empty()) { i--; continue; } // skip empty line

        string left, arrow, sym;
        stringstream ss(line);
        ss >> left >> arrow; // arrow is "->"

        vector<string> rhs;
        while (ss >> sym) {
            rhs.push_back(sym);
        }
        prods.push_back({left, rhs});
    }

    fin >> startSymbol;

    augmentedStart = VN[0]; // first nonterminal is S'
}

// =============================
// FIRST set computation
// =============================

void computeFirstSets() {
    FIRST.clear();

    // 1) terminals: FIRST(a) = { a }
    for (const auto& t : VT) {
        FIRST[t].insert(t);
    }
    // special end marker
    FIRST["#"].insert("#");

    // 2) nonterminals: iterative algorithm
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& p : prods) {
            const string& A = p.left;
            const vector<string>& alpha = p.right;

            // if empty production A -> ε (not present in your labs, but we support it)
            if (alpha.empty()) {
                if (!FIRST[A].count("ε")) {
                    FIRST[A].insert("ε");
                    changed = true;
                }
                continue;
            }

            bool nullablePrefix = true;
            for (size_t i = 0; i < alpha.size(); ++i) {
                const string& X = alpha[i];

                // ensure FIRST[X] exists
                if (!FIRST.count(X) && isNonTerminal(X)) {
                    FIRST[X] = {};
                }

                // add FIRST(X) \ {ε} to FIRST(A)
                for (const auto& t : FIRST[X]) {
                    if (t == "ε") continue;
                    if (!FIRST[A].count(t)) {
                        FIRST[A].insert(t);
                        changed = true;
                    }
                }

                // if X cannot produce ε, stop
                if (!FIRST[X].count("ε")) {
                    nullablePrefix = false;
                    break;
                }
            }

            if (nullablePrefix) {
                if (!FIRST[A].count("ε")) {
                    FIRST[A].insert("ε");
                    changed = true;
                }
            }
        }
    }
}

// FIRST of a sequence of symbols (could include "#")
set<string> firstOfSequence(const vector<string>& seq) {
    set<string> result;
    if (seq.empty()) {
        result.insert("ε");
        return result;
    }

    bool nullablePrefix = true;
    for (size_t i = 0; i < seq.size(); ++i) {
        const string& X = seq[i];

        if (X == "#") {
            result.insert("#");
            nullablePrefix = false;
            break;
        }

        if (!FIRST.count(X) && isNonTerminal(X)) {
            FIRST[X] = {};
        }

        for (const auto& t : FIRST[X]) {
            if (t == "ε") continue;
            result.insert(t);
        }

        if (!FIRST[X].count("ε")) {
            nullablePrefix = false;
            break;
        }
    }

    if (nullablePrefix) {
        result.insert("ε");
    }

    return result;
}

// =============================
// CLOSURE for LR(1)
// =============================

ItemSet closureLR1(const ItemSet& I) {
    ItemSet J = I;
    bool changed = true;

    while (changed) {
        changed = false;
        vector<LR1Item> toAdd;

        for (const auto& it : J.items) {
            const Production& p = prods[it.prodId];
            if (it.dotPos < (int)p.right.size()) {
                string B = p.right[it.dotPos];
                if (isNonTerminal(B)) {
                    // β a sequence
                    vector<string> beta;
                    for (size_t k = it.dotPos + 1; k < p.right.size(); ++k) {
                        beta.push_back(p.right[k]);
                    }
                    beta.push_back(it.lookahead); // append lookahead

                    set<string> firstSet = firstOfSequence(beta);

                    // for each production B -> gamma
                    for (int pid = 0; pid < (int)prods.size(); ++pid) {
                        if (prods[pid].left == B) {
                            for (const auto& b : firstSet) {
                                if (b == "ε") continue;
                                LR1Item newItem{pid, 0, b};
                                if (!J.items.count(newItem)) {
                                    toAdd.push_back(newItem);
                                }
                            }
                        }
                    }
                }
            }
        }

        if (!toAdd.empty()) {
            for (const auto& it : toAdd) {
                J.items.insert(it);
            }
            changed = true;
        }
    }

    return J;
}

// =============================
// GOTO for LR(1)
// =============================

ItemSet goLR1(const ItemSet& I, const string& X) {
    ItemSet J;
    for (const auto& it : I.items) {
        const Production& p = prods[it.prodId];
        if (it.dotPos < (int)p.right.size() && p.right[it.dotPos] == X) {
            LR1Item moved{it.prodId, it.dotPos + 1, it.lookahead};
            J.items.insert(moved);
        }
    }
    if (J.items.empty()) return J;
    return closureLR1(J);
}

// =============================
// Find item set by content
// =============================

int findItemSetId(const ItemSet& I) {
    for (const auto& s : C) {
        if (s.items == I.items) return s.id;
    }
    return -1;
}

// =============================
// Build canonical LR(1) collection
// =============================

void buildItemSets() {
    C.clear();
    dfaTran.clear();

    // Start item: production 0, dot before first symbol, lookahead "#"
    // In lab format, production 0 is S' -> S
    LR1Item start{0, 0, "#"};
    ItemSet I0;
    I0.items.insert(start);
    I0 = closureLR1(I0);
    I0.id = 0;
    C.push_back(I0);

    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < C.size(); ++i) {
            ItemSet I = C[i];

            // all symbols: VT + VN
            vector<string> symbols;
            symbols.insert(symbols.end(), VT.begin(), VT.end());
            symbols.insert(symbols.end(), VN.begin(), VN.end());

            for (const auto& X : symbols) {
                ItemSet J = goLR1(I, X);
                if (J.items.empty()) continue;

                int id = findItemSetId(J);
                if (id == -1) {
                    J.id = (int)C.size();
                    C.push_back(J);
                    id = J.id;
                    changed = true;
                }
                dfaTran[{I.id, X}] = id;
            }
        }
    }
}

// =============================
// Printing
// =============================

void printCFG() {
    cout << " CFG=(VN,VT,P,S)\n";
    cout << " VN:";
    for (auto& v : VN) cout << " " << v;
    cout << "\n  VT:";
    for (auto& t : VT) cout << " " << t;
    cout << "\n  Production:\n";
    for (int i = 0; i < (int)prods.size(); ++i) {
        cout << "     " << i << ": " << prods[i].left << " -> ";
        for (auto& s : prods[i].right) cout << s << " ";
        cout << "\n";
    }
    cout << "  StartSymbol: " << startSymbol << "\n\n";
}

void printItemSets() {
    cout << "[LR(1) item set cluster]\n";
    for (auto& I : C) {
        cout << "  I" << I.id << " :\n";
        for (const auto& it : I.items) {
            const Production& p = prods[it.prodId];
            cout << "        " << p.left << " -> ";
            for (int i = 0; i <= (int)p.right.size(); ++i) {
                if (i == it.dotPos) cout << ". ";
                if (i < (int)p.right.size()) cout << p.right[i] << " ";
            }
            cout << ", " << it.lookahead << "\n";
        }
    }
    cout << "\n";
}

void printDFA() {
    cout << "[LR(1) state tran function]\n";
    for (const auto& kv : dfaTran) {
        int s = kv.first.first;
        string X = kv.first.second;
        int t = kv.second;
        cout << "  " << s << " ,  " << X << "        -> " << t << "\n";
    }
    cout << "\n";
}

// =============================
// ACTION / GOTO construction
// =============================

int terminalIndex(const string& sym) {
    if (sym == "#") return 0;
    for (int i = 0; i < (int)VT.size(); ++i) {
        if (VT[i] == sym) return i + 1; // 1..nVT
    }
    return -1;
}

int nonterminalIndex(const string& sym) {
    // VN[0] is S' (augmented); we usually index from 1
    for (int i = 1; i < (int)VN.size(); ++i) {
        if (VN[i] == sym) return i; // 1..(nVN-1)
    }
    return -1;
}

void buildLR1Table() {
    ACTION.clear();
    GOTO.clear();
    isLR1 = true;

    auto setAction = [&](int state, const string& a, const string& act) {
        string& cell = ACTION[state][a];
        if (!cell.empty() && cell != act) {
            isLR1 = false;
        }
        cell = act;
    };

    for (const auto& I : C) {
        int k = I.id;

        // shift actions
        for (const auto& it : I.items) {
            const Production& p = prods[it.prodId];
            if (it.dotPos < (int)p.right.size()) {
                string a = p.right[it.dotPos];
                if (isTerminal(a)) {
                    auto itTran = dfaTran.find({k, a});
                    if (itTran != dfaTran.end()) {
                        int j = itTran->second;
                        setAction(k, a, "s" + to_string(j));
                    }
                }
            }
        }

        // reduce / accept
        for (const auto& it : I.items) {
            const Production& p = prods[it.prodId];
            if (it.dotPos == (int)p.right.size()) {
                // completed production A -> α , lookahead a
                if (p.left == augmentedStart && it.lookahead == "#") {
                    setAction(k, "#", "acc");
                } else {
                    string a = it.lookahead;
                    setAction(k, a, "r" + to_string(it.prodId));
                }
            }
        }

        // GOTO
        for (const auto& A : VN) {
            if (!isNonTerminal(A)) continue;
            auto itTran = dfaTran.find({k, A});
            if (itTran != dfaTran.end()) {
                GOTO[k][A] = itTran->second;
            }
        }
    }
}

// =============================
// Save .lrtbl
// =============================

void saveTable(const string& grammarFile) {
    string out = grammarFile;
    size_t pos = out.find_last_of('.');
    if (pos != string::npos) out = out.substr(0, pos);
    out += ".lrtbl";

    ofstream fout(out.c_str());
    if (!fout) {
        cerr << "Cannot write file: " << out << endl;
        return;
    }

    // ACTION
    vector<tuple<int,int,string>> actionEntries;
    for (const auto& row : ACTION) {
        int state = row.first;
        for (const auto& col : row.second) {
            int tid = terminalIndex(col.first);
            if (tid >= 0) {
                actionEntries.push_back({state, tid, col.second});
            }
        }
    }

    fout << actionEntries.size() << "\n";
    for (auto& e : actionEntries) {
        int st, tid;
        string act;
        tie(st, tid, act) = e;
        fout << st << " " << tid << " " << act << "\n";
    }

    // GOTO
    vector<tuple<int,int,int>> gotoEntries;
    for (const auto& row : GOTO) {
        int state = row.first;
        for (const auto& col : row.second) {
            int nid = nonterminalIndex(col.first);
            if (nid >= 0) {
                gotoEntries.push_back({state, nid, col.second});
            }
        }
    }

    fout << "\n" << gotoEntries.size() << "\n";
    for (auto& e : gotoEntries) {
        int st, nid, to;
        tie(st, nid, to) = e;
        fout << st << " " << nid << " " << to << "\n";
    }
}

// =============================
// MAIN
// =============================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: lr1 <grammar_file>\n";
        return 1;
    }

    string grammarFile = argv[1];
    readGrammar(grammarFile);
    computeFirstSets();

    printCFG();

    buildItemSets();
    printItemSets();
    printDFA();

    buildLR1Table();

    if (isLR1) {
        cout << "文法是 LR(1) 文法！\n";
    } else {
        cout << "文法不是 LR(1) 文法！\n";
    }

    saveTable(grammarFile);

    return 0;
}