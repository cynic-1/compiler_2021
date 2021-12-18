#ifndef PROJECT_UTILS_H
#define PROJECT_UTILS_H

#include "type.h"

int sizeOfType(IdentType type);
bool isValue(string& result);
bool isDeclBranch();
bool isFuncDefBranch();
bool hasFollow();
bool isExpBranch();
bool isDeclOrStmt();
bool isLValAtPrimaryExp();
bool isPrimaryExpAtUnExp();
bool isFuncRPsAtUnExp();
bool isFuncFPsAtFuncDef();
bool isExpAtReturnStmt();


#endif //PROJECT_UTILS_H
