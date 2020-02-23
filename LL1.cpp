#include<iostream>
#include <map>
#include <stack>
#include <set>
#include <vector>
#include <ctype.h>
#include <iomanip>

using namespace std;

struct Entry {										                                  //终结符 和 非终结符
	string str;									                                    	//该终结符或者非终结符的字符表示。比如生成式：E-> TG|U ;则 str 为："E"
	Entry(string str) { this->str = str; }
	bool isTerm = false;							                                //是否为终结符

	bool operator < (const Entry& rhs) const {		                    //map 所需的弱序关系
		return this->str < rhs.str;
	}

	bool operator == (const Entry& rhs) const {
		return this->str == rhs.str;
	}
};

struct RightExp {									                                  //一个产生式的 右侧表达式
	string str;										                                    //产生式右侧表达式的 字符串表示。比如生成式：E-> TG|U ;则 str 为："TG|U"
	vector<vector<Entry >> expList; 		                          		//右侧表达式中每出现一个 | 运算，就增加一个 exp。比如：E-> TG|U；exp1:TG,exp2:U ;
									 				                                          //一个 exp 是一个 Entry list
};

map<Entry,RightExp> generate;					                             	//产生式左侧和右侧的对应关系	即符号："->"
set<string> tsTable({"+","-","/","*",")","(","e","i"});				  		//终结符号表 termination symbol table
map<Entry,set<Entry>> simpleFirst;											            //单个 非终结符 的 first 集
map<Entry,set<Entry>> simplefollow;											            //单个 非终结符 的 follow 集
map<pair<Entry,string>,vector<vector<Entry>>> pdt;					      	//predictionTable 预测分析表。第一个pair保存 M[A,a],作为一个键,第二个,vector<vector>,表示M[A.a]对应的产生式集。
																			                              //标准的LL(1)是不允许一个表项具有多重定义的（M[A,a]有多个产生式），但是如果通过合理的定义是可以允许这种情况的。所以这里使用产生式列表：vector<vector>,而不是vector
void input();
void inputTest();
void computeFirstSet();
void addFirstSet(const Entry& a,set<Entry> B,bool) ;
void traverseU(const Entry& u);
void computeFirstTest() ;
vector<string> slice(const string& resStr,const string& del);
bool addSet(const Entry& a,set<Entry> B,bool epsilonIn,bool first=true);
set<Entry> computeFirstBeta(const vector<Entry> &u,vector<Entry>::reverse_iterator ic);
void computeFollowSet();
void computeFollowTest();
void computeFirstBetaTest(const vector<Entry> &u,vector<Entry>::reverse_iterator ic,set<Entry> firstBeta);
set<Entry> computeFirstBeta(vector<Entry> &u);
void constructPredictTable();
void fillTable(const Entry &leftPart,const vector<Entry> &exp,set<Entry> oneSet);
void pdtTest();
void LL1();

int main(int argc, char const *argv[])
{
	freopen("input.txt","r",stdin);
	input();
	inputTest();
	computeFirstSet();
	computeFirstTest();
	cout << endl;

	computeFollowSet();
	computeFollowTest();
	cout << endl;

	constructPredictTable();
	pdtTest();
	cout << endl;

	LL1();
	cout << "exited normaly";
	return 0;
}

