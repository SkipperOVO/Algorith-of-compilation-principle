#include <iostream>
#include <stack>
#include <ctype.h>
#include <map>
#include <queue>
#include <set>
using namespace std;

#define MAXSIZE 256

//State 是 NFA 中的节点	每个状态有三条出边
struct State {
	static int ID;						
	int id;								//节点的 id; 
	State* outE[2] = {nullptr,nullptr};	//两条标记 epsilon 的边
	State *outC = nullptr;					//字母表标记的出边
	char what = 0;					//标记转换边的字符
	bool isAccept = false;				//是否是接受状态

	State() {
		id = ID++;					//每构造一个 State ，id 自动自增
	}
};
int State::ID = 1;

//一个 NFA 包含一个初始状态节点和接受状态节点		最简单的 NFA ：O-a->1
struct NFA {
	State *start,*end;
	NFA(State* start,State* end) {
		this->start = start;
		this->end = end;
	}
};

//正规表达式的字母表可以不止包含字母，这里简化问题，认为只有 26 个小写字母
struct AlphaTable {
	int size = 0;
	char alpha[255];
}alphaTable;

char alphaHash[26];		//字母哈希表,用于字母查重

int Dtran[MAXSIZE][MAXSIZE];			//DFA 的状态转换表
int acceptState[MAXSIZE];		//DFA 的接受状态表(我自定义的)

//将正规表达式转化为他的后缀表达式
string getPost(const string&);
//根据正规表达式（的后缀形式）构造一个 NFA (NFA 建模为图)
NFA constructNFA(const string&);
//给定一个起始状态 输出一个 NFA 
void NFAoutput(State *);
//将 NFA 转化成 DFA 的 Dtran表
void transNFAToDFA(State*);
//对输入串的匹配过程
bool match(string inputStr) ;
//计算一个状态集合的 epsilon 转换集合（从这个集合中所有节点经过一次或多次 epsilon 转换后可
//到达的节点集合及它本身节点集合
set<State*> e_closure(set<State*> stateSet);

int main(int argc, char const *argv[])
{
	NFA nfa = constructNFA(getPost("((a|b)*.c)*|p"));
	
	NFAoutput(nfa.start);

	transNFAToDFA(nfa.start);

	// output Dtran 表
	for(int i = 1; i <= 12; ++i) {
		for(int j = 0; j <= 12; ++j) {
			cout << Dtran[i][j] << " ";
		}
		cout << endl;
	}

	for(int i = 0; i < 10; ++i) {
		if(acceptState[i])
			cout << "acceptState: " << i << endl;
	}

	string str;
	cin >> str;
	bool res = match(str);
	if(res) cout << "matched!\n";
	else cout << "failed!\n";

	return 0;
}

bool match(string inputStr) {
	int state = 1;
	for(int i = 0; i < inputStr.length(); ++i) {
		state = Dtran[state][inputStr[i]-'a'];
		if(state == 0) return false;
	}
	if(acceptState[state])
		return true;
	else false;
}

