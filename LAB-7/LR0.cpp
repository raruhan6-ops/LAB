#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
using namespace std;

//=============================
// Data Structures
//=============================

struct Production {
    string left;
    vector<string> right;
};

struct Item {
    int prodId;
    int dotPos;

    bool operator<(const Item& other) const {
        if (prodId != other.prodId) return prodId < other.prodId;
        return dotPos < other.dotPos;
    }

    // Needed for comparing sets of items
    bool operator==(const Item& other) const {
        return prodId == other.prodId && dotPos == other.dotPos;
    }
};

struct ItemSet {
    set<Item> items;
    int id;
};

//=============================
// Global Variables
//=============================

vector<string> VN;
vector<string> VT;
vector<Production> prods;
string startSymbol;
string augmentedStart;

vector<ItemSet> C;
map<pair<int,string>, int> dfaTran;

map<int, map<string, string>> ACTION;
map<int, map<string,int>> GOTO;

bool isLR0 = true;

//=============================
// Helpers
//=============================

bool isTerminal(const string& s) {
    return find(VT.begin(), VT.end(), s) != VT.end();
}

bool isNonTerminal(const string& s) {
    return find(VN.begin(), VN.end(), s) != VN.end();
}

//=============================
// Read Grammar
//=============================

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
    fin.ignore();

    prods.clear();
    for (int i = 0; i < nP; ++i) {
        string line;
        getline(fin, line);
        if (line.empty()) { i--; continue; }

        string left, arrow, sym;
        stringstream ss(line);
        ss >> left >> arrow;

        vector<string> rhs;
        while (ss >> sym) rhs.push_back(sym);

        prods.push_back({left, rhs});
    }

    fin >> startSymbol;

    augmentedStart = VN[0];
}

//=============================
// CLOSURE
//=============================

ItemSet closure(const ItemSet& I) {
    ItemSet J = I;
    bool changed = true;

    while (changed) {
        changed = false;
        vector<Item> toAdd;

        for (auto it : J.items) {
            const Production& p = prods[it.prodId];
            if (it.dotPos < (int)p.right.size()) {
                string B = p.right[it.dotPos];
                if (isNonTerminal(B)) {
                    for (int pid = 0; pid < (int)prods.size(); ++pid) {
                        if (prods[pid].left == B) {
                            Item newItem = {pid, 0};
                            if (!J.items.count(newItem)) {
                                toAdd.push_back(newItem);
                            }
                        }
                    }
                }
            }
        }

        for (auto &x : toAdd) {
            J.items.insert(x);
            changed = true;
        }
    }

    return J;
}

//=============================
// GO
//=============================

ItemSet go(const ItemSet &I, const string &X) {
    ItemSet J;
    for (auto it : I.items) {
        const Production &p = prods[it.prodId];
        if (it.dotPos < (int)p.right.size() && p.right[it.dotPos] == X) {
            J.items.insert({it.prodId, it.dotPos + 1});
        }
    }

    if (J.items.empty()) return J;
    return closure(J);
}

//=============================
// Find ItemSet ID
//=============================

int findItemSetId(const ItemSet &I) {
    for (auto &s : C) {
        if (s.items == I.items) return s.id;
    }
    return -1;
}

//=============================
// Build Item Sets (DFA)
//=============================

void buildItemSets() {
    C.clear();
    dfaTran.clear();

    // Build initial item (S' -> .S)
    int startProdId = -1;
    for (int i = 0; i < (int)prods.size(); i++) {
        if (prods[i].left == augmentedStart &&
            prods[i].right.size() == 1 &&
            prods[i].right[0] == startSymbol) {
            startProdId = i;
            break;
        }
    }

    ItemSet I0;
    I0.items.insert({startProdId, 0});
    I0 = closure(I0);
    I0.id = 0;
    C.push_back(I0);

    bool changed = true;
    while (changed) {
        changed = false;

        for (int i = 0; i < (int)C.size(); i++) {
            ItemSet I = C[i];

            vector<string> symbols;
            symbols.insert(symbols.end(), VT.begin(), VT.end());
            symbols.insert(symbols.end(), VN.begin(), VN.end());

            for (auto &X : symbols) {
                ItemSet J = go(I, X);
                if (J.items.empty()) continue;

                int id = findItemSetId(J);
                if (id == -1) {
                    J.id = C.size();
                    C.push_back(J);
                    id = J.id;
                    changed = true;
                }
                dfaTran[{I.id, X}] = id;
            }
        }
    }
}