/*
	对于终结符，在输入时要求将其用单引号括起来表示。(此为常见一般性要求，此程序不需要)
	每次处理一行，所以一个产生式不能换行。另外，一个非终结符只能存在于一个产生式的左侧。
	和以前一样，连接运算使用"."表示。例子：E -> T.G|U.F.P。
	这里的文法输入默认是已经进行过 消除左递归、提取公因子的文法。
	对于每个终结符认为是单字符。每个非终结符可以是一个串，但是测试只使用一个大写字母。并且时间紧迫，
	可能不会对非终结符是串的情况进行正确性测试。
*/
void input() {														                                    	//对输入的产生式建模
	string lineBuf;

	while(getline(cin,lineBuf),!cin.eof()) {							                        //每次处理一行，所以一个产生式不能换行
		int border = lineBuf.find("->");								                            //查找第一个 "->"
		if(border == -1) break;											                                //如果输入不是产生式，结束输入
		string leftpart = lineBuf.substr(0,border-0);
		string rightPart = lineBuf.substr(border+2);

		Entry leftues(leftpart);
		if(generate.find(leftues) == generate.end()) {				                      	//generate 中没有该结符号
			generate.insert(pair<Entry,RightExp>(leftues,RightExp()));	                //插入新的
		}
		vector<string> explist = slice(rightPart,"|");					                      //用 | 运算符分割产生式右侧串

		RightExp re;													                                      //新建一个 RightExp，构造完成后传给 generate。产生式的终结符和他的右侧就建立了联系
		re.str = rightPart;
		for(int i = 0; i < explist.size(); ++i) {				                              //例子：["ues1.ues2","ues3./.ues4","ues5.+.ues7"]
			vector<Entry> exp;
			string word = explist[i];							                                      //在这一步主要识别出上例中终结符（+,-,*,/)等,和非终结符。并对非终结符进行一些操作
			vector<string> expArr = slice(word,".");			                              //将单个的终结符或者非终结符分离出来。例：["ues1","ues2"]
			for(auto it : expArr) {
				if(tsTable.count(it) != 0) {
					Entry e(it);
					e.isTerm = true;
					exp.push_back(e);
				} else {										                                                //不是我们规定的终结符,认为他是一个非终结符
					Entry e(it);
					exp.push_back(e);
				}
			}
			re.expList.push_back(exp);
		}
		generate[leftues] = re;
	}
}

/*
	计算 first 集
*/
void computeFirstSet() {
	//初始化。所有终结符的 first 集为其本身
	for(auto it = tsTable.begin(); it != tsTable.end(); ++it) {
		Entry e = Entry(*it);
		simpleFirst.insert(pair<Entry,set<Entry>>(e,set<Entry>({e})));
	}

	//求 generate 中每个终结符的 first 集
	for(auto it = generate.begin(); it != generate.end(); ++it) {
		Entry leftEntry = it->first;
		if(simpleFirst.find(leftEntry) == simpleFirst.end()) {			                    //该终结符的 first 集没有被计算过
			traverseU(leftEntry);
		}
	}
}

void traverseU(const Entry& u) {										                                  //计算 u 这一非终结符的 first 集
	RightExp re = generate[u];
	vector<vector<Entry>> explist = re.expList;
	int i;
	for(i = 0; i < explist.size(); ++i) {							//右部的每个连接运算‘|’，都将产生两个exp，每个exp 的first集都是产生该右部的非终结符的first集
		vector<Entry> exp = explist.at(i);
		for(int j = 0; j < exp.size(); ++j) {							//计算该 exp 的first集（具体算法见龙书计算first集）
			if(simpleFirst.find(exp.at(j)) != simpleFirst.end()) {	    //该（非）终结符的first集已经计算过
				set<Entry> firstJ = simpleFirst[exp.at(j)];				//该（非）终结符的first集
				if(firstJ.count(Entry("e"))) {							//如果包含 epsilon 
					if(firstJ.size() == 1) 
						addSet(u,firstJ,true);						//此种情况为 E->epsilon,将 epsilon 加入 first(E)
					else 
						addSet(u,firstJ,false);		
				} else {												//firstJ集中无 epsilon，这个exp上的first集求解完毕
					addSet(u,firstJ,false);
					break;
				}
			} else {													//该（非）终结符的first集没有计算过，递归计算他
				traverseU(exp.at(j));
				j--;													//为了处理这个计算过first集的（非）终结符,让 j 再次指向他
			}
		}
	}
	if(i == explist.size()+1 && explist.size() > 1) 					//此种情况为所有的exp都包含epsilon，所以该产生式可以推导出epsilon，所以将epsilon加入first集
		addSet(u,set<Entry>({Entry("e")}),true);
}

