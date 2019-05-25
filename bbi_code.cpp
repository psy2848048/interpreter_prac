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

}

void statement(){

}

void block(){

}

double get_expression(int kind1, int kind2){

}

void expression(int kind1, int kind2){

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

}

void binaryExpr(TknKind op){

}

void post_if_set(bool& flg){

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

}

void fncExec(int fncNbr){

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

}

int get_memAdrs(CodeSet& cd){
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