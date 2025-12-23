#include "symbol_table.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* strdup(const char* s);

// ========== 哈希函数 ==========

unsigned int hash_string(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % SYMBOL_TABLE_SIZE;
}

// ========== 符号操作函数 ==========

Symbol* symbol_create(const char* name, SymbolKind kind, Type* type, int scope_level) {
    Symbol* symbol = (Symbol*)malloc(sizeof(Symbol));
    if (!symbol) {
        perror("malloc failed for Symbol");
        exit(1);
    }
    
    symbol->name = strdup(name);
    symbol->kind = kind;
    symbol->type = type_copy(type);
    symbol->scope_level = scope_level;
    symbol->is_defined = false;
    symbol->next = NULL;
    
    // 初始化变量信息
    symbol->var_info.is_const = false;
    symbol->var_info.offset = 0;
    
    // 初始化函数信息
    symbol->func_info.param_types = NULL;
    symbol->func_info.param_count = 0;
    symbol->func_info.is_declared = false;
    
    return symbol;
}

void symbol_destroy(Symbol* symbol) {
    if (!symbol) return;
    
    free(symbol->name);
    type_free(symbol->type);
    
    if (symbol->kind == SYMBOL_FUNC && symbol->func_info.param_types) {
        for (int i = 0; i < symbol->func_info.param_count; i++) {
            type_free(symbol->func_info.param_types[i]);
        }
        free(symbol->func_info.param_types);
    }
    
    free(symbol);
}

// ========== 作用域操作函数 ==========

Scope* scope_create(int level, Scope* parent) {
    Scope* scope = (Scope*)malloc(sizeof(Scope));
    if (!scope) {
        perror("malloc failed for Scope");
        exit(1);
    }
    
    scope->level = level;
    scope->parent = parent;
    scope->symbol_count = 0;
    scope->child_count = 0;
    scope->children = NULL;
    
    // 初始化符号哈希表
    scope->symbols = (Symbol**)calloc(SYMBOL_TABLE_SIZE, sizeof(Symbol*));
    if (!scope->symbols) {
        perror("calloc failed for symbols");
        exit(1);
    }
    
    return scope;
}

void scope_destroy(Scope* scope) {
    if (!scope) return;
    
    // 释放所有符号
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        Symbol* symbol = scope->symbols[i];
        while (symbol) {
            Symbol* next = symbol->next;
            symbol_destroy(symbol);
            symbol = next;
        }
    }
    free(scope->symbols);
    
    // 释放所有子作用域
    for (int i = 0; i < scope->child_count; i++) {
        scope_destroy(scope->children[i]);
    }
    free(scope->children);
    
    free(scope);
}

Symbol* scope_lookup(Scope* scope, const char* name) {
    if (!scope) return NULL;
    
    unsigned int hash = hash_string(name);
    Symbol* symbol = scope->symbols[hash];
    
    while (symbol) {
        if (strcmp(symbol->name, name) == 0) {
            return symbol;
        }
        symbol = symbol->next;
    }
    
    return NULL;
}

Symbol* scope_insert(Scope* scope, const char* name, SymbolKind kind, Type* type) {
    if (!scope) return NULL;
    
    unsigned int hash = hash_string(name);
    
    // 检查当前作用域是否已存在同名符号
    Symbol* existing = scope_lookup(scope, name);
    if (existing) {
        return NULL; // 重复定义
    }
    
    // 创建新符号并插入哈希表
    Symbol* symbol = symbol_create(name, kind, type, scope->level);
    symbol->next = scope->symbols[hash];
    scope->symbols[hash] = symbol;
    scope->symbol_count++;
    
    return symbol;
}

// ========== 符号表操作函数 ==========

SymbolTable* symbol_table_create() {
    SymbolTable* table = (SymbolTable*)malloc(sizeof(SymbolTable));
    if (!table) {
        perror("malloc failed for SymbolTable");
        exit(1);
    }
    
    table->global_scope = scope_create(0, NULL);
    table->current_scope = table->global_scope;
    table->current_level = 0;
    
    return table;
}