/*
	计算 follow 集
*/
void computeFollowSet() {
	simplefollow[generate.begin()->first].insert(Entry("$"));				//起始 非终结符后接 结束符 $
	bool hasMore = false;													//是否还能向某个follow集中添加
	do {
		hasMore = false;
		for(auto it = generate.begin(); it != generate.end(); ++it) {		//遍历所有的产生式
			Entry leftEntry = it->first;									//一个产生式的左部非终结符
			vector<vector<Entry>> explist = it->second.expList;
			for(auto ip = explist.begin(); ip != explist.end(); ++ip) {		//遍历该产生式的所有 exp.例子：["TG","PE"]
				for(auto ic = ip->rbegin(); ic != ip->rend(); ++ic) {		//逆序遍历一个 exp 的所有 Entry.例子：*ip: ["T","G"]
					if(ic->isTerm) continue;								//终结符不需计算follow集,只有非终结符才有follow集
					if(ic == ip->rbegin()) {								//如果是最后一个非终结符(可以认为，则该非终结符的follow集为他的父亲(推导树上）的follow集 
						if(true == addSet(*ic,simplefollow[leftEntry],false,false))	
							hasMore = true;				        			//有follow集被更新
					} else {
						set<Entry> fisrtBeta = computeFirstBeta(*ip,ic);    //ip指向一个exp。该函数计算ic后续的（非）终结符组成的串 β 的first集.firstBeta的解释：比如产生式：E-> TG|U ; firstBeta 表示符号串 "TG|U" 的 first 集
						if(true == addSet(*ic,fisrtBeta,false,false))
							hasMore = true;									//有follow集被更新
						if(fisrtBeta.count(Entry("e"))) {
							if(true == addSet(*ic,simplefollow[leftEntry],false,false))		//设有一产生式：E->αBβ，其中α和β为文法串（终结符和非终结符串），B 为非终结符。当 β 文法串可以推导出 epsilon(即 firstBeta 包含 epsilon) ，B 的紧随字母其所在的推导树层面可能为空，所以将他的父节点的 follow 集加入到他的 follow 集中。这是符合 follow 集定义的。
							hasMore = true;				        			//有follow集被更新
						}
					}
				}
			}
		}
	}while(hasMore);
}

/*
	构造预测分析表
*/
void constructPredictTable() {
	for(auto it = generate.begin(); it != generate.end(); ++it) {					//遍历每个产生式
		Entry leftPart = it->first;													//获得一个产生式左部和右部
		vector<vector<Entry>> explist = it->second.expList;							//产生式的右部
		for(auto ip = explist.begin(); ip != explist.end(); ++ip) {					//遍历每个 exp。（以“|”分隔的产生式右部）
			set<Entry> firstBeta = computeFirstBeta(*ip);							//计算产生式右部串的first集.A->β；则first(β)为所求
			fillTable(leftPart,*ip,firstBeta);
			if(firstBeta.count(Entry("e")) == 0) continue;							//如果firstBeta 中有 epsilon 则还需添加follow集

			set<Entry> followA =  simplefollow[leftPart];							//添加 follow 集
			fillTable(leftPart,*ip,followA);
			if(followA.count(Entry("$")) == 0) continue;

			fillTable(leftPart,*ip,set<Entry>({Entry("$")}));
		}
	}
}

