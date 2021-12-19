#include "SymbolTable.h"
#include "utils.h"


symbolTableNode *Scope::lookupVarSym(string &name) {
    symbolTableNode *ret = nullptr;
    for (auto & iter:allSymbols) {
        if (iter.kind != Function && iter.name == name) {
            ret = &(iter);
            break;
        }
    }
    return ret;
}


vector<Scope> SymbolTable::allScopes;
int SymbolTable::nextIndex = 0;
int SymbolTable::globalAddr = 0;

int SymbolTable::addScope(int parentIndex, ScopeKind kind, bool isVoidFunc, string funcName, string headLabel, string tailLabel) {
    Scope newScope;
    newScope.scopeKind = kind;
    newScope.hasReturned = false;
    newScope.isVoidFunc = isVoidFunc;
    newScope.funcName = funcName;
    newScope.parentScopeIndex = parentIndex;

    // special for loop scope, so that break or continue stmt can know where to jump.
    newScope.scopeHeadLabel = headLabel;
    newScope.scopeTailLabel = tailLabel;

    newScope.selfIndex = nextIndex;
    nextIndex++;


    if (kind == FuncScope || parentIndex == NON_PARENT) {
        newScope.localAddr = 0;
    } else {
        newScope.localAddr = findScope(parentIndex)->localAddr;
    }

    // refill the funcScopeIdx.
    if (kind == FuncScope) {
        findFuncSymbol(funcName)->funcScopeIdx = newScope.selfIndex;
    }

    allScopes.push_back(newScope);
    return newScope.selfIndex;
}

void SymbolTable::addFuncSymbol(int curScope, string &funcName, IdentType returnType,
                                vector<pair<IdentType, int>> &paramsTypeDimList, vector<string> &paramsNames) {
    symbolTableNode newItem;
    newItem.name = funcName;
    newItem.kind = Function;
    newItem.type = returnType;
    newItem.size = 0;
    newItem.scopeIdx = curScope;

    newItem.paramsTypeDimList = paramsTypeDimList;
    newItem.paramsNames = paramsNames;

    // don't change scope's localAddr
    SymbolTable::findScope(curScope)->allSymbols.push_back(newItem);
}

void SymbolTable::addConstSymbol(int curScope, string &constName, IdentType constType, vector<int>& axis
    , vector<int> &constValues) {

    Scope *scope = SymbolTable::findScope(curScope);

    symbolTableNode newItem;
    newItem.name = constName;
    newItem.kind = Const;
    newItem.type = constType;
    newItem.scopeIdx = curScope;

    newItem.dimension = axis.size();
    newItem.axis = axis;

    int count = 1;

    newItem.size = sizeOfType(constType) * count;
    // is going to be set in the RunTime Stack.
    if (count > 1) {
        // set in global, and don't increase the size of scope
        newItem.addr = globalAddr;
        globalAddr += newItem.size;
        if (curScope == 0) {
            scope->localAddr += newItem.size;
        }
    }

    newItem.constValues = constValues;

    scope->allSymbols.push_back(newItem);

}

void SymbolTable::addVarSymbol(int curScope, const string &name, const string& pointerName, IdentType varType, const vector<int>& axis) {
    Scope *scope = SymbolTable::findScope(curScope);

    symbolTableNode newItem;
    newItem.name = name;
    newItem.pointerName = pointerName;
    newItem.kind = Var;
    newItem.type = varType;
    newItem.scopeIdx = curScope;

    newItem.dimension = axis.size();
    newItem.axis = axis;
    int count = 1;

    newItem.size = sizeOfType(varType) * count;
    newItem.addr = scope->localAddr;
    scope->localAddr += newItem.size;

    // update globalAddr
    if (curScope == 0) {
        globalAddr += newItem.size;
    }

    scope->allSymbols.push_back(newItem);
}

void SymbolTable::addParamSymbol(int curScope, string &name, IdentType varType, const vector<int>& axis) {
    Scope *scope = SymbolTable::findScope(curScope);

    // copied from addVar
    symbolTableNode newItem;
    newItem.name = name;
    newItem.kind = Param;
    newItem.type = varType;
    newItem.scopeIdx = curScope;

    newItem.dimension = axis.size();
    newItem.axis = axis;

    int count = 1;

    newItem.size = sizeOfType(varType) * count;
    newItem.addr = scope->localAddr;
    scope->localAddr += newItem.size;



    scope->allSymbols.push_back(newItem);
}

void SymbolTable::addParamSymbol(int curScope, symbolTableNode &param) {
    Scope *scope = findScope(curScope);
    // refill the addr of param
    param.addr = scope->localAddr;
    // refill the scopeIdx
    param.scopeIdx = curScope;

    scope->localAddr += param.size;
    scope->allSymbols.emplace_back(param);
}

