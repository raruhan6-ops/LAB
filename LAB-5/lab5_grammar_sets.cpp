#include <bits/stdc++.h>
using namespace std;

/*
  Input format (matches your sample):
    <numNonTerm>
    <list of VN symbols separated by spaces>
    <numTerm>
    <list of VT symbols separated by spaces>
    <numProductions>
    Each production on its own line:  A -> RHS (symbols separated by spaces). Use ε for epsilon.
    <StartSymbol>

  Output:
    CFG header, then [First Set], [Follow Set], [Select Set] in the exact style shown in the brief.
*/

static const string EPS = "ε";
static const string END = "#";

struct Production {
    string lhs;
    vector<string> rhs; // empty means epsilon production, but we also accept single "ε"
};
struct Grammar {
    vector<string> VN, VT;
    vector<Production> P;
    string S;
    // quick lookups
    unordered_set<string> NTset, Tset;
    unordered_map<string, vector<int>> prodsOf; // lhs -> indices in P
};

static inline bool isBlank(const string& s){for(char c: s) if(!isspace((unsigned char)c)) return false; return true;}

vector<string> splitWords(const string &line) {
    vector<string> out;
    string tok;
    for (size_t i=0;i<line.size();++i){
        if (isspace((unsigned char)line[i])) {
            if(!tok.empty()){out.push_back(tok); tok.clear();}
        } else {
            tok.push_back(line[i]);
        }
    }
    if(!tok.empty()) out.push_back(tok);
    return out;
}

string readNonEmptyLine(istream& in){
    string s;
    while (getline(in, s)) {
        if (!isBlank(s)) return s;
    }
    return "";
}

Grammar readGrammar(istream& in){
    Grammar G;
    // VN
    int nVN = 0;
    {
        string line = readNonEmptyLine(in);
        nVN = stoi(splitWords(line).front());
    }
    {
        string line = readNonEmptyLine(in);
        G.VN = splitWords(line);
        if ((int)G.VN.size()!=nVN) {
            // tolerate extra blanks by collecting across lines
            while ((int)G.VN.size()<nVN){
                string more = readNonEmptyLine(in);
                auto w = splitWords(more);
                G.VN.insert(G.VN.end(), w.begin(), w.end());
            }
        }
    }
    // VT
    int nVT = 0;
    {
        string line = readNonEmptyLine(in);
        nVT = stoi(splitWords(line).front());
    }
    {
        string line = readNonEmptyLine(in);
        G.VT = splitWords(line);
        if ((int)G.VT.size()!=nVT) {
            while ((int)G.VT.size()<nVT){
                string more = readNonEmptyLine(in);
                auto w = splitWords(more);
                G.VT.insert(G.VT.end(), w.begin(), w.end());
            }
        }
    }
    // P
    int nP = 0;
    {
        string line = readNonEmptyLine(in);
        nP = stoi(splitWords(line).front());
    }
    G.P.reserve(nP);
    for (int i=0;i<nP;++i){
        string line = readNonEmptyLine(in);
        // Format: A -> x y z
        auto w = splitWords(line);
        // find "->"
        int arrow = -1;
        for (int j=0;j<(int)w.size();++j) if (w[j]=="->"){arrow=j; break;}
        if (arrow<=0){ cerr<<"Bad production line: "<<line<<"\n"; exit(1); }
        Production pr;
        pr.lhs = w[0];
        for (int j=arrow+1;j<(int)w.size();++j) pr.rhs.push_back(w[j]);
        // allow empty RHS as epsilon
        if (pr.rhs.size()==1 && pr.rhs[0]==EPS) {
            // keep as [ "ε" ]
        }
        G.P.push_back(move(pr));
    }
    // S
    {
        string line = readNonEmptyLine(in);
        G.S = splitWords(line).front();
    }
    // sets
    for (auto &x: G.VN) G.NTset.insert(x);
    for (auto &x: G.VT) G.Tset.insert(x);
    // map productions of each lhs
    for (int i=0;i<(int)G.P.size();++i) G.prodsOf[G.P[i].lhs].push_back(i);
    return G;
}

struct Sets {
    unordered_map<string, set<string>> FIRST, FOLLOW;
    vector< set<string> > SELECT; // per production
};

set<string> firstOfSequence(const vector<string>& seq,
                            const unordered_map<string,set<string>>& FIRST){
    set<string> out;
    bool allNullable = true;
    if (seq.empty()){
        out.insert(EPS);
        return out;
    }
    for (size_t i=0;i<seq.size();++i){
        const string &sym = seq[i];
        auto it = FIRST.find(sym);
        if (it==FIRST.end()){
            // should not happen if FIRST built for all VN/VT
            out.insert(sym);
            allNullable=false;
            break;
        }
        // add FIRST(sym) - {ε}
        for (auto &a: it->second) if (a!=EPS) out.insert(a);
        if (it->second.count(EPS)==0){ allNullable=false; break; }
    }
    if (allNullable) out.insert(EPS);
    return out;
}

