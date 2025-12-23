#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ast.h"
#include <stdbool.h>

// ========== 符号类型 ==========

typedef enum {
    SYMBOL_VAR,       // 变量
    SYMBOL_FUNC,      // 函数
    SYMBOL_PARAM,     // 函数参数
    SYMBOL_TYPE       // 类型定义(typedef)
} SymbolKind;

// ========== 符号表项 ==========

typedef struct Symbol {
    char* name;           // 符号名称
    SymbolKind kind;      // 符号类型
    Type* type;           // 符号的数据类型
    int scope_level;      // 作用域层级(0=全局, 1+=局部)
    bool is_defined;      // 是否已定义(区分声明和定义)
    
    // 变量特有信息
    struct {
        bool is_const;    // 是否为常量
        int offset;       // 栈帧偏移(用于代码生成)
    } var_info;
    
    // 函数特有信息
    struct {
        Type** param_types;  // 参数类型列表
        int param_count;     // 参数个数
        bool is_declared;    // 是否已声明
    } func_info;
    
    struct Symbol* next;  // 链表下一项(哈希冲突处理)
} Symbol;

// ========== 作用域 ==========

typedef struct Scope {
    int level;                // 作用域层级
    Symbol** symbols;         // 符号哈希表
    int symbol_count;         // 符号数量
    struct Scope* parent;     // 父作用域
    struct Scope** children;  // 子作用域列表
    int child_count;          // 子作用域数量
} Scope;

// ========== 符号表 ==========

#define SYMBOL_TABLE_SIZE 256  // 哈希表大小

typedef struct SymbolTable {
    Scope* global_scope;    // 全局作用域
    Scope* current_scope;   // 当前作用域
    int current_level;      // 当前作用域层级
} SymbolTable;

// ========== 符号表操作函数 ==========

// 创建和销毁
SymbolTable* symbol_table_create();
void symbol_table_destroy(SymbolTable* table);

// 作用域管理
void symbol_table_enter_scope(SymbolTable* table);
void symbol_table_exit_scope(SymbolTable* table);
Scope* symbol_table_current_scope(SymbolTable* table);

// 符号插入和查找
Symbol* symbol_table_insert(SymbolTable* table, const char* name, SymbolKind kind, Type* type);
Symbol* symbol_table_lookup(SymbolTable* table, const char* name);
Symbol* symbol_table_lookup_current_scope(SymbolTable* table, const char* name);

// 符号更新
void symbol_update_definition(Symbol* symbol, bool is_defined);
void symbol_update_var_info(Symbol* symbol, bool is_const, int offset);
void symbol_update_func_info(Symbol* symbol, Type** param_types, int param_count);

// 工具函数
void symbol_table_print(SymbolTable* table);
const char* symbol_kind_str(SymbolKind kind);

// ========== 作用域操作函数 ==========

Scope* scope_create(int level, Scope* parent);
void scope_destroy(Scope* scope);
Symbol* scope_lookup(Scope* scope, const char* name);
Symbol* scope_insert(Scope* scope, const char* name, SymbolKind kind, Type* type);

// ========== 符号操作函数 ==========

Symbol* symbol_create(const char* name, SymbolKind kind, Type* type, int scope_level);
void symbol_destroy(Symbol* symbol);

// ========== 哈希函数 ==========

unsigned int hash_string(const char* str);

#endif // SYMBOL_TABLE_H