void SymbolTable::addTempSymbol(int curScope, string &name, IdentType tempType) {
    Scope *scope = SymbolTable::findScope(curScope);

    symbolTableNode newItem;
    newItem.regName = name;
    newItem.kind = Temp;
    newItem.type = tempType;
    newItem.size = sizeOfType(tempType);
    newItem.scopeIdx = curScope;

    newItem.addr = scope->localAddr;
    scope->localAddr += newItem.size;

    // update globalAddr
    if (curScope == 0) {
        globalAddr += newItem.size;
    }

    scope->allSymbols.push_back(newItem);
}

symbolTableNode *SymbolTable::findFuncSymbol(const string &funcName, int curScope) {
    // funcSymbol only exists in the first scope, aka global scope (index is 0).
    symbolTableNode *ret = nullptr;
    vector<symbolTableNode> &itemList = SymbolTable::findScope(0)->allSymbols;
    for (auto iter = itemList.begin(); iter < itemList.end(); ++iter) {
        if (iter->kind == Function && iter->name == funcName) {
            ret = &(*iter);
            break;
        }
    }
    return ret;
}

symbolTableNode *SymbolTable::findVarSymbol(int curScope, string &varName) {
    Scope *searchScope = SymbolTable::findScope(curScope);
    symbolTableNode *ret = nullptr;
    while (true) {
        ret = searchScope->lookupVarSym(varName);
        if (ret != nullptr) {
            break;
        }
        if (searchScope->parentScopeIndex == NON_PARENT) {
            break;
        } else {
            searchScope = SymbolTable::findScope(searchScope->parentScopeIndex);
        }
    }
    return ret;
}

Scope *SymbolTable::findScope(int scopeIndex) {
    return &(allScopes.at(scopeIndex));
}

int SymbolTable::exitScope(int curScopeIndex) {
    Scope *curScope = SymbolTable::findScope(curScopeIndex);
    if (curScope->parentScopeIndex == NON_PARENT) {
        return NON_PARENT;
    }

    // refill the const array
    Scope *firstFuncScope = findFirstFuncScope(curScopeIndex);
    if (firstFuncScope != nullptr) {
        for (auto iter = curScope->allSymbols.begin(); iter < curScope->allSymbols.end(); ++iter) {
            if (iter->kind == Const && iter->dimension != 0) {
                firstFuncScope->fourByteConstArray_AddrValuesMap.emplace(make_pair(iter->addr, iter->constValues));
            }
        }
    }

    Scope *parent = SymbolTable::findScope(curScope->parentScopeIndex);
    if (curScope->scopeKind == FuncScope) {
        symbolTableNode *funcSym = findFuncSymbol(curScope->funcName);
        // refill the funcSym's size
        funcSym->size = curScope->localAddr;
        // refill the funcSym's funcScopeIdx
        // move the refill of funcScopeIdx to enterScope because we need to know the fparam's defScope in recursive function.
        // funcSym->funcScopeIdx = curScopeIndex;

    } else {
        parent->localAddr = curScope->localAddr;
    }

    return curScope->parentScopeIndex;
}

symbolTableNode SymbolTable::createParamItem(const string &name, const string &regOrPointerName, IdentType paramType, const vector<int>& axis) {
    // copied from addVar
    symbolTableNode newItem;
    newItem.name = name;
    newItem.kind = Param;
    newItem.type = paramType;

    newItem.dimension = axis.size();
    newItem.axis = axis;


    if (axis.empty()) { // non-array regName
        newItem.regName = regOrPointerName;
    }
    else { // array pointerName
        newItem.pointerName = regOrPointerName;
    }

    return newItem;
}

Scope *SymbolTable::findFirstFuncScope(int curScope) {
    int searchScopeIdx = curScope;
    while (allScopes.at(searchScopeIdx).scopeKind != FuncScope) {
        if (allScopes.at(searchScopeIdx).parentScopeIndex == NON_PARENT) {
            break;
        }
        searchScopeIdx = allScopes.at(searchScopeIdx).parentScopeIndex;
    }
    if (allScopes.at(searchScopeIdx).scopeKind == FuncScope) {
        return &(allScopes.at(searchScopeIdx));
    } else {
        return nullptr;
    }
}

symbolTableNode* SymbolTable::findVarSymbol4ErrorCheck(int curScope, string& varName) {
    return SymbolTable::allScopes.at(curScope).lookupVarSym(varName);
}

Scope* SymbolTable::findFirstLoopScope(int curScope) {
    int searchScopeIdx = curScope;
    while (allScopes.at(searchScopeIdx).scopeKind != LoopScope) {
        if (allScopes.at(searchScopeIdx).parentScopeIndex == NON_PARENT) {
            break;
        }
        searchScopeIdx = allScopes.at(searchScopeIdx).parentScopeIndex;
    }
    if (allScopes.at(searchScopeIdx).scopeKind == LoopScope) {
        return &(allScopes.at(searchScopeIdx));
    } else {
        return nullptr;
    }
}