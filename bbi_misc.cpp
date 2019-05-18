#include "bbi.hpp"
#include "bbi_prot.hpp"

void err_exit(Tobj a="\1", Tobj b="\1", Tobj c="\1", Tobj d="\1"){
    Tobj ob[5];
    ob[1] = a; ob[2] = b; ob[3] = c; ob[4] = d;
    cerr << "line:" << get_lineNo() << " ERRPR ";
    for (int i=1; i <= 4 && ob[i].s != "\1"; ++i){
        if (ob[i].type =='d') cout << ob[i].d;
        if (ob[i].type =='s') cout << ob[i].s;
    }
    cout << "\n";
    exit(1);
}