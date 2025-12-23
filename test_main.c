#include "ast.h"
#include "symbol_table.h"
#include "type_checker.h"
#include "semantic_analyzer.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== 测试符号表 ===\n");
    
    // 创建符号表
    SymbolTable* table = symbol_table_create();
    
    // 插入一些符号
    Type* int_type = type_create_basic(TYPE_INT);
    symbol_table_insert(table, "x", SYMBOL_VAR, int_type);
    symbol_table_insert(table, "y", SYMBOL_VAR, int_type);
    
    // 查找符号
    Symbol* sym_x = symbol_table_lookup(table, "x");
    if (sym_x) {
        printf("找到符号 'x': %s %s\n", symbol_kind_str(sym_x->kind), type_to_string(sym_x->type));
    }
    
    // 进入新作用域
    symbol_table_enter_scope(table);
    symbol_table_insert(table, "z", SYMBOL_VAR, int_type);
    
    // 打印符号表
    symbol_table_print(table);
    
    // 退出作用域
    symbol_table_exit_scope(table);
    
    printf("\n=== 测试AST构建 ===\n");
    
    // 创建AST: x + y * 2
    ASTNode* x = ast_create_identifier("x", 1);
    ASTNode* y = ast_create_identifier("y", 1);
    ASTNode* two = ast_create_int_literal(2, 1);
    ASTNode* mul = ast_create_binary_op(OP_MUL, y, two, 1);
    ASTNode* add = ast_create_binary_op(OP_ADD, x, mul, 1);
    
    printf("AST结构:\n");
    ast_print(add, 0);
    
    printf("\n=== 测试类型检查 ===\n");
    
    // 创建类型检查器
    TypeChecker* checker = type_checker_create();
    
    // 插入符号到类型检查器的符号表
    symbol_table_insert(checker->symbol_table, "x", SYMBOL_VAR, int_type);
    Symbol* sym_x_checker = symbol_table_lookup(checker->symbol_table, "x");
    if (sym_x_checker) {
        symbol_update_definition(sym_x_checker, true);
    }
    
    symbol_table_insert(checker->symbol_table, "y", SYMBOL_VAR, int_type);
    Symbol* sym_y_checker = symbol_table_lookup(checker->symbol_table, "y");
    if (sym_y_checker) {
        symbol_update_definition(sym_y_checker, true);
    }
    
    // 进行类型检查
    Type* result_type = type_check_node(checker, add);
    
    if (result_type) {
        printf("表达式类型: %s\n", type_to_string(result_type));
    }
    
    if (!checker->has_errors) {
        printf("\033[32m类型检查通过\033[0m\n");
    } else {
        printf("\033[31m类型检查失败\033[0m\n");
    }
    
    printf("\n=== 测试语义分析 ===\n");
    
    // 创建一个简单的程序AST
    ASTNode* var_decl_x = ast_create_var_decl("a", type_create_basic(TYPE_INT), 
                                               ast_create_int_literal(10, 2), 2);
    ASTNode* var_decl_y = ast_create_var_decl("b", type_create_basic(TYPE_INT), 
                                               ast_create_int_literal(20, 3), 3);
    
    ASTNode* a_id = ast_create_identifier("a", 4);
    ASTNode* b_id = ast_create_identifier("b", 4);
    ASTNode* expr = ast_create_binary_op(OP_ADD, a_id, b_id, 4);
    ASTNode* expr_stmt = ast_create_expr_stmt(expr, 4);
    
    ASTNode** decls = (ASTNode**)malloc(sizeof(ASTNode*) * 3);
    decls[0] = var_decl_x;
    decls[1] = var_decl_y;
    decls[2] = expr_stmt;
    
    ASTNode* program = ast_create_program(decls, 3);
    
    printf("程序AST结构:\n");
    ast_print(program, 0);
    
    // 创建语义分析器
    SemanticAnalyzer* analyzer = semantic_analyzer_create();
    
    // 进行语义分析
    bool success = semantic_analyze_program(analyzer, program);
    
    if (success) {
        printf("\n\033[32m语义分析通过!\033[0m\n");
    } else {
        printf("\n\033[31m语义分析失败!\033[0m\n");
    }
    
    // 清理资源
    semantic_analyzer_destroy(analyzer);
    type_checker_destroy(checker);
    symbol_table_destroy(table);
    ast_free(program);
    ast_free(add);
    type_free(int_type);
    
    return 0;
}
