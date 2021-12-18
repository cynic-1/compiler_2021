#include "error.h"

void ErrorCheckUnit::checkDupDef(int curScope, string &name, bool isFunc) {
    symbolTableNode *item;
    if (isFunc) {
        item = SymbolTable::findFuncSymbol(name);
    } else {
        item = SymbolTable::findVarSymbol4ErrorCheck(curScope, name);
    }
    if (item != nullptr) {
        cout << "Error! Duplicate Define" << endl;
        exit(-1);
    }
}

void ErrorCheckUnit::checkUndef(int curScope, string &name, bool isFunc) {
    symbolTableNode *item;
    if (isFunc) {
        item = SymbolTable::findFuncSymbol(name);
    } else {
        item = SymbolTable::findVarSymbol(curScope, name);
    }
    if (item == nullptr) {
        cout << "Error! UnDefine" << endl;
        exit(-1);
    }
}

void ErrorCheckUnit::checkParamsNum(string &funcName, int calledNum) {
    symbolTableNode *funcItem = SymbolTable::findFuncSymbol(funcName);
    if (funcItem->paramsNames.size() != calledNum) {
        cout << "Error! Param Number Unmatched " << endl;
        exit(-1);
    }
}

void ErrorCheckUnit::checkParamsType(string &funcName, vector<pair<IdentType, int>> &calledParams) {
    symbolTableNode *funcItem = SymbolTable::findFuncSymbol(funcName);
    int correctNum = funcItem->paramsNames.size();
    int calledNum = calledParams.size();
    int checkNum = correctNum <= calledNum ? correctNum : calledNum;
    for (int i = 0; i < checkNum; i++) {
        if (funcItem->paramsTypeDimList.at(i) != calledParams.at(i)) {
            cout << "Error! Param Type Unmatched" << endl;
            exit(-1);
        }
    }
}

void ErrorCheckUnit::checkVoidReturn(int curScope) {
    Scope *funcScope = SymbolTable::findFirstFuncScope(curScope);
    if (funcScope->isVoidFunc) {
        cout << "Error! Void Return";
        exit(-1);
    }
}

// called when exit any scope, so only check the nearest scope is enough
void ErrorCheckUnit::checkNoReturn(int curScope) {
    Scope *funcScope = SymbolTable::findScope(curScope);
    if (funcScope->scopeKind == FuncScope && !funcScope->isVoidFunc && !funcScope->hasReturned) {
        cout << "Error! No Return" << endl;
        exit(-1);
    }
}

void ErrorCheckUnit::checkConstAssign(int curScope, string &name) {
    symbolTableNode *item = SymbolTable::findVarSymbol(curScope, name);
    if (item != nullptr && item->kind == Const) {
        cout << "Error! Const Assign" << endl;
        exit(-1);
    }
}

void ErrorCheckUnit::checkOutsideLoop(int curScope) {
    if (SymbolTable::findFirstLoopScope(curScope) == nullptr) {
        cout << "Error! Outside Loop" << endl;
        exit(-1);
    }
}

void ErrorCheckUnit::checkTokenUnmatched(const int &appeared, const int &expected) {
    if (appeared == expected) {
        return;
    }
    cout << "Error! Token Unmatched" << endl;
    exit(-1);
}