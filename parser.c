#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 全局符号表（初始为空）
SymbolTableEntry* symbol_table = NULL;

// 语义错误处理（输出错误并终止）
void semantic_error(const char* reason) {
    fprintf(stderr, "\033[31m语义错误：%s\033[0m\n", reason);
    st_free();       // 释放符号表
    lexer_close();   // 关闭词法分析器
    exit(1);
}

// 查询符号表 返回标识符对应的表项
SymbolTableEntry* st_lookup(const char* name) {
    SymbolTableEntry* entry = symbol_table;
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

// 标识符声明
void st_insert(const char* name, VarType type) {
    // 检查是否重复声明
    if (st_lookup(name) != NULL) {
        char reason[256];
        snprintf(reason, sizeof(reason), "标识符 '%s' 重复声明", name);
        semantic_error(reason);
    }
    // 新建表项
    SymbolTableEntry* new_entry = (SymbolTableEntry*)malloc(sizeof(SymbolTableEntry));
    if (new_entry == NULL) {
        perror("malloc failed");
        exit(1);
    }
    new_entry->name = strdup(name);
    new_entry->type = type;
    new_entry->is_declared = true;
    new_entry->next = symbol_table; // 头插法插入链表
    symbol_table = new_entry;
}

// 释放符号表内存
void st_free() {
    SymbolTableEntry* entry = symbol_table;
    while (entry != NULL) {
        SymbolTableEntry* temp = entry;
        entry = entry->next;
        free(temp->name);
        free(temp);
    }
    symbol_table = NULL;
}

// 匹配Token
static void match(TokenType expected_type) {
    if (current_token.type == expected_type) {
        lexer_peek_next_token();
    } else {
        char reason[256];
        const char* token_str = token_type_str(current_token.type);
        const char* expected_str = token_type_str(expected_type);
        snprintf(reason, sizeof(reason), 
                 "期望Token类型：%s，实际Token类型：%s（值：%s）",
                 expected_str, token_str, current_token.value);
        syntax_error(reason);
    }
}

// 解析变量声明
void parse_declaration() {
    // 匹配关键字 int
    if (current_token.type == TOKEN_INT) {
        match(TOKEN_INT);
        // 匹配标识符（变量名）
        if (current_token.type != TOKEN_IDENTIFIER) {
            syntax_error("声明变量时期望标识符");
        }
        char* var_name = strdup(current_token.value);
        // 插入符号表（声明为整型）
        st_insert(var_name, TYPE_INT);
        match(TOKEN_IDENTIFIER);
        // 匹配分号
        match(TOKEN_SEMICOLON);
        free(var_name);
    } else {
        syntax_error("期望变量声明关键字 int");
    }
}


void parse_E() {
    parse_T();
    parse_E_prime();
}

void parse_E_prime() {
    if (current_token.type == TOKEN_PLUS) {
        match(TOKEN_PLUS);
        parse_T();
        parse_E_prime();
    } else if (current_token.type == TOKEN_MINUS) {
        match(TOKEN_MINUS);
        parse_T();
        parse_E_prime();
    }
}

void parse_T() {
    parse_F();
    parse_T_prime();
}

void parse_T_prime() {
    if (current_token.type == TOKEN_MUL) {
        match(TOKEN_MUL);
        parse_F();
        parse_T_prime();
    } else if (current_token.type == TOKEN_DIV) {
        match(TOKEN_DIV);
        // 语义检查：除数不能为0
        if (current_token.type == TOKEN_INT_CONST) {
            int divisor = atoi(current_token.value);
            if (divisor == 0) {
                semantic_error("除法运算中除数不能为0");
            }
        }
        parse_F();
        parse_T_prime();
    }
}

void parse_F() {
    if (current_token.type == TOKEN_LPAREN) {
        match(TOKEN_LPAREN);
        parse_E();
        match(TOKEN_RPAREN);
    } else if (current_token.type == TOKEN_IDENTIFIER) {
        // 语义检查：标识符是否已声明
        char* var_name = strdup(current_token.value);
        SymbolTableEntry* entry = st_lookup(var_name);
        if (entry == NULL || !entry->is_declared) {
            char reason[256];
            snprintf(reason, sizeof(reason), "标识符 '%s' 未声明", var_name);
            semantic_error(reason);
        }
        // 语义检查：类型是否为整型（本项目仅支持int）
        if (entry->type != TYPE_INT) {
            char reason[256];
            snprintf(reason, sizeof(reason), "标识符 '%s' 类型非整型，不支持运算", var_name);
            semantic_error(reason);
        }
        match(TOKEN_IDENTIFIER);
        free(var_name);
    } else if (current_token.type == TOKEN_INT_CONST) {
        match(TOKEN_INT_CONST);
    } else {
        char reason[256];
        snprintf(reason, sizeof(reason),
                 "期望括号/标识符/整型常量，实际Token：%s（值：%s）",
                 token_type_str(current_token.type), current_token.value);
        syntax_error(reason);
    }
}

// 语法分析入口
int parse_expression() {
    // 先解析所有变量声明（如 int a; int b;）
    while (current_token.type == TOKEN_INT) {
        parse_declaration();
    }
    // 再解析表达式
    parse_E();
    if (current_token.type != TOKEN_EOF) {
        syntax_error("表达式后存在多余的Token");
    }
    printf("\033[32m语法正确，语义合法\033[0m\n");
    st_free(); // 释放符号表
    return 0;
}

void syntax_error(const char* reason) {
    fprintf(stderr, "\033[31m语法错误：%s\033[0m\n", reason);
    st_free();       // 释放符号表
    lexer_close();   // 关闭词法分析器
    exit(1);
}
