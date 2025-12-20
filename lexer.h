#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>

// Token类型枚举
typedef enum {
    // 关键字
    TOKEN_IF, TOKEN_ELSE, TOKEN_INT, TOKEN_RETURN, TOKEN_VOID, TOKEN_WHILE,
    // 标识符
    TOKEN_IDENTIFIER,
    // 整型常量（十进制/八进制/十六进制）
    TOKEN_INT_CONST,
    // 运算符
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MUL, TOKEN_DIV,
    TOKEN_GT, TOKEN_GE, TOKEN_LT, TOKEN_LE, TOKEN_EQ, TOKEN_NE,
    TOKEN_ASSIGN, TOKEN_PLUS_EQ, TOKEN_MINUS_EQ, TOKEN_INC, TOKEN_DEC,
    // 分界符
    TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_SEMICOLON, TOKEN_COMMA,
    // 结束/错误
    TOKEN_EOF, TOKEN_ERROR
} TokenType;

// Token结构体（类型+值）
typedef struct {
    TokenType type;
    char* value; // 存储标识符/常量的字符串值
} Token;

// 再声明全局变量和函数 

Token token_copy(Token src);

// 全局变量 input_file
extern FILE* input_file;

// 全局当前Token（语法分析器需预读Token）
extern Token current_token;

// 初始化词法分析器（传入输入文件路径）
int lexer_init(const char* filename);
// 获取下一个Token
Token lexer_next_token();
// 释放Token的value内存
void token_free(Token token);
// 关闭词法分析器
void lexer_close();
// 新增：预读下一个Token并更新current_token
void lexer_peek_next_token();

// Token类型转字符串（供其他文件调用）
const char* token_type_str(TokenType type);

#endif // LEXER_H
