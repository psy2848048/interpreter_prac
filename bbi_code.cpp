#include "bbi.hpp"
#include "bbi_prot.hpp"

CodeSet code;
int startPc;
int Pc = -1;
int baseReg;
int spReg;
int maxLine;
vector<char*> intercode;
char *code_ptr;
double returnValue;
bool break_Flg, return_Flg, exit_Flg;
Mymemory Dmem;
vector<string> strLITERAL;
vector<double> nbrLITERAL;
bool syntaxChk_mode = false;
extern vector<SymTbl> Gtable;

class Mystack {
    private:
        stack<double> st;

    public:
        void push(double n){
            st.push(n);
        }
        int size(){
            return (int)st.size();
        }
        bool empty(){
            return st.empty();
        }
        double pop(){
            if (st.empty()) err_exit("Stack underflow", "", "", "");
            double d = st.top();
            st.pop();
            return d;
        }
};
Mystack stk;

void syntaxChk(){
    syntaxChk_mode = true;
    for (Pc = 1; Pc < (int)intercode.size(); ++Pc){
        code = firstCode(Pc);
        switch (code.kind){
        case TknKind::Func:
        case TknKind::Option:
        case TknKind::Var:
            break;

        case TknKind::Else:
        case TknKind::End:
        case TknKind::Exit:
            code = nextCode();
            chk_EofLine();
            break;

        case TknKind::If:
        case TknKind::Elif:
        case TknKind::While:
            code = nextCode();
            (void) get_expression(0, TknKind::EofLine);
            break;

        case TknKind::For:
            code = nextCode();
            (void)get_memAdrs(code);
            (void)get_expression('=', 0);
            (void)get_expression(TknKind::To, 0);
            if (code.kind == TknKind::Step) (void)get_expression(TknKind::Step, 0);
            chk_EofLine();
            break;

        case TknKind::Fcall:
            fncCall_syntax( (TknKind)code.symNbr );
            chk_EofLine();
            (void)stk.pop();
            break;

        case TknKind::Print:
        case TknKind::Println:
            sysFNcExec_syntax(code.kind);
            break;

        case TknKind::Gvar:
        case TknKind::Lvar:
            (void)get_memAdrs(code);
            (void)get_expression('=', TknKind::EofLine);
            break;

        case TknKind::Return:
            code = nextCode();
            if (code.kind != '?' && code.kind != TknKind::EofLine) (void)get_expression();
            if (code.kind == '?') (void)get_expression('?', 0);
            chk_EofLine();
            break;

        case TknKind::Break:
            code = nextCode();
            if (code.kind == '?') (void)get_expression('?', 0);
            chk_EofLine();
            break;

        case TknKind::EofLine:
            break;
        
        default:
            err_exit("Wrong kind of techset", kind_to_s(code.kind), "", "");
        }
    }

    syntaxChk_mode = false;
}

void set_startPc(int n){

}

void execute(){
    baseReg = 0;
    spReg = Dmem.size;
    Dmem.resize(spReg + 1000);
    break_Flg = return_Flg = exit_Flg = false;

    Pc = startPc;
    maxLine = intercode.size() - 1;
    while (Pc <= maxLine && !exit_Flg){
        statement();
    }
    Pc = -1;
}

