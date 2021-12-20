#include "type.h"
#include "lex.h"
#include "parse.h"

using namespace std;


ifstream inputFile;
ofstream irFile;


int main(int argc, char* argv[]) {
    if (argc == 1) {
        inputFile.open("input.txt");
        irFile.open("ir.txt");
    } else {
        inputFile.open(argv[1]);
        irFile.open(argv[2]);
    }
    if (inputFile.fail()) {
        return -1;
    }
//    string str;
//    getline(inputFile, str, '\0');
//    cout << str;
    getNextToken();
    compUnit();

    inputFile.close();
    irFile.close();
    return 0;
}
