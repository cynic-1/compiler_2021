#ifndef PROJECT_GLOBALDATA_H
#define PROJECT_GLOBALDATA_H

extern const string tokenTypeStr[];
extern const map<string, TokenType> reservedMap;
extern const map<string, TokenType> separatorMap;
extern const map<SyntaxType, set<int>> firstSetMap;

#endif //PROJECT_GLOBALDATA_H
