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
        err_exit("Code should be written within ", LIN_SIZ, " characters per one line", "");
    if (++srcLineno > MAX_LINE)
        err_exit("Code exceeded ", MAX_LINE, " (s).", "");

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
            kd = TknKind::IntNum;
            while(ctyp[CH] == Digit){ txt += CH; NEXT_CH(); }
            if (CH == '.') {kd = TknKind::DblNum; txt += CH; NEXT_CH(); }
            while(ctyp[CH] == Digit){ txt += CH; NEXT_CH(); }
            return Token(kd, txt, atof(txt.c_str()));

        case DblQ:
            NEXT_CH();
            while(CH != '\0' && CH != '"'){ txt += CH; NEXT_CH(); }
            if (CH != '"') NEXT_CH(); else err_exit("No closure for string litteral\n", "", "", "");
            return Token(TknKind::String, txt);

        default:
            if (CH == '/' && C2 == '/') return Token(TknKind::EofLine);
            if (is_ope2(CH, C2)){
                txt += CH;
                txt += C2;
                NEXT_CH();
                NEXT_CH();
            } else {
                txt += CH;
                NEXT_CH();
            }
    }
    kd = get_kind(txt);

    if (kd == TknKind::Others) err_exit("Wrong token: ", txt, "", "");
    return Token(kd, txt);
}

bool is_ope2(int c1, int c2){
    char s[] = "  ";
    if (c1 == '\0' || c2 == '\0') return false;
    s[1] = c1;
    s[2] = c2;

    return strstr(" ++ -- <= >= == != && || ", s) != NULL;
}

TknKind get_kind(const string& s){
    for (int i=0; keyWdTbl[i].keyKind != TknKind::END_Keylist; ++i){
        if (s == keyWdTbl[i].keyName) return keyWdTbl[i].keyKind;
    }
    if (ctyp[ s[0] ] == TknKind::Letter || ctyp[ s[0] ] == TknKind::Doll) return TknKind::Ident;
    if (ctyp[ s[0] ] == Digit ) return TknKind::DblNum;
    return TknKind::Others;
}

Token chk_nextTkn(const Token& tk, int kind2){
    if (tk.kind != kind2) err_exit(err_msg(tk.text, kind_to_s(kind2)), "", "", "");
    return nextTkn();
}

void set_token_p(char *p){
    token_p = p;
}

string kind_to_s(int kd){
    for (int i=0; ; ++i){
        if (keyWdTbl[i].keyKind == TknKind::END_Keylist) break;
        if (keyWdTbl[i].keyKind == kd) return keyWdTbl[i].keyName;
    }
    return "";
}

string kind_to_s(const CodeSet& cd){
    switch (cd.kind){
    case TknKind::Lvar:
    case TknKind::Gvar:
    case TknKind::Fcall:
        return tableP(cd)->name;
    
    case TknKind::IntNum:
    case TknKind::DblNum:
        return dbl_to_s(cd.dblVal);

    case TknKind::String:
        return string("\"") + cd.text + "\"";

    case TknKind::EofLine:
        return "";
    }
    return kind_to_s(cd.kind);
}

int get_lineNo(){
    extern int Pc;
    return (Pc == -1) ? srcLineno : Pc;
}