#include "bbi.h"
//#include "bbi_prot.h"

struct KeyWord {
	const char* keyName;
	TknKind keyKind;
};

KeyWord keyWdTbl[] = {
    {"func", Func}, {"var", Var},
	{"if", If}, {"elif", Elif},
    {"else", Else}, {"for", For},
    {"to", To}, {"step", Step},
    {"while", While}, {"end", End},
    {"break", Break}, {"return", Return},
	{"print", Print}, {"println", Println},
    {"option", Option}, {"input", Input},
    {"toint", Toint}, {"exit", Exit},
	{"(", Lparen}, {"}", Rparen},
	{"[", Lbracket}, {"]", Rbracket},
	{"+", Plus}, {"-", Minus},
	{"*", Multi}, {"/", Divi},
	{"==", Equal}, {"!=", NotEq},
	{"<", Less}, {"<=", LessEq},
	{">", Great}, {">=", GreatEq},
	{"&&", And}, {"||", Or},
	{"!", Not}, {"%", Mod},
	{"?", Ifsub}, {"=", Assign},
    {"\\", IntDivi}, {",", Comma},
    {"\"", DblQ},
	{"@dummy", END_Keylist}
};

int srcLineno;
TknKind ctyp[256];
char *token_p;
bool endOfFile_F;
char buf[LIN_SIZ + 5];
ifstream fin;
#define MAX_LINE 2000

void initChTyp(){
    int i;

    for (i=0; i < 256; ++i) { ctyp[i] = Others; }
    for (i='0'; i <= '9'; ++i) { ctyp[i] = Digit; }
    for (i='A'; i <= 'Z'; ++i) { ctyp[i] = Letter; }
    for (i='a'; i <= 'z'; ++i) { ctyp[i] = Letter; }
    ctyp['('] = Lparen; ctyp[')'] = Rparen;
    ctyp['<'] = Less; ctyp['>'] = Great;
    ctyp['+'] = Plus; ctyp['-'] = Minus;
    ctyp['*'] = Multi; ctyp['/'] = Divi;
    ctyp['_'] = Letter; ctyp['='] = Assign;
    ctyp[','] = Comma; ctyp['"'] = DblQ;
}

///// 20190517 //////


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

int main(int argc, char *argv[]){
	if (argc == 1) exit(1);
	fin.open(argv[1]); if (!fin) exit(1);

	cout << "Text        kind intVal\n";
	initChTyp();
	for (token = nextTkn(); token.kind != EofTkn; token = nextTkn()){
        cout << left << setw(10) << token.text << right << setw(3) 
            << token.kind << " " << token.intVal << endl;
    }

    return 0;
}

Token nextTkn(){
    TknKind kd;
    int ch0, num = 0;
    static int ch = ' ';
    string txt = "";

    while (isspace(ch)) { ch = nextCh(); }
    if (ch == EOF) return Token(EofTkn, txt);

    switch (ctyp[ch]){
        case Letter:
            for ( ; ctyp[ch] == Letter || ctyp[ch] == Digit; ch = nextCh() ) txt += ch;
            break;

        case Digit:
            for (num=0; ctyp[ch] == Digit; ch = nextCh()){
                num = num*10 + (ch - '0');
            }
            return Token(IntNum, txt, num);

        case DblQ:
            for (ch=nextCh(); ch!= EOF && ch!='\n' && ch != '"'; ch = nextCh() ) txt += ch;
            if (ch != '"'){ cout << "No closure for string litteral\n"; exit(1); }
            ch = nextCh();
            return Token(String, txt);

        default:
            txt += ch; ch0 = ch; ch = nextCh();
            if (is_ope2(ch0, ch)){ txt += ch; ch = nextCh(); }
    }

    kd = get_kind(txt);
    if (kd == Others){
        cout << "Wrong token: " << txt << endl;
        exit(1);
    }

    return Token(kd, txt);
}

int nextCh(){
    static int c = 0;
    if (c == EOF) return c;
    if ( (c=fin.get() ) == EOF) fin.close();
    return c;
}

bool is_ope2(int c1, int c2){
    char s[] = "  ";
    if (c1 == '\0' || c2 == '\0') return false;
    s[1] = c1;
    s[2] = c2;

    return strstr(" <= >= == != ", s) != NULL;
}

TknKind get_kind(const string& s){
    for (int i=0; keyWdTbl[i].keyKind != End_list; ++i){
        if (s == keyWdTbl[i].keyName) return keyWdTbl[i].keyKind;
    }
    if (ctyp[ s[0] ] == Letter ) return Ident;
    if (ctyp[ s[0] ] == Digit ) return IntNum;

    return Others;
}