void transNFAToDFA(State* start) {
	int id = 1;
	set<State*> T = e_closure(set<State*>({start}));
	set<set<State*>> dStates;							//DFA 的状态集，每个状态也是一个状态集合
	queue<set<State*>> dQueue;
	map<set<State*>,int> index;							//根据整数索引一个状态集，这个状态集也是 DFA 的一个状态
	dStates.insert(T);
	dQueue.push(T);
	index.insert(pair<set<State*>,int> (T,id++));

	while(!dQueue.empty()) {
		set<State*> states = dQueue.front();
		dQueue.pop();
		for(int i = 0; i < alphaTable.size; ++i) {  //对每个字母表中的字母
			int from = index[states];				//获取当前 states 状态的索引 id
			char a = alphaTable.alpha[i];
			set<State*> newStates;

			bool isAccept = false;					//新的状态集是否是接受状态集（包含一个接受状态节点）
			for(set<State*>::iterator it = states.begin(); it != states.end(); ++it) { //状态集 states 中每个状态在每个字母表中字母上的转换形成一个新的状态（集）
				if((*it)->what == a) {								//该状态存在读取一个给定字符的转换
					newStates.insert((*it)->outC);
				}
			}
			newStates = e_closure(newStates);
			for(set<State*>::iterator it = newStates.begin(); it != newStates.end(); ++it) {
				if((*it)->isAccept == true) isAccept = true;
			}
			if(dStates.count(newStates) == 0) {			//新的状态不在 DStates 中
				dStates.insert(newStates);
				dQueue.push(newStates);					//标记为一个新的“未标记”状体
				index.insert(pair<set<State*>,int>(newStates,id++));		//新建一个索引
			}
			Dtran[from][a-'a'] = index[newStates];		//构建转换表,状态 from 经过字符 a(变量,不是‘a') 到达 newStates
			if(isAccept == true)
				acceptState[Dtran[from][a-'a']] = 1;
		}
	}
}


set<State*> e_closure(set<State*> stateSet) {
	set<State*> res = stateSet;
	stack<State*> stateStack;

	for(set<State*>::iterator it = stateSet.begin(); it != stateSet.end(); it++) {
		stateStack.push(*it);
	}

	while(!stateStack.empty()) {
		State* top = stateStack.top();
		stateStack.pop();
		State* e1 = top->outE[0];
		State* e2 = top->outE[1];
		if(e1 != nullptr) {
			if(!res.count(e1)) {
				res.insert(e1);
				stateStack.push(e1);
			}
		}
		if(e2 != nullptr) {
			if(!res.count(e2)) {
				res.insert(e2);
				stateStack.push(e2);
			}
		}
	}
	return res;
}


//给定一个起始状态 输出一个 NFA 
int vis[1001];
bool firstTime = true;

void NFAoutput(State *state) {
	if(firstTime) firstTime = false;
	else {
		cout << state->id  << ";"; 
		if(state->isAccept) {
			cout << "(Accept)" << endl;
		} else cout << endl;
	}

	if(!vis[state->id]) vis[state->id] = 1;
	else  return ;							//访问过的节点会被(上面代码块）输出但是不会再递归访问

	if(state->outE[0] != nullptr) {
		cout << state->id << "-e->";
		NFAoutput(state->outE[0]);
	}
	if(state->outE[1] != nullptr) {
		cout << state->id << "-e->";
		NFAoutput(state->outE[1]);
	}
	if(state->outC != nullptr) {
		cout << state->id << "-" << state->what << "->";
		NFAoutput(state->outC);
	}
}

