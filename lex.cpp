#include "lex.h"
#include "globalData.h"

TokenContext tokenContextBuffer = {
        .token = "",
        .lineNum = 1
};
char curChar;

// the current token who is being analysed
TokenContext curTokenContext;

// use it when peeking and backtracking
deque<TokenContext> tokenPeekQueue;
int peekPos = 0;

string outputBuffer = "";


// post-condition: curChar is the first char after blank
void skipBlank() {
    while (curChar == ' ' || curChar == '\n' || curChar == '\t' || curChar == '\r') {
        if (curChar == '\n')
            tokenContextBuffer.lineNum += 1;
        curChar = inputFile.get();
    }
}

// post-condition: curChar is the first char after annotation
void skipComment() {
    if (curChar == '/' && inputFile.peek() == '/') {
        while (curChar != '\n') {
            if (curChar == EOF) {
                return;
            }
            curChar = inputFile.get();
        }
        curChar = inputFile.get();
        tokenContextBuffer.lineNum += 1;
        return;
    } else if (curChar == '/' && inputFile.peek() == '*'){
        // move the curChar to the first character after /*
        curChar = inputFile.get();
        curChar = inputFile.get();
        while (!(curChar == '*' && inputFile.peek() == '/')) {
            if (curChar == '\n')
                tokenContextBuffer.lineNum += 1;
            curChar = inputFile.get();
            if (curChar == EOF)
                // error check
                break;
        }
        // move the curChar to the first character after */
        curChar = inputFile.get();
        curChar = inputFile.get();
    }
}

bool isBlank(const char ch) {
    if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r')
        return true;
    return false;
}

bool isComment(const char ch1, const char ch2) {
    if (ch1 == '/' && (ch2 == '/' || ch2 == '*')) {
        return true;
    } else {
        return false;
    }
}

bool isDigit(const char ch) {
    if (ch >= '0' && ch <= '9')
        return true;
    else
        return false;
}

bool isIdentNonDigit(const char ch) {
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')
        return true;
    else
        return false;
}

int getIdent() {
    while (isDigit(curChar) || isIdentNonDigit(curChar)) {
        tokenContextBuffer.token += curChar;
        curChar = inputFile.get();
    }

    // check if it's a reserved word
    auto itor = reservedMap.find(tokenContextBuffer.token);
    if (itor != reservedMap.end()) {
        return itor->second;
    } else {
        return Ident;
    }
}

int getIntConst() {
    while (isDigit(curChar)) {
        tokenContextBuffer.token += curChar;
        curChar = inputFile.get();
        // value = value * 10 + curChar - '0';
    }
    string& token = tokenContextBuffer.token;

    // 处理八、十六进制
    if (token.size() >= 2) {
        if (token.at(0) == '0') {
            if (token.at(1) == 'x' || token.at(1) == 'X') {
                token = to_string(stoi(token, nullptr, 16));
            } else {
                token = to_string(stoi(token, nullptr, 8));
            }
        }
    }
    return IntConst;
}

int getStrConst() {
    curChar = inputFile.get();
    while (curChar != '\"') {
        tokenContextBuffer.token += curChar;
        curChar = inputFile.get();
        continue;
    }
    if (curChar == '\"') {
        tokenContextBuffer.token = "\"" + tokenContextBuffer.token + "\"";
        curChar = inputFile.get();
        return StrConst;
    } else {
        // prepared for error check
        return -999;
    }
}

int getSeparator() {

    // store the first char
    tokenContextBuffer.token += curChar;
    curChar = inputFile.get();

    // multi-char separator
    string temp(tokenContextBuffer.token + curChar);
    auto iter = separatorMap.find(temp);
    if (iter != separatorMap.end()) {
        tokenContextBuffer.token = temp;
        curChar = inputFile.get();
        return iter->second;
    }

    // it's a one-char separator
    iter = separatorMap.find(tokenContextBuffer.token);
    if (iter != separatorMap.end()) {
        return iter->second;
    }

    // 不是分隔符
    return READ_ERROR;
}


void getTokenFile2Buff() {
    curChar = inputFile.get();
    tokenContextBuffer.token.clear();

    while (isBlank(curChar) || isComment(curChar, inputFile.peek())) {
        skipBlank();
        skipComment();
    }
    int return_type = READ_ERROR;
    if (isIdentNonDigit(curChar)) {
        return_type = getIdent();
    } else if (isDigit(curChar)) {
        return_type = getIntConst();
    } else if (curChar == '\"') {
        return_type = getStrConst();
    } else if (curChar == EOF){
        return_type = READ_EOF;
    } else {  // the peek is safe because of the error-checker in getSeparator()
        // whether it's seperator or it's fault
        return_type = getSeparator();
    }

    // move the file pointer back one, in order to get the right char in the next getToken call.
    inputFile.putback(curChar);
    tokenContextBuffer.tokenType = return_type;

}

/**
 * peek the next token from file, then push it into the tokenPeekQueue
 */
void peekNextToken() {
    getTokenFile2Buff();
    tokenPeekQueue.push_back(tokenContextBuffer);
}

/**
 * get curToken from tokenPeekQueue
 */
void getTokenFromQueue() {
    curTokenContext = tokenPeekQueue.front();
    tokenPeekQueue.pop_front();
}

/**
 * get curToken from the FileStream
 */
void getTokenFromFile() {
    getTokenFile2Buff();
    curTokenContext = tokenContextBuffer;
}

// get next token. If tokenPeekQueue is not empty, then get from the queue; else from the FileStream.
// set the curToken
/**
 * 队空从文件拿，不空从队拿（队里相当于预先看了的字符）
 */
void getNextToken() {
    if (!isPreCheck) {
        if (tokenPeekQueue.empty()) {
            getTokenFromFile(); // set the newest data to curTokenContext
        } else {
            getTokenFromQueue(); // pop_front the peekQueue
        }
    } else {

        if (peekPos >= tokenPeekQueue.size()) {  // peekPos is over the newest token, so peek it from file.
            peekNextToken();    // always > 1
        }
        curTokenContext = tokenPeekQueue.at(peekPos);
        peekPos++;
    }

}
