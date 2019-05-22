#include "bbi.hpp"
#include "bbi_prot.hpp"

int main(int argc, char *argv[]){
    if (argc == 1){
        cout << "Usage: bbi filename\n";
        exit(1);
    } 
    convert_to_internalCode(argv[1]);
    syntaxChk();
    execute();

    return 0;
}