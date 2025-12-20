#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdbool.h>
#include <stdlib.h>

// 标识符类型
typedef enum {
    TYPE_INT
} VarType;

// 符号表项
typedef struct SymbolTableEntry {
    char* name;          // 标识符名称
    VarType type;        // 标识符类型
    bool is_declared;    // 是否已声明
    struct SymbolTableEntry* next; // 链表下一项（解决标识符重名冲突）
} SymbolTableEntry;

// 全局符号表
extern SymbolTableEntry* symbol_table;

// 符号表操作函数声明
SymbolTableEntry* st_lookup(const char* name);
void st_insert(const char* name, VarType type);
void st_free();

// 语法分析入口函数
int parse_expression();

// 递归下降核心函数
void parse_E();    // 解析E → T E'
void parse_E_prime(); // 解析E' → + T E' | - T E' | ε
void parse_T();    // 解析T → F T'
void parse_T_prime(); // 解析T' → * F T' | / F T' | ε
void parse_F();    // 解析F → ( E ) | ID | INT_CONST

// 语义分析函数
void semantic_error(const char* reason);       // 语义错误处理
SymbolTableEntry* st_lookup(const char* name); // 查询符号表
void st_insert(const char* name, VarType type); // 插入符号表
void st_free();                                // 释放符号表内存

// 解析变量声明
void parse_declaration();

// 语法错误处理函数
void syntax_error(const char* reason);

#endif // PARSER_H
