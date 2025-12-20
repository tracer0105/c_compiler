#include "lexer.h"
#include "parser.h" // 新增：引入语法分析器函数声明

// （原有token_type_str函数不变，若已移到lexer.h则删除此处重复定义）
const char* token_type_str(TokenType type) {
    switch (type) {
        case TOKEN_IF: return "TOKEN_IF";
        case TOKEN_ELSE: return "TOKEN_ELSE";
        case TOKEN_INT: return "TOKEN_INT";
        case TOKEN_RETURN: return "TOKEN_RETURN";
        case TOKEN_VOID: return "TOKEN_VOID";
        case TOKEN_WHILE: return "TOKEN_WHILE";
        case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
        case TOKEN_INT_CONST: return "TOKEN_INT_CONST";
        case TOKEN_PLUS: return "TOKEN_PLUS";
        case TOKEN_MINUS: return "TOKEN_MINUS";
        case TOKEN_MUL: return "TOKEN_MUL";
        case TOKEN_DIV: return "TOKEN_DIV";
        case TOKEN_GT: return "TOKEN_GT";
        case TOKEN_GE: return "TOKEN_GE";
        case TOKEN_LT: return "TOKEN_LT";
        case TOKEN_LE: return "TOKEN_LE";
        case TOKEN_EQ: return "TOKEN_EQ";
        case TOKEN_NE: return "TOKEN_NE";
        case TOKEN_ASSIGN: return "TOKEN_ASSIGN";
        case TOKEN_PLUS_EQ: return "TOKEN_PLUS_EQ";
        case TOKEN_MINUS_EQ: return "TOKEN_MINUS_EQ";
        case TOKEN_INC: return "TOKEN_INC";
        case TOKEN_DEC: return "TOKEN_DEC";
        case TOKEN_LBRACE: return "TOKEN_LBRACE";
        case TOKEN_RBRACE: return "TOKEN_RBRACE";
        case TOKEN_LPAREN: return "TOKEN_LPAREN";
        case TOKEN_RPAREN: return "TOKEN_RPAREN";
        case TOKEN_SEMICOLON: return "TOKEN_SEMICOLON";
        case TOKEN_COMMA: return "TOKEN_COMMA";
        case TOKEN_EOF: return "TOKEN_EOF";
        case TOKEN_ERROR: return "TOKEN_ERROR";
        default: return "TOKEN_UNKNOWN";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input.c>\n", argv[0]);
        return 1;
    }

    // 打印 Token 流
    printf("=== Token 流 ===\n");
    if (lexer_init(argv[1]) != 0) return 1;
    
    Token temp_token;
    while (1) {
        // 深拷贝，避免共享内存
        temp_token = token_copy(current_token);
        
        // 先打印 Token（包括 EOF）
        printf("[%s] -> %s\n", token_type_str(temp_token.type), temp_token.value);
        token_free(temp_token); // 释放拷贝的内存
        
        // 判断是否为 EOF，是则退出循环
        if (current_token.type == TOKEN_EOF) {
            break;
        }
        
        // 非 EOF 则预读下一个 Token
        lexer_peek_next_token();
    }
    lexer_close(); // 关闭文件，释放current_token
    printf("=== Token 流结束 ===\n");

    // 语法+语义分析
    if (lexer_init(argv[1]) != 0) return 1;
    parse_expression();
    lexer_close();

    return 0;
}