void symbol_table_destroy(SymbolTable* table) {
    if (!table) return;
    
    scope_destroy(table->global_scope);
    free(table);
}

void symbol_table_enter_scope(SymbolTable* table) {
    if (!table) return;
    
    // 创建新的子作用域
    Scope* new_scope = scope_create(table->current_level + 1, table->current_scope);
    
    // 将新作用域添加到父作用域的子作用域列表
    table->current_scope->child_count++;
    table->current_scope->children = (Scope**)realloc(
        table->current_scope->children,
        sizeof(Scope*) * table->current_scope->child_count
    );
    table->current_scope->children[table->current_scope->child_count - 1] = new_scope;
    
    // 更新当前作用域
    table->current_scope = new_scope;
    table->current_level++;
}

void symbol_table_exit_scope(SymbolTable* table) {
    if (!table || !table->current_scope->parent) return;
    
    table->current_scope = table->current_scope->parent;
    table->current_level--;
}

Scope* symbol_table_current_scope(SymbolTable* table) {
    return table ? table->current_scope : NULL;
}

Symbol* symbol_table_insert(SymbolTable* table, const char* name, SymbolKind kind, Type* type) {
    if (!table) return NULL;
    
    return scope_insert(table->current_scope, name, kind, type);
}

Symbol* symbol_table_lookup(SymbolTable* table, const char* name) {
    if (!table) return NULL;
    
    // 从当前作用域向上查找
    Scope* scope = table->current_scope;
    while (scope) {
        Symbol* symbol = scope_lookup(scope, name);
        if (symbol) {
            return symbol;
        }
        scope = scope->parent;
    }
    
    return NULL;
}

Symbol* symbol_table_lookup_current_scope(SymbolTable* table, const char* name) {
    if (!table) return NULL;
    
    return scope_lookup(table->current_scope, name);
}

void symbol_update_definition(Symbol* symbol, bool is_defined) {
    if (symbol) {
        symbol->is_defined = is_defined;
    }
}

void symbol_update_var_info(Symbol* symbol, bool is_const, int offset) {
    if (symbol && symbol->kind == SYMBOL_VAR) {
        symbol->var_info.is_const = is_const;
        symbol->var_info.offset = offset;
    }
}

void symbol_update_func_info(Symbol* symbol, Type** param_types, int param_count) {
    if (symbol && symbol->kind == SYMBOL_FUNC) {
        symbol->func_info.param_types = param_types;
        symbol->func_info.param_count = param_count;
        symbol->func_info.is_declared = true;
    }
}

void symbol_table_print(SymbolTable* table) {
    if (!table) return;
    
    printf("\n========== Symbol Table ==========\n");
    printf("Current Level: %d\n\n", table->current_level);
    
    // 递归打印作用域
    void print_scope(Scope* scope, int indent) {
        if (!scope) return;
        
        for (int i = 0; i < indent; i++) printf("  ");
        printf("Scope Level %d (symbols: %d)\n", scope->level, scope->symbol_count);
        
        // 打印该作用域的所有符号
        for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
            Symbol* symbol = scope->symbols[i];
            while (symbol) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                printf("- %s: %s %s", 
                       symbol->name, 
                       symbol_kind_str(symbol->kind),
                       type_to_string(symbol->type));
                if (symbol->is_defined) {
                    printf(" [defined]");
                } else {
                    printf(" [declared]");
                }
                printf("\n");
                symbol = symbol->next;
            }
        }
        
        // 递归打印子作用域
        for (int i = 0; i < scope->child_count; i++) {
            print_scope(scope->children[i], indent + 1);
        }
    }
    
    print_scope(table->global_scope, 0);
    printf("==================================\n\n");
}

const char* symbol_kind_str(SymbolKind kind) {
    switch (kind) {
        case SYMBOL_VAR: return "var";
        case SYMBOL_FUNC: return "func";
        case SYMBOL_PARAM: return "param";
        case SYMBOL_TYPE: return "type";
        default: return "unknown";
    }
}