void statement(){
    CodeSet save;
    int top_line, end_line, varAdrs;
    double wkVal, endDt, stepDt;

    if (Pc > maxLine || exit_Flg) return;
    code = save = firstCode(Pc);
    top_line = Pc;
    end_line = code.jmpAdrs;
    if (code.kind == TknKind::If) end_line = endline_of_If(Pc);

    switch (code.kind){
    case TknKind::If:
        if (get_expression(TknKind::If, 0)){
            ++Pc;
            block();
            Pc = end_line + 1;
            return;
        }
        Pc = save.jmpAdrs;

        while (lookCode(Pc) == TknKind::Elif){
            save = firstCode(Pc);
            code = nextCode();
            if (get_expression()){
                ++Pc;
                block();
                Pc = end_line + 1;
                return;
            }
            Pc = save.jmpAdrs;
        }
        if (lookCode(Pc) == TknKind::Else){
            ++Pc;
            block();
            Pc = end_line + 1;
            return;
        }
        ++Pc;
        break;

    case TknKind::While:
        for (;;){
            if (!get_expression(TknKind::While, 0)) break;
            ++Pc;
            block();
            if (break_Flg || return_Flg || exit_Flg){
                break_Flg = false;
                break;
            }
            Pc = top_line;
            code = firstCode(Pc);
        }
        Pc = end_line + 1;
        break;

    case TknKind::For:
        save = nextCode();
        varAdrs = get_memAdrs(save);

        expression('=', 0);
        set_dtTyp(save, DBL_T);
        Dmem.set(varAdrs, stk.pop());

        endDt = get_expression(TknKind::To, 0);

        if (code.kind == TknKind::Step) stepDt = get_expression(TknKind::Step, 0);
        else stepDt = 1.0;
        for (;; Pc = top_line){
            if (stepDt >= 0){
                if (Dmem.get(varAdrs) > endDt) break;
            } else {
                if (Dmem.get(varAdrs) < endDt) break;
            }
            ++Pc;
            block();
            if (break_Flg || return_Flg || exit_Flg){
                break_Flg = false;
                break;
            }
            Dmem.add(varAdrs, stepDt);
        }
        Pc = end_line + 1;
        break;

    case TknKind::Fcall:
        fncCall(code.symNbr);
        (void)stk.pop();
        ++Pc;
        break;

    case TknKind::Func:
        Pc = end_line + 1;
        break;

    case TknKind::Print:
    case TknKind::Println:
        sysFncExec(code.kind);
        ++Pc;
        break;

    case TknKind::Gvar:
    case TknKind::Lvar:
        varAdrs = get_memAdrs(code);
        expression('=', 0);
        set_dtTyp(save, DBL_T);
        Dmem.set(varAdrs, stk.pop());
        ++Pc;
        break;

    case TknKind::Return:
        wkVal = returnValue;
        code = nextCode();
        if (code.kind != '?' && code.kind != TknKind::EofLine)
            wkVal = get_expression();
        post_if_set(return_Flg);
        if (return_Flg) returnValue = wkVal;
        if (!return_Flg) ++Pc;

    case TknKind::Break:
        code = nextCode();
        post_if_set(break_Flg);
        if (!break_Flg) ++Pc;
        break;

    case TknKind::Exit:
        code = nextCode();
        exit_Flg = true;
        break;

    case TknKind::Option:
    case TknKind::Var:
    case TknKind::EofLine:
        ++Pc;
        break;
    
    default:
        err_exit("Wrong description: ", kind_to_s(code.kind), "", "");
        break;
    }
}

void block(){
    TknKind k;
    while (!break_Flg && !return_Flg && !exit_Flg){
        k = lookCode(Pc);
        if (k == TknKind::Elif || k == TknKind::Else || k == TknKind::End) break;
        statement();
    }
}

double get_expression(int kind1, int kind2){
    expression(kind1, kind2);
    return stk.pop();
}

void expression(int kind1, int kind2){
    if (kind1 != 0) code = chk_nextCode(code, kind1);
    expression();
    if (kind2 != 0) code = chk_nextCode(code, kind2);
}

void expression(){
    term(1);
}

void term (int n){
    TknKind op;
    if (n == 7){
        factor();
        return;
    }
    term(n+1);
    while (n == opOrder(code.kind)){
        op = code.kind;
        code = nextCode();
        term(n+1);
        if (syntaxChk_mode){
            stk.pop();
            stk.pop();
            stk.push(1.0);
        } else {
            binaryExpr(op);
        }
    }
}

void factor(){
    TknKind kd = code.kind;

    if (syntaxChk_mode){
        switch (kd){
        case TknKind::Not:
        case TknKind::Minus:
        case TknKind::Plus:
            code = nextCode();
            factor();
            stk.pop();
            stk.push(1.0);
            break;

        case TknKind::Lparen:
            expression('(', ')');
            break;

        case TknKind::IntNum:
        case TknKind::DblNum:
            stk.push(1.0);
            code = nextCode();
            break;

        case TknKind::Gvar:
        case TknKind::Lvar:
            (void)get_memAdrs(code);
            stk.push(1.0);
            break;

        case TknKind::Toint:
        case TknKind::Input:
            sysFNcExec_syntax(kd);
            break;

        case TknKind::Fcall:
            fncCall_syntax((TknKind)code.symNbr);
            break;

        case TknKind::EofLine:
            err_exit("Wrong expression", "", "", "");
            break;
        
        default:
            err_exit("Wrong expression", kind_to_s(code), "", "");
            break;
        }
    }
}

