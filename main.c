#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "ast.h"
#include "symbol_table.h"
#include "type_checker.h"
#include "semantic_analyzer.h"




// Token类型转字符串
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

// 简单的递归下降解析器(构建AST)
ASTNode* parse_expression_to_ast(const char* filename);
ASTNode* parse_E_ast();
ASTNode* parse_T_ast();
ASTNode* parse_F_ast();

ASTNode* parse_F_ast() {
    if (current_token.type == TOKEN_LPAREN) {
        lexer_peek_next_token();
        ASTNode* expr = parse_E_ast();
        if (current_token.type != TOKEN_RPAREN) {
            fprintf(stderr, "语法错误: 期望 ')'\n");
            exit(1);
        }
        lexer_peek_next_token();
        return expr;
    } else if (current_token.type == TOKEN_IDENTIFIER) {
        char* name = strdup(current_token.value);
        ASTNode* node = ast_create_identifier(name, 0);
        free(name);
        lexer_peek_next_token();
        return node;
    } else if (current_token.type == TOKEN_INT_CONST) {
        int value = atoi(current_token.value);
        ASTNode* node = ast_create_int_literal(value, 0);
        lexer_peek_next_token();
        return node;
    } else {
        fprintf(stderr, "语法错误: 期望标识符或常量\n");
        exit(1);
    }
}

ASTNode* parse_T_ast() {
    ASTNode* left = parse_F_ast();
    
    while (current_token.type == TOKEN_MUL || current_token.type == TOKEN_DIV) {
        BinaryOp op = (current_token.type == TOKEN_MUL) ? OP_MUL : OP_DIV;
        lexer_peek_next_token();
        ASTNode* right = parse_F_ast();
        left = ast_create_binary_op(op, left, right, 0);
    }
    
    return left;
}

ASTNode* parse_E_ast() {
    ASTNode* left = parse_T_ast();
    
    while (current_token.type == TOKEN_PLUS || current_token.type == TOKEN_MINUS) {
        BinaryOp op = (current_token.type == TOKEN_PLUS) ? OP_ADD : OP_SUB;
        lexer_peek_next_token();
        ASTNode* right = parse_T_ast();
        left = ast_create_binary_op(op, left, right, 0);
    }
    
    return left;
}

ASTNode* parse_expression_to_ast(const char* filename) {
    // 解析变量声明
    ASTNode** declarations = NULL;
    int decl_count = 0;
    
    while (current_token.type == TOKEN_INT) {
        lexer_peek_next_token();
        
        if (current_token.type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "语法错误: 期望标识符\n");
            exit(1);
        }
        
        char* var_name = strdup(current_token.value);
        Type* var_type = type_create_basic(TYPE_INT);
        
        ASTNode* var_decl = ast_create_var_decl(var_name, var_type, NULL, 0);
        free(var_name);
        
        declarations = (ASTNode**)realloc(declarations, sizeof(ASTNode*) * (decl_count + 1));
        declarations[decl_count++] = var_decl;
        
        lexer_peek_next_token();
        
        if (current_token.type != TOKEN_SEMICOLON) {
            fprintf(stderr, "语法错误: 期望 ';'\n");
            exit(1);
        }
        
        lexer_peek_next_token();
    }
    
    // 解析表达式或赋值语句
    ASTNode* stmt = NULL;
    
    // 检查是否是赋值语句
    if (current_token.type == TOKEN_IDENTIFIER) {
        // 保存标识符
        char* id_name = strdup(current_token.value);
        lexer_peek_next_token();
        
        if (current_token.type == TOKEN_ASSIGN) {
            // 这是赋值语句
            lexer_peek_next_token();
            ASTNode* lvalue = ast_create_identifier(id_name, 0);
            ASTNode* rvalue = parse_E_ast();
            stmt = ast_create_assign_stmt(lvalue, rvalue, 0);
            free(id_name);
        } else {
            // 回退,这是普通表达式
            free(id_name);
            // 重新初始化词法分析器
            lexer_close();
            if (lexer_init(filename) != 0) exit(1);
            // 跳过已经解析的声明
            for (int i = 0; i < decl_count; i++) {
                lexer_peek_next_token(); // int
                lexer_peek_next_token(); // identifier
                lexer_peek_next_token(); // ;
            }
            ASTNode* expr = parse_E_ast();
            stmt = ast_create_expr_stmt(expr, 0);
        }
    } else {
        ASTNode* expr = parse_E_ast();
        stmt = ast_create_expr_stmt(expr, 0);
    }
    
    declarations = (ASTNode**)realloc(declarations, sizeof(ASTNode*) * (decl_count + 1));
    declarations[decl_count++] = stmt;
    
    return ast_create_program(declarations, decl_count);
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
        temp_token = token_copy(current_token);
        printf("[%s] -> %s\n", token_type_str(temp_token.type), temp_token.value);
        token_free(temp_token);
        
        if (current_token.type == TOKEN_EOF) {
            break;
        }
        
        lexer_peek_next_token();
    }
    lexer_close();
    printf("=== Token 流结束 ===\n\n");

    // 构建AST并进行语义分析
    if (lexer_init(argv[1]) != 0) return 1;
    
     printf("=== 构建AST ===\n");
    ASTNode* program = parse_expression_to_ast(argv[1]);
    printf("AST构建完成\n\n");;
    
    // 打印AST
    printf("=== 抽象语法树结构 ===\n");
    ast_print(program, 0);
    printf("\n");
    
    // 创建语义分析器
    SemanticAnalyzer* analyzer = semantic_analyzer_create();
    
    // 进行语义分析(包含类型检查)
    bool success = semantic_analyze_program(analyzer, program);
    
    // 打印符号表
    symbol_table_print(analyzer->symbol_table);
    
    // 清理资源
    semantic_analyzer_destroy(analyzer);
    ast_free(program);
    lexer_close();
    
    if (success) {
        printf("\n\033[32m编译成功!\033[0m\n");
        return 0;
    } else {
        printf("\n\033[31m编译失败!\033[0m\n");
        return 1;
    }
}