//=============================
// Print Item Sets
//=============================

void printItemSets() {
    cout << "[LR(0) item set cluster]\n";
    for (auto &I : C) {
        cout << "  I" << I.id << ":\n";
        for (auto it : I.items) {
            const Production &p = prods[it.prodId];
            cout << "        " << p.left << " -> ";
            for (int i = 0; i <= (int)p.right.size(); i++) {
                if (i == it.dotPos) cout << ". ";
                if (i < (int)p.right.size()) cout << p.right[i] << " ";
            }
            cout << "\n";
        }
    }
}

//=============================
// Print DFA
//=============================

void printDFA() {
    cout << "\n[LR(0) state tran function]\n";
    for (auto &kv : dfaTran) {
        cout << "  " << kv.first.first << " , " << kv.first.second
             << " -> " << kv.second << "\n";
    }
}

//=============================
// Build ACTION/GOTO Table
//=============================

void buildLR0Table() {
    ACTION.clear();
    GOTO.clear();
    isLR0 = true;

    auto setAction = [&](int state, const string&a, const string &act) {
        string &cell = ACTION[state][a];
        if (!cell.empty() && cell != act) isLR0 = false;
        cell = act;
    };

    for (auto &I: C) {
        int k = I.id;

        // shift
        for (auto it: I.items) {
            const Production &p = prods[it.prodId];
            if (it.dotPos < (int)p.right.size()) {
                string a = p.right[it.dotPos];
                if (isTerminal(a)) {
                    int j = dfaTran[{k,a}];
                    setAction(k, a, "s" + to_string(j));
                }
            }
        }

        // reduce or accept
        for (auto it : I.items) {
            const Production &p = prods[it.prodId];
            if (it.dotPos == (int)p.right.size()) {
                if (p.left == augmentedStart) {
                    setAction(k, "#", "acc");
                } else {
                    for (auto &a : VT)
                        setAction(k, a, "r" + to_string(it.prodId));
                    setAction(k, "#", "r" + to_string(it.prodId));
                }
            }
        }

        // GOTO
        for (auto &A : VN) {
            if (dfaTran.count({k, A})) {
                GOTO[k][A] = dfaTran[{k, A}];
            }
        }
    }
}

//=============================
// Save .lrtbl File
//=============================

void saveTable(const string& grammarFile) {
    string out = grammarFile.substr(0, grammarFile.find("."));
    out += ".lrtbl";

    ofstream fout(out.c_str());
    if (!fout) {
        cerr << "Cannot write file: " << out << endl;
        return;
    }

    int actionCount = 0;
    for (auto &r : ACTION)
        for (auto &c : r.second)
            actionCount++;

    fout << actionCount << "\n";
    for (auto &r : ACTION)
        for (auto &c : r.second)
            fout << "  " << r.first << "   " << c.first << "   " << c.second << "\n";

    int gotoCount = 0;
    for (auto &r : GOTO)
        for (auto &c : r.second)
            gotoCount++;

    fout << gotoCount << "\n";
    for (auto &r : GOTO)
        for (auto &c : r.second)
            fout << "  " << r.first << "   " << c.first << "   " << c.second << "\n";
}

//=============================
// MAIN
//=============================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: lr0 <grammar_file>\n";
        return 1;
    }

    readGrammar(argv[1]);

    // 🔥 DEBUG PRINT — verify grammar parsed correctly
    cout << "VN: ";
    for (auto &x : VN) cout << "[" << x << "]";
    cout << "\nVT: ";
    for (auto &x : VT) cout << "[" << x << "]";
    cout << "\nProductions:\n";
    for (int i = 0; i < prods.size(); i++) {
        cout << i << ": " << prods[i].left << " -> ";
        for (auto &s : prods[i].right) cout << "[" << s << "]";
        cout << "\n";
    }
    cout << "StartSymbol: [" << startSymbol << "]\n\n";

    // Build LR(0)
    buildItemSets();
    printItemSets();
    printDFA();

    // Build table
    buildLR0Table();
    cout << "\n" << (isLR0 ? "Grammar IS LR(0)" : "Grammar is NOT LR(0)") << "\n";

    saveTable(argv[1]);

    return 0;
}