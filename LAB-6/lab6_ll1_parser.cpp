#include <bits/stdc++.h>
using namespace std;

static const string EPS="ε", END="#";

struct Production { string lhs; vector<string> rhs; };
struct Grammar {
    vector<string> VN, VT;
    vector<Production> P;
    string S;
    unordered_set<string> NTset, Tset;
    unordered_map<string, vector<int>> prodsOf;
};

bool isBlank(const string& s){ for(char c: s) if(!isspace((unsigned char)c)) return false; return true; }
vector<string> splitWords(const string &line){
    vector<string> out; string tok;
    for(char c: line){ if (isspace((unsigned char)c)){ if(!tok.empty()){out.push_back(tok); tok.clear();} } else tok.push_back(c); }
    if(!tok.empty()) out.push_back(tok); return out;
}
string readNonEmptyLine(istream& in){
    string s; while (getline(in, s)) if(!isBlank(s)) return s; return "";
}

Grammar readGrammar(istream& in){
    Grammar G;
    int nVN=stoi(splitWords(readNonEmptyLine(in)).front());
    {
        auto w=splitWords(readNonEmptyLine(in)); G.VN=w;
        while ((int)G.VN.size()<nVN){ auto more=splitWords(readNonEmptyLine(in)); G.VN.insert(G.VN.end(),more.begin(),more.end()); }
    }
    int nVT=stoi(splitWords(readNonEmptyLine(in)).front());
    {
        auto w=splitWords(readNonEmptyLine(in)); G.VT=w;
        while ((int)G.VT.size()<nVT){ auto more=splitWords(readNonEmptyLine(in)); G.VT.insert(G.VT.end(),more.begin(),more.end()); }
    }
    int nP=stoi(splitWords(readNonEmptyLine(in)).front());
    G.P.reserve(nP);
    for(int i=0;i<nP;++i){
        string line=readNonEmptyLine(in);
        auto w=splitWords(line);
        int arrow=-1; for(int j=0;j<(int)w.size();++j) if(w[j]=="->"){arrow=j;break;}
        Production pr; pr.lhs=w[0];
        for(int j=arrow+1;j<(int)w.size();++j) pr.rhs.push_back(w[j]);
        G.P.push_back(move(pr));
    }
    G.S = splitWords(readNonEmptyLine(in)).front();
    for(auto &x:G.VN) G.NTset.insert(x);
    for(auto &x:G.VT) G.Tset.insert(x);
    for(int i=0;i<(int)G.P.size();++i) G.prodsOf[G.P[i].lhs].push_back(i);
    return G;
}

struct Sets {
    unordered_map<string,set<string>> FIRST, FOLLOW;
    vector< set<string> > SELECT;
};

set<string> firstOfSeq(const vector<string>& seq,
                       const unordered_map<string,set<string>>& FIRST){
    set<string> out; bool allNullable=true;
    if(seq.empty()){ out.insert(EPS); return out; }
    for(size_t i=0;i<seq.size();++i){
        const string &sym=seq[i];
        auto it=FIRST.find(sym);
        if(it==FIRST.end()){ out.insert(sym); allNullable=false; break; }
        for(auto &a: it->second) if(a!=EPS) out.insert(a);
        if(!it->second.count(EPS)){ allNullable=false; break; }
    }
    if(allNullable) out.insert(EPS);
    return out;
}

Sets computeSets(const Grammar& G){
    Sets S;
    for(auto&t:G.VT) S.FIRST[t].insert(t);
    for(auto&A:G.VN) S.FIRST[A];

    bool changed=true;
    while(changed){
        changed=false;
        for(auto &pr:G.P){
            const string &A=pr.lhs;
            if(pr.rhs.size()==1 && pr.rhs[0]==EPS){
                if(!S.FIRST[A].count(EPS)){ S.FIRST[A].insert(EPS); changed=true; }
                continue;
            }
            auto add=firstOfSeq(pr.rhs,S.FIRST);
            for(auto &x:add) if(!S.FIRST[A].count(x)){ S.FIRST[A].insert(x); changed=true; }
        }
    }

    for(auto&A:G.VN) S.FOLLOW[A];
    S.FOLLOW[G.S].insert(END);

    changed=true;
    while(changed){
        changed=false;
        for(auto &pr:G.P){
            const string &A=pr.lhs;
            const auto &rhs=pr.rhs;
            for(int i=0;i<(int)rhs.size();++i){
                const string &B=rhs[i];
                if(!G.NTset.count(B)) continue;
                vector<string> beta;
                for(int j=i+1;j<(int)rhs.size();++j) beta.push_back(rhs[j]);
                auto Fb=firstOfSeq(beta,S.FIRST);
                for(auto &a: Fb){ if(a==EPS) continue; if(!S.FOLLOW[B].count(a)){S.FOLLOW[B].insert(a); changed=true;} }
                if(beta.empty() || Fb.count(EPS)){
                    for(auto &f:S.FOLLOW[A]) if(!S.FOLLOW[B].count(f)){ S.FOLLOW[B].insert(f); changed=true; }
                }
            }
        }
    }

    S.SELECT.resize(G.P.size());
    for(int i=0;i<(int)G.P.size();++i){
        const auto &pr=G.P[i];
        if(pr.rhs.size()==1 && pr.rhs[0]==EPS){
            S.SELECT[i]=S.FOLLOW[pr.lhs];
        }else{
            auto F=firstOfSeq(pr.rhs,S.FIRST);
            if(F.count(EPS)){
                F.erase(EPS);
                S.SELECT[i]=F;
                for(auto &b:S.FOLLOW[pr.lhs]) S.SELECT[i].insert(b);
            }else S.SELECT[i]=F;
        }
    }
    return S;
}

