#include <bits/stdc++.h>
using namespace std;

/* ==============================
   Structures
   ============================== */
struct NFA {
    int nStates;
    vector<char> alphabet;
    map<int, vector<pair<char, int>>> transitions;
    int start;
    int finalState;
};

struct DFA {
    vector<set<int>> states;
    map<set<int>, map<char, set<int>>> transitions;
    set<int> start;
    set<set<int>> finals;
};

/* ==============================
   ε-closure helpers
   ============================== */
set<int> epsilonClosure(int state, map<int, vector<pair<char, int>>> &trans) {
    stack<int> st;
    set<int> closure;
    st.push(state);
    closure.insert(state);
    while (!st.empty()) {
        int s = st.top(); st.pop();
        for (auto [c, nxt] : trans[s]) {
            if (c == 'e' && !closure.count(nxt)) {
                closure.insert(nxt);
                st.push(nxt);
            }
        }
    }
    return closure;
}

set<int> epsilonClosureSet(set<int> states, map<int, vector<pair<char, int>>> &trans) {
    set<int> result;
    for (int s : states) {
        auto part = epsilonClosure(s, trans);
        result.insert(part.begin(), part.end());
    }
    return result;
}

/* ==============================
   NFA → DFA Conversion
   ============================== */
DFA convertNFAtoDFA(NFA &nfa) {
    DFA dfa;
    auto startSet = epsilonClosure(nfa.start, nfa.transitions);
    queue<set<int>> q;
    q.push(startSet);
    dfa.states.push_back(startSet);
    dfa.start = startSet;

    while (!q.empty()) {
        auto cur = q.front(); q.pop();
        for (char a : nfa.alphabet) {
            if (a == 'e') continue;
            set<int> moveSet;
            for (int s : cur) {
                for (auto [c, nxt] : nfa.transitions[s]) {
                    if (c == a) moveSet.insert(nxt);
                }
            }
            if (moveSet.empty()) continue;
            auto closure = epsilonClosureSet(moveSet, nfa.transitions);
            dfa.transitions[cur][a] = closure;
            if (find(dfa.states.begin(), dfa.states.end(), closure) == dfa.states.end()) {
                dfa.states.push_back(closure);
                q.push(closure);
            }
        }
    }

    for (auto st : dfa.states)
        if (st.count(nfa.finalState))
            dfa.finals.insert(st);

    return dfa;
}

/* ==============================
   DFA Minimization
   ============================== */
DFA minimizeDFA(DFA &dfa, vector<char> &alphabet) {
    vector<set<set<int>>> partitions;
    set<set<int>> finals = dfa.finals;
    set<set<int>> nonfinals;
    for (auto st : dfa.states)
        if (!finals.count(st)) nonfinals.insert(st);
    if (!finals.empty()) partitions.push_back(finals);
    if (!nonfinals.empty()) partitions.push_back(nonfinals);

    bool changed = true;
    while (changed) {
        changed = false;
        vector<set<set<int>>> newParts;
        for (auto &part : partitions) {
            map<vector<int>, set<set<int>>> groups;
            for (auto &st : part) {
                vector<int> sig;
                for (char a : alphabet) {
                    if (a == 'e') continue;
                    auto target = dfa.transitions[st][a];
                    int idx = -1;
                    for (size_t i = 0; i < partitions.size(); ++i)
                        if (!target.empty() && partitions[i].count(target))
                            idx = (int)i;
                    sig.push_back(idx);
                }
                groups[sig].insert(st);
            }
            for (auto &[_, g] : groups)
                newParts.push_back(g);
        }
        if (newParts.size() != partitions.size()) {
            partitions = newParts;
            changed = true;
        }
    }

    DFA minDFA;
    for (auto &part : partitions) {
        set<int> rep = *part.begin();
        minDFA.states.push_back(rep);
        if (dfa.start == *part.begin())
            minDFA.start = rep;
        for (auto &st : part)
            if (dfa.finals.count(st))
                minDFA.finals.insert(rep);
    }

    for (auto &part : partitions) {
        auto rep = *part.begin();
        for (char a : alphabet) {
            if (a == 'e') continue;
            auto target = dfa.transitions[rep][a];
            if (target.empty()) continue;
            for (auto &p : partitions)
                if (p.count(target)) {
                    set<int> srcRep(rep.begin(), rep.end());
                    set<int> tgtRep((*p.begin()).begin(), (*p.begin()).end());
                    minDFA.transitions[srcRep][a] = tgtRep;
                }
        }
    }
    return minDFA;
}

/* ==============================
   Printing Helpers
   ============================== */
void printSet(const set<int> &s) {
    cout << "{";
    for (auto it = s.begin(); it != s.end(); ++it) {
        cout << *it;
        if (next(it) != s.end()) cout << ",";
    }
    cout << "}";
}

void printDFA(DFA &dfa, vector<char> &alphabet, const string &title) {
    cout << "\n==== " << title << " ====\n";
    int idx = 0;
    for (auto &st : dfa.states) {
        cout << "State " << idx++ << " = ";
        printSet(st);
        cout << " | ";
        for (char a : alphabet) {
            if (a == 'e') continue;
            cout << a << "->";
            printSet(dfa.transitions[st][a]);
            cout << "  ";
        }
        if (dfa.finals.count(st)) cout << "[Final]";
        if (dfa.start == st) cout << " [Start]";
        cout << "\n";
    }
}

/* ==============================
   Main
   ============================== */
int main() {
    NFA nfa;
    cout << "Reading NFA from nfa_input.txt...\n";

    ifstream fin("nfa_input.txt");
    if (!fin.is_open()) {
        cerr << "Cannot open file nfa_input.txt\n";
        return 1;
    }

    int n, k;
    fin >> n >> k;
    nfa.nStates = n;
    nfa.alphabet.resize(k);
    for (int i = 0; i < k; ++i)
        fin >> nfa.alphabet[i];

    int from, to;
    char sym;
    string line;
    getline(fin, line); // consume leftover newline
    while (getline(fin, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        if (!(ss >> from)) continue;

        // line with only one number = start state
        if (ss.eof()) {
            nfa.start = from;
            fin >> nfa.finalState;
            break;
        }

        ss >> sym >> to;
        nfa.transitions[from].push_back({sym, to});
    }
    fin.close();

    DFA dfa = convertNFAtoDFA(nfa);
    printDFA(dfa, nfa.alphabet, "Constructed DFA");

    DFA minDFA = minimizeDFA(dfa, nfa.alphabet);
    printDFA(minDFA, nfa.alphabet, "Minimized DFA");

    cout << "\nProcess complete.\n";
    return 0;
}
