#ifndef PROJECT_ERROR_H
#define PROJECT_ERROR_H

#include "type.h"
#include "SymbolTable.h"
#include "globalData.h"

class ErrorCheckUnit{
public:

    static void checkDupDef(int curScope, string& name, bool isFunc);
    static void checkUndef(int curScope, string& name, bool isFunc);
    static void checkParamsNum(string& funcName, int calledNum);
    static void checkParamsType(string& funcName, vector<pair<IdentType, int>>& calledParams);
    static void checkVoidReturn(int curScope);
    static void checkNoReturn(int curScope);
    static void checkConstAssign(int curScope, string& name);
    static void checkOutsideLoop(int curScope);
    static void checkTokenUnmatched(const int& appeared, const int& expected);

};



#endif //PROJECT_ERROR_H
