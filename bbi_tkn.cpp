#include "bbi.hpp"
#include "bbi_prot.hpp"

struct KeyWord {
	const char* keyName;
	TknKind keyKind;
};

KeyWord keyWdTbl[] = {
    {"func", TknKind::Func}, {"var", TknKind::Var},
	{"if", TknKind::If}, {"elif", TknKind::Elif},
    {"else", TknKind::Else}, {"for", TknKind::For},
    {"to", TknKind::To}, {"step", TknKind::Step},
    {"while", TknKind::While}, {"end", TknKind::End},
    {"break", TknKind::Break}, {"return", TknKind::Return},
	{"print", TknKind::Print}, {"println", TknKind::Println},
    {"option", TknKind::Option}, {"input", TknKind::Input},
    {"toint", TknKind::Toint}, {"exit", TknKind::Exit},
	{"(", TknKind::Lparen}, {"}", TknKind::Rparen},
	{"[", TknKind::Lbracket}, {"]", TknKind::Rbracket},
	{"+", TknKind::Plus}, {"-", TknKind::Minus},
	{"*", TknKind::Multi}, {"/", TknKind::Divi},
	{"==", TknKind::Equal}, {"!=", TknKind::NotEq},
	{"<", TknKind::Less}, {"<=", TknKind::LessEq},
	{">", TknKind::Great}, {">=", TknKind::GreatEq},
	{"&&", TknKind::And}, {"||", TknKind::Or},
	{"!", TknKind::Not}, {"%", TknKind::Mod},
	{"?", TknKind::Ifsub}, {"=", TknKind::Assign},
    {"\\", TknKind::IntDivi}, {",", TknKind::Comma},
    {"\"", TknKind::DblQ},
	{"@dummy", TknKind::END_Keylist}
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

    for (i=0; i < 256; ++i) { ctyp[i] = TknKind::Others; }
    for (i='0'; i <= '9'; ++i) { ctyp[i] = TknKind::Digit; }
    for (i='A'; i <= 'Z'; ++i) { ctyp[i] = TknKind::Letter; }
    for (i='a'; i <= 'z'; ++i) { ctyp[i] = TknKind::Letter; }
    ctyp['('] = TknKind::Lparen; ctyp[')'] = TknKind::Rparen;
    ctyp['<'] = TknKind::Less; ctyp['>'] = TknKind::Great;
    ctyp['+'] = TknKind::Plus; ctyp['-'] = TknKind::Minus;
    ctyp['*'] = TknKind::Multi; ctyp['/'] = TknKind::Divi;
    ctyp['_'] = TknKind::Letter; ctyp['='] = TknKind::Assign;
    ctyp[','] = TknKind::Comma; ctyp['"'] = TknKind::DblQ;
}

void fileOpen(char* fname){
    srcLineno = 0;
    endOfFile_F = false;
    fin.open(fname);
    if (!fin){
        cout << "Cannot open " << fname << "\n";
        exit(1);
    }
}

void nextLine(){
    string s;

    if (endOfFile_F) return;

    fin.getline(buf, LIN_SIZ+5);
    if (fin.eof()){
        fin.clear();
        fin.close();
        endOfFile_F = true;
        return;
    }
    if (strlen(buf) > LIN_SIZ)
        err_exit("Code should be written within ", LIN_SIZ, " characters per one line");
    if (++srcLineno > MAX_LINE)
        err_exit("Code exceeded ", MAX_LINE, " (s).");

    token_p = buf;
}

Token nextLine_tkn(){
    nextLine();
    return nextTkn();
}

#define CH (*token_p)
#define C2 (*(token_p+1))
#define NEXT_CH() ++token_p

Token nextTkn(){
    TknKind kd;
    string txt = "";
    
    if (endOfFile_F) return Token(TknKind::EofProg);
    while (isspace(CH)) NEXT_CH();
    if (CH == '\0') return Token(TknKind::EofLine);

    switch (ctyp[CH]){
        case Doll:
        case Letter:
            txt += CH; NEXT_CH();
            while (ctyp[CH] == Letter || ctyp[CH] == Digit){
                txt += CH; NEXT_CH();
            }
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

///// 20190517 //////

/* Prototype */
void initChTyp();
Token nextTkn();
int nextCh();
bool is_ope2(int c1, int c2);

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
