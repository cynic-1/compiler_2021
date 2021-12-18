#include "type.h"
#include "lex.h"
#include "parse.h"
#include "error.h"

using namespace std;


ifstream inputFile;
ofstream irFile;


int main(int argc, char* argv[]) {
    if (argc == 0) {
        inputFile.open("testfile.txt");
        irFile.open("ir.txt");
    } else if (argc == 2){
        inputFile.open(argv[1]);
        irFile.open(argv[2]);
    } else {
        cout << "Error! In ARGUMENT!" << endl;
        exit(-1);
    }
    if (inputFile.fail()) {
        return -1;
    }


    getNextToken();
    compUnit();
    if (curTokenContext.tokenType == READ_EOF) {
        cout << "file ended" << endl;
    } else {
        cout << "file not ended" << endl;
    }

    inputFile.close();
    irFile.close();
    return 0;
}