Sets computeSets(const Grammar& G){
    Sets S;
    // Init FIRST for VT and VN empty
    for (auto &t: G.VT) S.FIRST[t].insert(t);
    for (auto &A: G.VN) S.FIRST[A]; // ensure keys exist

    // FIRST fixed-point
    bool changed=true;
    while (changed){
        changed=false;
        for (auto &pr: G.P){
            const string &A = pr.lhs;
            // ε production
            if (pr.rhs.size()==1 && pr.rhs[0]==EPS){
                if (!S.FIRST[A].count(EPS)){
                    S.FIRST[A].insert(EPS);
                    changed=true;
                }
                continue;
            }
            // FIRST of sequence
            auto add = firstOfSequence(pr.rhs, S.FIRST);
            for (auto &x: add){
                if (!S.FIRST[A].count(x)){
                    S.FIRST[A].insert(x);
                    changed=true;
                }
            }
        }
    }

    // FOLLOW init
    for (auto &A: G.VN) S.FOLLOW[A]; // ensure keys
    S.FOLLOW.at(G.S).insert(END);

    // FOLLOW fixed-point
    changed=true;
    while (changed){
        changed=false;
        for (auto &pr: G.P){
            const string &A = pr.lhs;
            const auto &rhs = pr.rhs;
            for (int i=0;i<(int)rhs.size();++i){
                const string &B = rhs[i];
                if (!G.NTset.count(B)) continue; // only nonterminals
                // beta = rhs[i+1..]
                vector<string> beta;
                for (int j=i+1;j<(int)rhs.size();++j) beta.push_back(rhs[j]);
                auto FIRSTbeta = firstOfSequence(beta, S.FIRST);
                // FIRST(beta) - {ε} into FOLLOW(B)
                for (auto &a: FIRSTbeta){
                    if (a==EPS) continue;
                    if (!S.FOLLOW[B].count(a)){ S.FOLLOW[B].insert(a); changed=true; }
                }
                // if ε in FIRST(beta) or beta empty: FOLLOW(A) ⊆ FOLLOW(B)
                if (beta.empty() || FIRSTbeta.count(EPS)){
                    for (auto &f: S.FOLLOW[A]){
                        if (!S.FOLLOW[B].count(f)){ S.FOLLOW[B].insert(f); changed=true; }
                    }
                }
            }
        }
    }

    // SELECT per production
    S.SELECT.resize(G.P.size());
    for (int i=0;i<(int)G.P.size();++i){
        const auto &pr = G.P[i];
        if (pr.rhs.size()==1 && pr.rhs[0]==EPS){
            // SELECT = FOLLOW(lhs)
            S.SELECT[i] = S.FOLLOW[pr.lhs];
        } else {
            auto F = firstOfSequence(pr.rhs, S.FIRST);
            if (F.count(EPS)){
                F.erase(EPS);
                S.SELECT[i] = F;
                for (auto &b: S.FOLLOW[pr.lhs]) S.SELECT[i].insert(b);
            } else {
                S.SELECT[i] = F;
            }
        }
    }
    return S;
}

string join(const vector<string>& v, const string& sep=" "){
    string s;
    for (size_t i=0;i<v.size();++i){ if (i) s+=sep; s+=v[i]; }
    return s;
}

int main(int argc, char** argv){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    if (argc<2){ cerr<<"Usage: "<<argv[0]<<" <grammarfile>\n"; return 1; }
    ifstream fin(argv[1]);
    if(!fin){ cerr<<"Cannot open grammar file.\n"; return 1; }

    Grammar G = readGrammar(fin);
    auto S = computeSets(G);

    // Print CFG
    cout<<"CFG=(VN,VT,P,S)\n";
    cout<<"  VN: "<<join(G.VN, " ")<<"\n";
    cout<<"  VT: "<<join(G.VT, " ")<<"\n";
    cout<<"  Production:\n";
    for (int i=0;i<(int)G.P.size();++i){
        cout<<"     "<<i<<": "<<G.P[i].lhs<<" -> ";
        if (G.P[i].rhs.size()==1 && G.P[i].rhs[0]==EPS) cout<<EPS;
        else cout<<join(G.P[i].rhs, " ");
        cout<<"\n";
    }
    cout<<"  StartSymbol: "<<G.S<<"\n\n";

    auto printSetLine = [](const string& name, const set<string>& st){
        cout<<setw(2)<<""<<left<<setw(16)<<name<<": ";
        for (auto &x: st) cout<<x<<"  ";
        cout<<"\n";
    };

    cout<<"[First Set]\n";
    for (auto &A: G.VN) printSetLine(A, S.FIRST[A]);
    cout<<"\n[Follow Set]\n";
    for (auto &A: G.VN) printSetLine(A, S.FOLLOW[A]);

    cout<<"\n[Select Set]\n";
    for (int i=0;i<(int)G.P.size();++i){
        string rhsStr = (G.P[i].rhs.size()==1 && G.P[i].rhs[0]==EPS) ? EPS : join(G.P[i].rhs, " ");
        string head = to_string(i)+":"+G.P[i].lhs+" -> "+rhsStr;
        cout<<setw(3)<<""<<left<<setw(30)<<head<<" : ";
        for (auto &x: S.SELECT[i]) cout<<x<<"  ";
        cout<<"\n";
    }
    return 0;
}
