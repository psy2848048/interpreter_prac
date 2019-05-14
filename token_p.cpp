#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <cctype>

using namespace std;

enum TknKind {
	Lparen=1, Rparen, Plus, Minus, Multi, Divi,
	Assign, Comma, DblQ,
	Equal, NotEq, Less, LessEq, Great, GreatEq,
	If, Else, End, Print, Ident, IntNum,
	String, Letter, Digit, EofTkn, Others, End_list
};

struct Token {
	TknKind kind;
	string text;
	int intVal;
	Token() { kind=Others; text=""; intVal=0;}

	Token (TknKind k, const string& s, int d=0){
		kind = k; text = s; intVal=d;	
	}
};

/* Prototype */
void initChTyp();
Token nextTkn();
int nextCh();
bool is_ope2(int c1, int c2);
TknKind get_kind(const string& s);
/************/

TknKind ctyp[256];
Token token;
ifstream fin;

struct KeyWord {
	const char* keyName;
	TknKind keyKind;
};

KeyWord keyWdTbl[] = {
	{"if", If}, {"else", Else},
	{"end", End}, {"print", Print},
	{"(", Lparen}, {")", Rparen},
	{"+", Plus}, {"-", Minus},
	{"*", Multi}, {"/", Divi},
	{"=", Assign}, {",", Comma},
	{"==", Equal}, {"!=", NotEq},
	{"<", Less}, {"<=", LessEq},
	{">", Great}, {">=", GreatEq},
	{"", End_list}
};

int main(int args, char *argv[]){
	if (argc == 1) exit(1);
	fin.open(argv[1]); if (!fin) exit(1);

	cout << "Text        kind intVal\n";
	initChTyp();
	for (token = nextTkn(); token.kind != EofTkn; token = nextTkn())
}