NFA constructNFA(const string& postRex) {
	stack<NFA> nfaStack;
	int id = 1;
	for(int i = 0; i < postRex.length(); ++i) {
		if(isalpha(postRex[i])) {				//单个字母表字符，构造一个在这个字符上的转换的 NFA
			State *start = new State(),*end = new State();
			start->what = postRex[i];
			start->outC = end;
			end->isAccept = true;
			nfaStack.push(NFA(start,end));

			//更新字母表
			if(alphaHash[postRex[i]-'a'] == 0) {
				alphaHash[postRex[i]-'a'] = 1;
				alphaTable.alpha[alphaTable.size++] = postRex[i];
			} 
		} else if(postRex[i] == '.') {			//连接操作
			//ERROR
			if(nfaStack.size() < 2) {
				cout << "ERROR(01): Number of oprand is not sufficent\n";
				return NFA(nullptr,nullptr);
			}
			//

			NFA n2 = nfaStack.top(); nfaStack.pop();
			NFA n1 = nfaStack.top(); nfaStack.pop();

			n1.end->outE[0] = n2.start->outE[0];
			n1.end->outE[1] = n2.start->outE[1];
			n1.end->outC = n2.start->outC;
			n1.end->what = n2.start->what;
			n1.end->isAccept = false;
			delete n2.start;
			// n2.end->id--;
			// State::ID--;
			// n1.start->outC = n2.start;					
			// delete n1.end;
			// n1.end = n2.start;
			nfaStack.push(NFA(n1.start,n2.end));

		} else if(postRex[i] == '|') {
			//ERROR
			if(nfaStack.size() < 2) {
				cout << "ERROR(02): Number of oprand is not sufficent\n";
				return NFA(nullptr,nullptr);
			}
			//

			State *start = new State(),*end = new State();
			end->isAccept = true;
			NFA n2 = nfaStack.top(); nfaStack.pop();
			NFA n1 = nfaStack.top(); nfaStack.pop();

			start->outE[0] = n1.start;					//两个 epsilon 转换
			start->outE[1] = n2.start;
			n1.end->outE[0] = end;
			n2.end->outE[0] = end;

			n1.end->isAccept = false;
			n2.end->isAccept = false;

			nfaStack.push(NFA(start,end));
		} else if(postRex[i] == '*') {
			State *start = new State(),*end = new State();
			end->isAccept = true;
			start->outE[0] = end;

			NFA n = nfaStack.top(); nfaStack.pop();
			n.end->isAccept = false;
			n.end->outE[0] = n.start;  //每个 NFA 终态无出边，所以对于 n.end 的 outE 直接使用 0 索引第一个就可

			start->outE[1] = n.start;
			n.end->outE[1] = end;			

			nfaStack.push(NFA(start,end));
		} else {
			//其他正则表达式运算符构造 NFA 也都大抵相似，重复性的就不做了
			cout << "unconsiderd for now...\n";
		}
	}
	// cout << nfaStack.size() << endl;;
	return nfaStack.top();
}


/*
这里使用 '.' 代表 连接 符号
优先级关系从高到低："*?+" > '.' > '|' > '('
*/

/* 例子
a.b|c.d*		ab.cd*.|
(a|b)*.a.b.b    ab|*a.b.b.
((a|b)*.c)*|p   结果正确
*/
string getPost(const string& regExp) {
	string resStr;
	stack<char> opStack;
	map<char,int> pri;

	//初始化优先级
	pri.insert(pair<char,int>('*',3));
	pri.insert(pair<char,int>('?',3));
	pri.insert(pair<char,int>('+',3));
	pri.insert(pair<char,int>('.',2));
	pri.insert(pair<char,int>('|',1));
	pri.insert(pair<char,int>('(',0));

	for(int i = 0; i < regExp.length(); ++i) {
		char item = regExp.at(i);
		if(isalpha(item)) {						//字母表中字母直接取出(这里认为字母表中都是字母,用 isalpha等效)
			resStr.push_back(item);
		} else if(!opStack.empty()) {			//item 是操作符,且栈不空
			char top = opStack.top();
			if(item == ')') {								//右括号，将右括号和左括号之间的符号弹出.
				//ERROR										//右括号没有优先级，必须第一个处理
				if(opStack.empty()) {
					cout << "ERROR: opStack is empty\n";
					return nullptr;
				}
				//

				while(!opStack.empty() && top != '(') {
					opStack.pop();
					resStr.push_back(top);
					top = opStack.top();
				}
				opStack.pop();			//把左括号出栈
			} else if(pri[item] > pri[top] || item == '(') {					
				opStack.push(item);
			} else if(pri[item] <= pri[top]) {							//item <= top,pop
				while(pri[item] <= pri[top]) {
					resStr.push_back(top);
					opStack.pop();
					if(!opStack.empty()) {
						top = opStack.top();
					} else break;
				}
				opStack.push(item);		
			}
		} else if(item != ')') {		//符号栈为空。）不会 被 push 
			opStack.push(item);
		}
	}

	while(!opStack.empty()) {
		resStr.push_back(opStack.top());
		opStack.pop();
	}

	return resStr;
}
