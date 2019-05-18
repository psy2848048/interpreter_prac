#include "bbi.hpp"
#include "bbi_prot.hpp"

#define NO_FIX_ADRS 0
Token token;
SymTbl tmpTb;
int blkNest;
int localAdrs;
int mainTblNbr;
int loopNest;
bool fncDecl_F;
bool explicit_F;
char codebuf[ LIN_SIZ +1 ], *codebuf_p;
extern vector<char*> intercode;

void init(){
    initChTyp();
    mainTblNbr = -1;
    blkNest = loopNest = 0;
    fncDecl_F = explicit_F = false;
    codebuf_p = codebuf;
}

void convert_to_internalCode(char *fname){
    init();

    fileOpen(fname);
    while(token = nextLine_tkn(), token.kind != TknKind::EofProg){
        if (token.kind == TknKind::Func){
            token = nextTkn();
            set_name();
            enter(tmpTb, SymKind::fncId);
        }
    }
    push_intercode();
    fileOpen(fname);
    token = nextLine_tkn();

    while(token.kind != TknKind::EofProg) convert();
    set_startPc(1);
    if (mainTblNbr != -1){
        set_startPc(intercode.size());
        setCode(TknKind::Fcall, mainTblNbr);
        setCode('(');
        setCode(')');
        push_intercode();
    }
}

void convert(){
    switch (token.kind)
    {
    case TknKind::Option:
        optionSet();
        break;

    case TknKind::Var:
        varDecl();
        break;

    case TknKind::Func:
        fncDecl();
        break;

    case TknKind::While:
    case TknKind::For:
        ++loopNest;
        convert_block_set();
        setCode_End();
        --loopNest;
        break;

    case TknKind::If:
        convert_block_set();
        while (token.kind == TknKind::Elif) convert_block_set();
        if (token.kind == TknKind::Else) convert_block_set();
        setCode_End();
        break;

    case TknKind::Break:
        if (loopNest <= 0) err_exit("Wrong break usage", "", "", "");
        setCode(token.kind);
        token = nextTkn();
        convert_rest();
        break;

    case TknKind::Return:
        if (!fncDecl_F) err_exit("Wrong return usage", "", "", "");
        setCode(token.kind);
        token = nextTkn();
        convert_rest();
        break;

    case TknKind::Exit:
        setCode(token.kind);
        token = nextTkn();
        convert_rest();
        break;

    case TknKind::Print:
    case TknKind::Println:
        setCode(token.kind);
        token = nextTkn();
        convert_rest();
        break;
    
    case TknKind::End:
        err_exit("Wrong return usage", "", "", "");
        break;

    default:
        convert_rest();
        break;
    }
}

void convert_block_set(){
    int patch_line;
    patch_line = setCode(token.kind, NO_FIX_ADRS);
    token = nextTkn();
    convert_rest();
    convert_block();
    backPatch(patch_line, get_lineNo());
}

void convert_block(){
    TknKind k;
    ++blkNest;
    while (k=token.kind, k != TknKind::Elif && k != TknKind::Else && k != TknKind::End && k != TknKind::EofProg) convert();
    --blkNest;
}

void convert_rest(){
    int tblNbr;

    for(;;){
        if (token.kind == TknKind::EofLine) break;

        switch (token.kind){
        case TknKind::If:
        case TknKind::Elif:
        case TknKind::Else:
        case TknKind::For:
        case TknKind::While:
        case TknKind::Break:
        case TknKind::Func:
        case TknKind::Return:
        case TknKind::Exit:
        case TknKind::Print:
        case TknKind::Println:
        case TknKind::Option:
        case TknKind::Var:
        case TknKind::End:
            err_exit("Wrong usage", "", "", "");
            break;

        case TknKind::Ident:
            set_name();
            if ( (tblNbr=searchName(tmpTb.name, 'F')) != -1){
                if (tmpTb.name == "main") err_exit("Cannot call main function", "", "", "");
                setCode(TknKind::Fcall, tblNbr);
                continue;
            }
            if ( (tblNbr=searchName(tmpTb.name, 'V')) == -1){
                if (explicit_F) err_exit("Need to declare variable first: ", tmpTb.name, "", "");
                tblNbr = enter(tmpTb, SymKind::varId);
                continue;
            }
            if (is_localName(tmpTb.name, SymKind::varId)) setCode(TknKind::Lvar, tblNbr);
            else setCode(TknKind::Gvar, tblNbr);
            continue;
        
        case TknKind::IntNum:
        case TknKind::DblNum:
            setCode(token.kind, set_LITERAL(token.dblVal));
            break;

        case TknKind::String:
            setCode(token.kind, set_LITERAL(token.text));
            break;

        default:
            setCode(token.kind);
            break;
        }
        token = nextTkn();
    }
    push_intercode();
    token = nextLine_tkn();
}

void optionSet(){
    setCode(TknKind::Option);
    setCode_rest();
    token = nextTkn();
    if (token.kind == TknKind::String && token.text == "var") explicit_F = true;
    else err_exit("Wrong usage of", " Option ", "", "");
    token = nextTkn();
    setCode_EofLine();
}

void varDecl(){
    setCode(TknKind::Var);
    setCode_rest();
    for (;;){
        token = nextTkn();
        var_namechk(token);
        set_name();
        set_aryLen();
        enter(tmpTb, SymKind::varId);
        if (token.kind != ',') break;
    }
    setCode_EofLine();
}

void var_nameChk(const Token& tk){
    if (tk.kind != TknKind::Ident) err_exit( err_msg(tk.text, "identifier"));
    if (is_localScope() && tk.text[0] == '$')
        err_exit("Connot define a variable which contains '$': ", tk.text, "", "");
    if ( searchName(tk.text, 'V') != -1)
        err_exit("Duplicate variable: ", tk.text, "", "");
}

void set_name(){
    if (token.kind != TknKind::Ident) err_exit("Need identifier: ", token.text);
    tmpTb.clear();
    tmpTb.name = token.text;
    token = nextTkn();
}

void set_aryLen(){
    tmpTb.aryLen = 0;
    if (token.kind != '[') return;

    token = nextTkn();
    if (token.kind != TknKind::IntNum)
        err_exit("Set positive integer for the length of the array: ", token.text, "", "");
    tmpTb.aryLen = (int)token.dblVal + 1;
}