/*
	预测分析程序
*/
void LL1() {
	string inputStr;
	cout << "Please input a grammatical string(no space): ";
	cin >> inputStr;													//输入串不能包含任何空白符
	cout << inputStr << endl;
	inputStr.push_back('$');											//在这个串的末尾添加 $ 作为结束标识
	stack<Entry> entrySt;
	entrySt.push(Entry("E"));											//压入开始非终结符。注意：整个程序，每个 Entry 的主键都是一个字符串，确定了一个字符串，那么就确定了一个 Entry

	int p = 0,len = inputStr.length();
	while(!entrySt.empty()) {
		Entry uts = entrySt.top();										//uts: unterminated symbol
		if(uts.str == "e") {											//如果选择了某个产生式：S->e,则 e 将会被加入栈中。被取出时为 uts。epsilon 表示空转换，直接跳过。
			entrySt.pop();
			continue;
		}
		if(uts.isTerm == true || uts.str == "$") {						//如果 uts 是一个终结符
			if(uts.str[0] == inputStr[p]) {								//uts 匹配 输入串的一个字符.由于程序假定终结符都是单字符，所以 uts.str[0],取第一个字符
				//debug
				// cout << "match: " << p << " " << uts.str[0] << "," << inputStr[p] << endl;
				//debug
				entrySt.pop();
				p++;
			} else {
				cout << "Parsing error1" << endl;
				return ;
			}
		} else {														//uts 为非终结符
			pair<Entry,string> key = pair<Entry,string>(uts,inputStr.substr(p,1));				//pair 中 Entry 和 string 分别对于 M[A,a] 中的非终结符 A 和终结符 a。设计程序之初考虑终结符可能也是一个串，但是目前都是用一个单字符来处理的。这里由于 C++ 的限制，需要用 substr 来将一个单字符转换为字符串
			if(pdt.find(key) != pdt.end()) {							// 如果 M[A,a] 项不为空
				entrySt.pop();
				vector<Entry> entrys = pdt[key][0];						//返回 第一个 vector，即第一个产生式。(当输入的文法为标准LL1 文法时，只有第一个 vector 不为空)
				for(auto ip = entrys.rbegin(); ip != entrys.rend(); ++ip) {
					entrySt.push(*ip);
				}

				//output 
				stack<Entry> tmpSt(entrySt);
				while(tmpSt.empty() != true) {
					cout << tmpSt.top().str;tmpSt.pop();
				}
				cout << "   |   ";
				cout << inputStr.c_str() + p << "   |   ";
				//output 

				//debug 输出产生式
				cout << uts.str << "->";
				for(auto ip = entrys.begin(); ip != entrys.end(); ++ip) {
					cout << ip->str;
				}
				cout << endl;
				//debug 输出产生式
			} else {
				cout << "Parsing error2" << endl;
				//debug 
				// cout << key.first.str << " " << key.second << endl;
				// cout << "->" << p << inputStr[p] << endl;
				//debug 
				return ;
			}
		}
	}
	cout << "Parsing finished" << endl;
}

/* 
	工具函数
*/
//	根据给定分割串分割一个原始串,并返回分割后结果(没时间去找C++正则，还是写个算法快:P)
vector<string> slice(const string& resStr,const string& del) {
	vector<string> arr;
	int start = 0;
	int border = resStr.find(del,start);
	while(border != string::npos) {
		arr.push_back(resStr.substr(start,border-start));
		start = border + del.length();
		border = resStr.find(del,start);
	}
	arr.push_back(resStr.substr(start));
	return arr;
}

//将集合B的元素加入到a的集合A中.epsilonIn 表示是否包含 epsilon
bool addSet(const Entry& a,set<Entry> B,bool epsilonIn,bool first) {
	bool res = false;
	set<Entry> *A;
	if(first) A = &simpleFirst[a];
	else A = &simplefollow[a];
	for(auto it = B.begin(); it != B.end(); ++it) {
		if(it->str == "e" && epsilonIn)
			A->insert(*it);
		else if(it->str != "e") {
			if(A->count(*it) == 0) {
				res = true;
				A->insert(*it);
			}
		}
	}
	return res;
}

set<Entry> computeFirstBeta(const vector<Entry> &u,						    //u 是一个exp。该函数计算ic后续的（非）终结符组成的串 β 的first集
	vector<Entry>::reverse_iterator ic) {								//注意 ic 是一个反向迭代器.

	set<Entry> firstBeta;
	ic--;																//从指向的（非）终结符的下一个开始（ic-- 在一个数组上体现是向数组末端移动）
	while(ic != u.rbegin()-1) {											//从 u 到最后一个entry,对应龙书：first1，first2，...，firsti
		set<Entry> firsti = simpleFirst[*ic];


		for(auto ip = firsti.begin(); ip != firsti.end(); ++ip) {
			if(ip->str == "e") continue;
			firstBeta.insert(*ip);
		}

		if(firsti.count(Entry("e")) == 0) {									//如果有一个（非）终结符的first集含有 epsilon。那么就得继续查看下一个first集。如果有一个first集不包含epsilon，将这个集合加入firstBeta，并返回。
			//test
			// computeFirstBetaTest(u,ic,firstBeta);
			//test
			return firstBeta;
		}
		ic--;
	}
	firstBeta.insert(Entry("e"));
	//test
	// computeFirstBetaTest(u,ic,firstBeta);
	//test
	return firstBeta;
}

