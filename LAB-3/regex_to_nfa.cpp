#include <bits/stdc++.h>
using namespace std;

struct State {
    int id;
    map<char, vector<int>> trans; // transitions: symbol -> list of next states
};

struct NFA {
    int start, end;
    vector<State> states;
};

// global counter for unique state IDs
int stateCount = 0;

NFA symbolNFA(char c) {
    NFA n;
    n.start = stateCount++;
    n.end = stateCount++;
    n.states.resize(stateCount);
    n.states[n.start].id = n.start;
    n.states[n.end].id = n.end;
    n.states[n.start].trans[c].push_back(n.end);
    return n;
}

NFA concatNFA(NFA a, NFA b) {
    NFA n;
    n.start = a.start;
    n.end = b.end;
    n.states.resize(max(a.states.size(), b.states.size()) + 1);
    for (auto &s : a.states)
        for (auto &t : s.trans)
            for (int x : t.second)
                n.states[s.id].trans[t.first].push_back(x);
    for (auto &s : b.states)
        for (auto &t : s.trans)
            for (int x : t.second)
                n.states[s.id].trans[t.first].push_back(x);
    n.states[a.end].trans['e'].push_back(b.start); // ε-transition
    return n;
}

NFA unionNFA(NFA a, NFA b) {
    NFA n;
    n.start = stateCount++;
    n.end = stateCount++;
    n.states.resize(stateCount);
    for (auto &s : a.states)
        for (auto &t : s.trans)
            for (int x : t.second)
                n.states[s.id].trans[t.first].push_back(x);
    for (auto &s : b.states)
        for (auto &t : s.trans)
            for (int x : t.second)
                n.states[s.id].trans[t.first].push_back(x);
    n.states[n.start].trans['e'].push_back(a.start);
    n.states[n.start].trans['e'].push_back(b.start);
    n.states[a.end].trans['e'].push_back(n.end);
    n.states[b.end].trans['e'].push_back(n.end);
    return n;
}

NFA kleeneStarNFA(NFA a) {
    NFA n;
    n.start = stateCount++;
    n.end = stateCount++;
    n.states.resize(stateCount);
    for (auto &s : a.states)
        for (auto &t : s.trans)
            for (int x : t.second)
                n.states[s.id].trans[t.first].push_back(x);
    n.states[n.start].trans['e'].push_back(a.start);
    n.states[n.start].trans['e'].push_back(n.end);
    n.states[a.end].trans['e'].push_back(a.start);
    n.states[a.end].trans['e'].push_back(n.end);
    return n;
}

void printNFA(const NFA &nfa) {
    cout << "\n==== NFA Transition Table ====\n";
    for (auto &s : nfa.states) {
        if (s.id >= stateCount) continue;
        cout << "State " << s.id << ": ";
        for (auto &t : s.trans) {
            cout << t.first << " -> { ";
            for (int x : t.second) cout << x << " ";
            cout << "}  ";
        }
        cout << "\n";
    }
    cout << "Start state: " << nfa.start << "\nFinal state: " << nfa.end << "\n";
}

/* -------------------------------
   MAIN
-------------------------------- */
int main() {
    cout << "Enter a regular expression (supported ops: |  *  concatenation): ";
    string re;
    cin >> re;

    // Build NFAs for simple patterns: a|b, ab, a*, etc.
    NFA a = symbolNFA(re[0]);
    NFA nfa;

    if (re.size() == 1)
        nfa = a;
    else if (re[1] == '*')
        nfa = kleeneStarNFA(a);
    else if (re[1] == '|') {
        NFA b = symbolNFA(re[2]);
        nfa = unionNFA(a, b);
    } else {
        NFA b = symbolNFA(re[1]);
        nfa = concatNFA(a, b);
    }

    printNFA(nfa);
    cout << "\nProcess complete.\n";
    return 0;
}