int opOrder (TknKind kd){
    switch (kd){
    case TknKind::Multi:
    case TknKind::Divi:
    case TknKind::Mod:
    case TknKind::IntDivi:
        return 6;

    case TknKind::Plus:
    case TknKind::Minus:
        return 5;

    case TknKind::Less:
    case TknKind::LessEq:
    case TknKind::Great:
    case TknKind::GreatEq:
        return 4;

    case TknKind::Equal:
    case TknKind::NotEq:
        return 3;

    case TknKind::And:
        return 2;

    case TknKind::Or:
        return 1;
    
    default:
        return 0;
    }
}

void binaryExpr(TknKind op){
    double d = 0, d2 = stk.pop(), d1 = stk.pop();

    if ((op == TknKind::Divi || op == TknKind::Mod || op == TknKind::IntDivi) && d2 == 0)
        err_exit("Divided by 0", "", "", "");

    switch (op){
    case TknKind::Plus:
        d = d1 + d2;
        break;

    case TknKind::Minus:
        d = d1 - d2;
        break;

    case TknKind::Multi:
        d = d1 * d2;
        break;

    case TknKind::Divi:
        d = d1 / d2;
        break;

    case TknKind::Mod:
        d = (int)d1 % (int)d2;
        break;

    case TknKind::IntDivi:
        d = (int)d1 / (int)d2;
        break;

    case TknKind::Less:
        d = d1 < d2;
        break;

    case TknKind::LessEq:
        d = d1 <= d2;
        break;

    case TknKind::Great:
        d = d1 > d2;
        break;

    case TknKind::GreatEq:
        d = d1 >= d2;
        break;

    case TknKind::Equal:
        d = d1 == d2;
        break;

    case TknKind::NotEq:
        d = d1 != d2;
        break;

    case TknKind::And:
        d = d1 && d2;
        break;

    case TknKind::Or:
        d = d1 || d2;
        break;
    }

    stk.push(d);
}

void post_if_set(bool& flg){
    if (code.kind == TknKind::EofLine){
        flg = true;
        return;
    }
    if (get_expression('?', 0)) flg = true;
}

void fncCall_syntax(int fncNbr){
    int argCt = 0;

    code = nextCode();
    code = chk_nextCode(code, '(');
    if (code.kind != ')'){
        for (;; code=nextCode()){
            (void) get_expression();
            ++argCt;
            if (code.kind != ',') break;
        }
    }
    code = chk_nextCode(code, ')');
    if (argCt != Gtable[fncNbr].args)
        err_exit(Gtable[fncNbr].name, " Wrong the number of parameters", "", "");

    stk.push(1.0);
}

void fncCall(int fncNbr){
    int n, argCt = 0;
    vector<double> vc;

    nextCode(); code = nextCode();
    if (code.kind != ')'){
        for (;; code=nextCode()){
            expression();
            ++argCt;
            if (code.kind != ',') break;
        }
    }
    code = nextCode();

    for (n=0; n < argCt; ++n) vc.push_back(stk.pop());
    for (n=0; n < argCt; ++n) {
        stk.push(vc[n]);
    }
    fncExec(fncNbr);
}

void fncExec(int fncNbr){
    int save_Pc = Pc;
    int save_baseReg = baseReg;
    int save_spReg = spReg;
    char *save_code_ptr = code_ptr;
    CodeSet save_code = code;

    Pc = Gtable[fncNbr].adrs;
    baseReg = spReg;
    spReg += Gtable[fncNbr].frame;
    Dmem.auto_resize(spReg);
    returnValue = 1.0;
    code = firstCode(Pc);

    nextCode(); code = nextCode();
    if (code.kind != '('){
        for (;; code = nextCode()){
            set_dtTyp(code, DBL_T);
            Dmem.set(get_memAdrs(code), stk.pop());
            if (code.kind != ',') break;
        }
    }
    code = nextCode();

    ++Pc;
    block();
    return_Flg = false;

    stk.push(returnValue);
    Pc = save_Pc;
    baseReg = save_baseReg;
    spReg = save_spReg;
    code_ptr = save_code_ptr;
    code = save_code;
}

void sysFncExec_syntax(TknKind kd){
    switch (kd){
    case TknKind::Toint:
        code = nextCode();
        (void)get_expression('(', ')');
        stk.push(1.0);
        break;

    case TknKind::Input:
        code = nextCode();
        code = chk_nextCode(code, '(');
        code = chk_nextCode(code, ')');
        stk.push(1.0);
        break;

    case TknKind::Print:
    case TknKind::Println:
        do {
            code = nextCode();
            if (code.kind == TknKind::String) code = nextCode();
            else (void) get_expression();
        } while (code.kind == ',');
        chk_EofLine();
        break;
    }
}

