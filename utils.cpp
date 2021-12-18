#include "utils.h"
#include "lex.h"
#include "globalData.h"
int sizeOfType(IdentType type) {
    if (type == IntType) {
        return 4;
    } else {
        return 0;
    }
}

bool isValue(string &result) {
    // empty string is not a value! occured at axisLen
    if (result.size() == 0) {
        return false;
    }
    // notice negative number
    if ((result[0] >= '0' && result[0] <= '9') || result[0] == '-') {
        return true;
    }
    return false;
}

/**
 * Choose whether to go to decl or to func def
 * @return true -> decl; false -> func def
 */
bool isDeclBranch() {
    if (curTokenContext.tokenType == ConstToken)
        return true;

    peekNextToken();
    peekNextToken();
    if (tokenPeekQueue.at(1).tokenType == LParen) {
        return false;
    }
    return true;
}

/**
 * Choose whether to jump to func def or to main
 * @return true -> func def; false -> main
 */
bool isFuncDefBranch() {
    if (curTokenContext.tokenType == Void)
        return true;

    if (tokenPeekQueue.empty()) {
        peekNextToken();
    }
    // 有main走main，无main走funcDef
    if (tokenPeekQueue.front().tokenType == Main)
        return false;
    return true;
}

// synType's followSet contains current syntax
bool followContains(SyntaxType synType) {
    auto iter = firstSetMap.find(synType);
    if (iter == firstSetMap.end()) {
        cout << "wrong in hasFollow" << endl;
        exit(-1);
    }
    auto tempSet = iter->second;
    auto iter2 = tempSet.find(curTokenContext.tokenType);
    if (iter2 == tempSet.end()) {
        return false;
    } else {
        return true;
    }
}

bool isExpBranch() {
    if (followContains(Exp))
        return true;
    else
        return false;
}

bool isDeclOrStmt() {
    if (followContains(Decl))
        return true;
    else
        return false;
}

bool isLValAtPrimaryExp() {
    if (followContains(LVal))
        return true;
    return false;
}

bool isPrimaryExpAtUnExp() {
    if (!followContains(PrimaryExp))
        return false;
    if (curTokenContext.tokenType != Ident)
        return true;
    if (tokenPeekQueue.empty()) {
        peekNextToken();
    }
    if (tokenPeekQueue.front().tokenType == LParen)
        return false;
    return true;
}

bool isFuncRPsAtUnExp() {
    if (followContains(FuncRParams))
        return true;
    return false;
}

bool isFuncFPsAtFuncDef() {
    if (followContains(FuncFParams))
        return true;
    return false;
}

bool isExpAtReturnStmt() {
    if (followContains(Exp))
        return true;
    return false;
}