// Build LL(1) parse table: map[A][a] = production index (>=0); -1 = error
unordered_map<string, unordered_map<string,int>> buildTable(const Grammar& G, const Sets& S){
    unordered_map<string, unordered_map<string,int>> M;
    // initialize all entries to -1
    for(auto &A:G.VN){
        for(auto &a:G.VT) M[A][a]=-1;
        M[A][END]=-1;
    }
    for(int i=0;i<(int)G.P.size();++i){
        for(auto &a:S.SELECT[i]){
            if (a==EPS) continue; // should not happen in SELECT
            M[G.P[i].lhs][a]=i;
        }
    }
    return M;
}

void printTable(const Grammar& G, const unordered_map<string, unordered_map<string,int>>& M){
    // header: VT plus #
    vector<string> cols = G.VT; cols.push_back(END);
    cout<<"预测分析表:\n      ";
    for (auto &c: cols) cout<<setw(5)<<c<<" ";
    cout<<"\n-----+";
    for (size_t i=0;i<cols.size();++i) cout<<"-----+";
    cout<<"\n";
    for (auto &A: G.VN){
        cout<<left<<setw(5)<<A<<" ";
        for (auto &c: cols){
            int idx = M.at(A).at(c);
            if (idx>=0) cout<<setw(5)<<("p"+to_string(idx))<<" ";
            else        cout<<setw(5)<<""<<" ";
        }
        cout<<"\n";
    }
    cout<<"-----+";
    for (size_t i=0;i<cols.size();++i) cout<<"-----+";
    cout<<"\n\n";
}

vector<string> readInputTokens(istream& in){
    // tokens separated by spaces; do not strip punctuation
    string line, all;
    while (getline(in, line)) { if (!all.empty()) all.push_back(' '); all += line; }
    auto v = splitWords(all);
    return v;
}

int main(int argc, char** argv){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    if (argc<2){ cerr<<"Usage: "<<argv[0]<<" <grammarfile> <inputfile(optional)>\n"; return 1; }
    ifstream gfin(argv[1]);
    if(!gfin){ cerr<<"Cannot open grammar file.\n"; return 1; }
    Grammar G = readGrammar(gfin);
    auto S = computeSets(G);
    auto M = buildTable(G, S);

    // print table
    printTable(G, M);

    if (argc<3) return 0; // only table required
    ifstream ifin(argv[2]);
    if(!ifin){ cerr<<"Cannot open input string file.\n"; return 1; }
    vector<string> inp = readInputTokens(ifin);
    inp.push_back(END);

    // analysis stack
    vector<string> stk; stk.push_back(END); stk.push_back(G.S);

    cout<<"(";
    for (size_t i=0;i+1<inp.size();++i){ if(i) cout<<","; cout<<inp[i]; }
    cout<<")#分析过程：\n";
    cout<<"初始化：#入栈，"<<G.S<<"入栈；\n";

    size_t ip = 0; int step=1; bool ok=false;
    while (true){
        string X = stk.back(); stk.pop_back();
        string a = inp[ip];

        if (X==END && a==END){ cout<<setw(2)<<setfill('0')<<step++<<setfill(' ')<<":出栈X=#， 输入c=#，匹配，成功。\n"; ok=true; break; }

        if (G.Tset.count(X) || X==END){
            if (X==a){
                cout<<setw(2)<<setfill('0')<<step++<<setfill(' ')<<":出栈X="<<X<<"， 输入c="<<a<<"，匹配，输入指针后移；\n";
                ++ip;
            } else {
                cout<<"ERROR: 终结符不匹配，栈顶="<<X<<", 输入="<<a<<"\n"; break;
            }
        } else if (G.NTset.count(X)){
            int pid = -1;
            // when a is not a VT (e.g., unexpected token), treat as error column
            if (M[X].count(a)) pid = M[X][a];
            if (pid>=0){
                const auto &pr = G.P[pid];
                cout<<setw(2)<<setfill('0')<<step++<<setfill(' ')<<":出栈X="<<X<<"， 输入c="<<a<<"，查表，M[X,c]="<<pr.lhs<<"->";
                if (pr.rhs.size()==1 && pr.rhs[0]==EPS) cout<<EPS<<"；";
                else { for (size_t i=0;i<pr.rhs.size();++i){ if(i) cout<<" "; cout<<pr.rhs[i]; } cout<<"，"; }
                cout<<"产生式右部逆序入栈；\n";
                // push RHS in reverse (skip ε)
                if (!(pr.rhs.size()==1 && pr.rhs[0]==EPS)){
                    for (int i=(int)pr.rhs.size()-1;i>=0;--i) stk.push_back(pr.rhs[i]);
                }
            } else {
                cout<<"ERROR: 表项缺失 M["<<X<<","<<a<<"]，无法分析。\n"; break;
            }
        } else {
            cout<<"ERROR: 未知栈符号 "<<X<<"\n"; break;
        }
    }

    return ok?0:2;
}