set<Entry> computeFirstBeta(vector<Entry> &u) {						//computeFirstBeta 的适配器。（c++ 函数重载还可以实现适配器 OVO）
	return computeFirstBeta(u,u.rend());
}

//
void fillTable(const Entry &leftPart,const vector<Entry> &exp,set<Entry> oneSet) {

	for(auto ic = oneSet.begin(); ic != oneSet.end(); ++ic) {
		if(ic->str == "e") continue;												//epsilon 不处理
		//M[A,a] = 产生式
		pair<Entry,string> key = pair<Entry,string> (leftPart,ic->str);
		vector<vector<Entry>> value;
		if(pdt.count(key) != 0) {													
			value = pdt[key];
			if(value.size() > 0) {													//M[A,a] 已有表项，意味着现在有两个了(二义性)
				if(value.size() > 1) {												//当重复项为两个时我们也许还有通过指定规则来消除二义性的可能，但是超过两个，暂时就不考虑了
					cout << "found a ambiguous item. (Failed)\n";
				} else if(value.size() == 1){
					cout << "found a ambiguous item. (need process)\n";
					if(value[0] == exp) continue;									//pass 如果 *ip 和 value[0] 相等，那就不必插入.(使用 vector 中的 Entry 的 == 重载比较每个元素)
				}
			}
			value.push_back(vector<Entry> (exp));									//将一个 exp 即产生式右部,放到 M[A,a]中
			pdt[key] = value;														//更新 pdt
		} else {
			value.push_back(vector<Entry> (exp));									//将一个 exp 即产生式右部,放到 M[A,a]中
			pdt[key] = value;														//更新 pdt
		}
	}
}

/* 工具函数 */


/* 
	测试函数
*/
void inputTest() {
	cout << "input Test: \n";
	for(auto it = generate.begin(); it != generate.end(); ++it) {
		cout << it->first.str << "->";
		vector<vector<Entry>> explist = it->second.expList;
		for(auto itt = explist.begin(); itt != explist.end(); ++itt) {
			vector<Entry> exp = *itt;
			for(auto ittt = exp.begin(); ittt != exp.end(); ++ittt) {
				cout << ittt->str;
			}
			if(itt < explist.end()-1)
				cout << "|";
		}
		cout << endl;
	}
	cout << endl;
}


//查看 first 集
void computeFirstTest() {
	for(auto it = simpleFirst.begin(); it != simpleFirst.end(); it++) {
		if(tsTable.count(it->first.str) != 0) continue;
		cout << "first(" << it->first.str << "): { ";
		set<Entry> sf = it->second;
		for(auto itt = sf.begin(); itt != sf.end(); itt++) {
			cout << itt->str << ", ";
		}
		cout << " }" << endl;
	}
}

//查看 follow 集
void computeFollowTest() {
	for(auto it = simplefollow.begin(); it != simplefollow.end(); it++) {
		cout << "follow(" << it->first.str << "): { ";
		set<Entry> sf = it->second;
		for(auto itt = sf.begin(); itt != sf.end(); itt++) {
			cout << itt->str << ", ";
		}
		cout << " }" << endl;
	}
}

//输出一个 firstBeta 的内容
void computeFirstBetaTest(const vector<Entry> &u,
	vector<Entry>::reverse_iterator ic,set<Entry> firstBeta) {
	cout << "Test firstBeta: \n";
	for(;ic != u.rbegin()-1; --ic) {
		cout << ic->str;
	}
	cout << " " ;
	for(auto it = firstBeta.begin(); it != firstBeta.end(); ++it) {
		cout << it->str;
	}
	cout << endl;
	return ;
}

void pdtTest() {
	cout << "predictionTable: " << endl;
	for(auto it = pdt.begin(); it != pdt.end(); ++it) {
		cout << "[" << it->first.first.str << "," << it->first.second << "]: ";
		vector<vector<Entry>> vv = it->second;
		for(auto ip = vv.begin(); ip != vv.end(); ++ip) {
			cout << it->first.first.str << "->" ;
			for(auto ic = ip->begin(); ic != ip->end(); ++ic) {
				cout << ic->str;
			}
			cout << "; ";
		}
		cout << endl << endl;
	}
}

/* 测试方法 */