void sysFncExec(TknKind kd){
    double d;
    string s;

    switch (kd){
    case TknKind::Toint:
        code = nextCode();
        stk.push( (int)get_expression('(', ')') );
        break;

    case TknKind::Input:
        nextCode();
        nextCode();
        code = nextCode();
        getline(cin, s);
        stk.push( atof( s.c_str() ) );

    case TknKind::Print:
    case TknKind::Println:
        do {
            code = nextCode();
            if (code.kind == TknKind::String){
                cout << code.text;
                code = nextCode();
            } else {
                d = get_expression();
                if (!exit_Flg) cout << d;
            }
        } while(code.kind == ',');
        if (kd == TknKind::Println) cout << '\n';
        break;
    }
}

int get_memAdrs(const CodeSet& cd){
    int adr = 0, index, len, line;
    double d;

    adr = get_topAdrs(cd);
    len = tableP(cd)->aryLen;
    code = nextCode();
    if (len ==0) return adr;

    d = get_expression('[', ']');
    if ((int)d != d)
        err_exit("No float ..", "", "", "");

    if (syntaxChk_mode) return adr;

    index = (int) d;
    if (index < 0 || index >= len)
        err_exit(index, " is out of range (range: 0 - ", len-1, ")");
    
    return adr + index;
}

int get_topAdrs (const CodeSet& cd){
    switch (cd.kind)
    {
    case TknKind::Gvar:
        return tableP(cd)->adrs;

    case TknKind::Lvar:
        return tableP(cd)->adrs + baseReg;
    
    default:
        err_exit("Need a name of the variable: ", kind_to_s(cd), "", "");
    }
    return 0;
}

int endline_of_If(int line){
    CodeSet cd;
    char *save = code_ptr;

    cd = firstCode(line);
    for (;;){
        line = cd.jmpAdrs;
        if (cd.kind == TknKind::Elif || cd.kind == TknKind::Else) continue;
        if (cd.kind == TknKind::End) break;
        cd = nextCode();
    }

    code_ptr = save;
    return line;
}

void chk_EofLine(){
    if (code.kind != TknKind::EofLine)
        err_exit("Wrong description: ", kind_to_s(code), "", "");
}

TknKind lookCode(int line){
    return (TknKind) (unsigned char) intercode[line][0];
}

CodeSet chk_nextCode(const CodeSet& cd, int kind2){
    if (cd.kind != kind2){
        if (kind2 == TknKind::EofLine) err_exit("Wrong description: ", kind_to_s(cd), "", "");
        if (cd.kind == TknKind::EofLine) err_exit(kind_to_s(kind2), " is needed.", "", "");
        err_exit(kind_to_s(kind2), " is needed in front of ", kind_to_s(cd), "");
    }
    return nextCode();
}

CodeSet firstCode(int line){
    code_ptr = intercode[line];
    return nextCode();
}

CodeSet nextCode(){
    TknKind kd;
    short int jmpAdrs, tblNbr;

    if (*code_ptr == '\0') return CodeSet(TknKind::EofLine);
    kd = (TknKind)*UCHAR_P(code_ptr++);

    switch (kd){
    case TknKind::Func:
    case TknKind::While:
    case TknKind::For:
    case TknKind::If:
    case TknKind::Elif:
    case TknKind::Else:
        jmpAdrs = *SHORT_P(code_ptr);
        code_ptr += SHORT_SIZ;
        return CodeSet(kd, -1, jmpAdrs);

    case TknKind::String:
        tblNbr = *SHORT_P(code_ptr);
        code_ptr += SHORT_SIZ;
        return CodeSet(kd, strLITERAL[tblNbr].c_str());

    case TknKind::IntNum:
    case TknKind::DblNum:
        tblNbr = *SHORT_P(code_ptr);
        code_ptr += SHORT_SIZ;
        return CodeSet(kd, nbrLITERAL[tblNbr]);

    case TknKind::Fcall:
    case TknKind::Gvar:
    case TknKind::Lvar:
        tblNbr = *SHORT_P(code_ptr);
        code_ptr += SHORT_SIZ;
        return CodeSet(kd, tblNbr, -1);
    
    default:
        return CodeSet(kd);
    }
}