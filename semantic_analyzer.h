#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "ast.h"
#include "symbol_table.h"
#include "type_checker.h"
#include <stdbool.h>

// ========== 语义分析器结构 ==========

typedef struct SemanticAnalyzer {
    SymbolTable* symbol_table;
    TypeChecker* type_checker;
    bool has_errors;
    int error_count;
    int warning_count;
    
    // 控制流分析
    bool in_loop;           // 是否在循环内(用于检查break/continue)
    bool in_function;       // 是否在函数内
    bool has_return;        // 当前函数是否有return语句
    
    // 常量折叠
    bool enable_constant_folding;
} SemanticAnalyzer;

// ========== 语义分析器操作函数 ==========

SemanticAnalyzer* semantic_analyzer_create();
void semantic_analyzer_destroy(SemanticAnalyzer* analyzer);

// 主要的语义分析函数
bool semantic_analyze_program(SemanticAnalyzer* analyzer, ASTNode* program);
void semantic_analyze_node(SemanticAnalyzer* analyzer, ASTNode* node);

// 具体的语义检查
void semantic_check_var_decl(SemanticAnalyzer* analyzer, ASTNode* node);
void semantic_check_func_decl(SemanticAnalyzer* analyzer, ASTNode* node);
void semantic_check_assign_stmt(SemanticAnalyzer* analyzer, ASTNode* node);
void semantic_check_return_stmt(SemanticAnalyzer* analyzer, ASTNode* node);
void semantic_check_binary_op(SemanticAnalyzer* analyzer, ASTNode* node);
void semantic_check_unary_op(SemanticAnalyzer* analyzer, ASTNode* node);
void semantic_check_func_call(SemanticAnalyzer* analyzer, ASTNode* node);

// 控制流分析
void semantic_check_control_flow(SemanticAnalyzer* analyzer, ASTNode* node);
bool check_all_paths_return(ASTNode* node);

// 常量折叠优化
ASTNode* constant_fold(SemanticAnalyzer* analyzer, ASTNode* node);
ASTNode* fold_binary_op(ASTNode* node);
ASTNode* fold_unary_op(ASTNode* node);

// 死代码检测
void detect_dead_code(SemanticAnalyzer* analyzer, ASTNode* node);
bool is_unreachable_after(ASTNode* node);

// 未使用变量检测
void check_unused_variables(SemanticAnalyzer* analyzer);

// 错误和警告报告
void semantic_error(SemanticAnalyzer* analyzer, int line, const char* format, ...);
void semantic_warning(SemanticAnalyzer* analyzer, int line, const char* format, ...);

// 工具函数
bool is_constant_expr(ASTNode* node);
int evaluate_constant_expr(ASTNode* node);

#endif // SEMANTIC_ANALYZER_H
