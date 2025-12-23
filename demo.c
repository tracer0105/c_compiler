#include "ast.h"
#include "symbol_table.h"
#include "type_checker.h"
#include "semantic_analyzer.h"
#include <stdio.h>
#include <stdlib.h>

void demo_symbol_table() {
    printf("\n========================================\n");
    printf("演示1: 符号表功能\n");
    printf("========================================\n");
    
    SymbolTable* table = symbol_table_create();
    
    // 全局作用域
    printf("\n[全局作用域]\n");
    Type* int_type = type_create_basic(TYPE_INT);
    Type* float_type = type_create_basic(TYPE_FLOAT);
    
    symbol_table_insert(table, "global_var", SYMBOL_VAR, int_type);
    symbol_table_insert(table, "pi", SYMBOL_VAR, float_type);
    
    Symbol* global_var = symbol_table_lookup(table, "global_var");
    if (global_var) {
        printf("✓ 插入并查找全局变量 'global_var': %s\n", type_to_string(global_var->type));
    }
    
    // 进入函数作用域   
    printf("\n[进入函数作用域]\n");
    symbol_table_enter_scope(table);
    
    symbol_table_insert(table, "local_var", SYMBOL_VAR, int_type);
    symbol_table_insert(table, "temp", SYMBOL_VAR, float_type);
    
    Symbol* local_var = symbol_table_lookup(table, "local_var");
    if (local_var) {
        printf("✓ 插入并查找局部变量 'local_var': %s (作用域层级: %d)\n", 
               type_to_string(local_var->type), local_var->scope_level);
    }
    
    // 测试向上查找
    Symbol* found_global = symbol_table_lookup(table, "global_var");
    if (found_global) {
        printf("✓ 在局部作用域中查找全局变量 'global_var': 成功\n");
    }
    
    // 进入嵌套作用域
    printf("\n[进入嵌套作用域]\n");
    symbol_table_enter_scope(table);
    
    symbol_table_insert(table, "nested_var", SYMBOL_VAR, int_type);
    
    // 打印完整符号表
    symbol_table_print(table);
    
    // 退出作用域
    printf("[退出嵌套作用域]\n");
    symbol_table_exit_scope(table);
    
    printf("[退出函数作用域]\n");
    symbol_table_exit_scope(table);
    
    symbol_table_destroy(table);
    type_free(int_type);
    type_free(float_type);
}

void demo_type_system() {
    printf("\n========================================\n");
    printf("演示2: 类型系统\n");
    printf("========================================\n");
    
    // 基础类型
    printf("\n[基础类型]\n");
    Type* int_type_base = type_create_basic(TYPE_INT);
    Type* float_type_base = type_create_basic(TYPE_FLOAT);
    Type* void_type = type_create_basic(TYPE_VOID);
    
    printf("✓ int类型: %s\n", type_to_string(int_type_base));
    printf("✓ float类型: %s\n", type_to_string(float_type_base));
    printf("✓ void类型: %s\n", type_to_string(void_type));
    
    // 数组类型
    printf("\n[数组类型]\n");
    Type* int_type = type_create_basic(TYPE_INT);
    Type* int_array = type_create_array(int_type, 10);
    printf("✓ int数组: %s\n", type_to_string(int_array));
    
    // 指针类型
    printf("\n[指针类型]\n");
    Type* int_type2 = type_create_basic(TYPE_INT);
    Type* int_ptr = type_create_pointer(int_type2);
    printf("✓ int指针: %s\n", type_to_string(int_ptr));
    
    // 函数类型
    printf("\n[函数类型]\n");
    Type* int_type3 = type_create_basic(TYPE_INT);
    Type* float_type = type_create_basic(TYPE_FLOAT);
    Type** param_types = (Type**)malloc(sizeof(Type*) * 2);
    param_types[0] = int_type3;
    param_types[1] = float_type;
    Type* ret_type = type_create_basic(TYPE_INT);
    Type* func_type = type_create_function(ret_type, param_types, 2);
    printf("✓ 函数类型: %s\n", type_to_string(func_type));
    
    // 类型兼容性
    printf("\n[类型兼容性测试]\n");
    printf("✓ int == int: %s\n", type_equals(int_type_base, int_type_base) ? "是" : "否");
    printf("✓ int == float: %s\n", type_equals(int_type_base, float_type_base) ? "是" : "否");
    printf("✓ int 兼容 float: %s\n", type_is_compatible(int_type_base, float_type_base) ? "是" : "否");
    
    // 释放类型
    type_free(int_array);
    type_free(int_ptr);
    type_free(func_type);
    type_free(void_type);
    type_free(int_type_base);
    type_free(float_type_base);
}

void demo_ast_building() {
    printf("\n========================================\n");
    printf("演示3: AST构建\n");
    printf("========================================\n");
    
    // 构建表达式: (a + b) * (c - 5)
    printf("\n[构建表达式: (a + b) * (c - 5)]\n");
    
    ASTNode* a = ast_create_identifier("a", 1);
    ASTNode* b = ast_create_identifier("b", 1);
    ASTNode* c = ast_create_identifier("c", 1);
    ASTNode* five = ast_create_int_literal(5, 1);
    
    ASTNode* add = ast_create_binary_op(OP_ADD, a, b, 1);
    ASTNode* sub = ast_create_binary_op(OP_SUB, c, five, 1);
    ASTNode* mul = ast_create_binary_op(OP_MUL, add, sub, 1);
    
    printf("AST结构:\n");
    ast_print(mul, 0);
    
    ast_free(mul);
}

void demo_type_checking() {
    printf("\n========================================\n");
    printf("演示4: 类型检查\n");
    printf("========================================\n");
    
    TypeChecker* checker = type_checker_create();
    Type* int_type = type_create_basic(TYPE_INT);
    
    // 插入符号
    symbol_table_insert(checker->symbol_table, "x", SYMBOL_VAR, int_type);
    symbol_table_insert(checker->symbol_table, "y", SYMBOL_VAR, int_type);
    
    Symbol* x_sym = symbol_table_lookup(checker->symbol_table, "x");
    Symbol* y_sym = symbol_table_lookup(checker->symbol_table, "y");
    symbol_update_definition(x_sym, true);
    symbol_update_definition(y_sym, true);
    
    // 测试1: 正确的表达式
    printf("\n[测试1: x + y * 2]\n");
    ASTNode* x1 = ast_create_identifier("x", 1);
    ASTNode* y1 = ast_create_identifier("y", 1);
    ASTNode* two = ast_create_int_literal(2, 1);
    ASTNode* mul1 = ast_create_binary_op(OP_MUL, y1, two, 1);
    ASTNode* add1 = ast_create_binary_op(OP_ADD, x1, mul1, 1);
    
    Type* result1 = type_check_node(checker, add1);
    if (result1 && !checker->has_errors) {
        printf("✓ 类型检查通过, 结果类型: %s\n", type_to_string(result1));
    }
    
    // 测试2: 未声明的变量
    printf("\n[测试2: z + 1 (z未声明)]\n");
    checker->has_errors = false;
    checker->error_count = 0;
    
    ASTNode* z = ast_create_identifier("z", 2);
    ASTNode* one = ast_create_int_literal(1, 2);
    ASTNode* add2 = ast_create_binary_op(OP_ADD, z, one, 2);
    
    type_check_node(checker, add2);
    if (checker->has_errors) {
        printf("✓ 正确检测到错误\n");
    }
    
    ast_free(add1);
    ast_free(add2);
    type_checker_destroy(checker);
    type_free(int_type);
}

void demo_semantic_analysis() {
    printf("\n========================================\n");
    printf("演示5: 完整语义分析\n");
    printf("========================================\n");
    
    // 创建程序: int a = 10; int b = 20; int c = a + b;
    printf("\n[分析程序]\n");
    printf("int a = 10;\n");
    printf("int b = 20;\n");
    printf("int c = a + b;\n\n");
    
    Type* int_type = type_create_basic(TYPE_INT);
    
    ASTNode* decl_a = ast_create_var_decl("a", type_copy(int_type), 
                                          ast_create_int_literal(10, 1), 1);
    ASTNode* decl_b = ast_create_var_decl("b", type_copy(int_type), 
                                          ast_create_int_literal(20, 2), 2);
    
    ASTNode* a_id = ast_create_identifier("a", 3);
    ASTNode* b_id = ast_create_identifier("b", 3);
    ASTNode* add_expr = ast_create_binary_op(OP_ADD, a_id, b_id, 3);
    ASTNode* decl_c = ast_create_var_decl("c", type_copy(int_type), add_expr, 3);
    
    ASTNode** decls = (ASTNode**)malloc(sizeof(ASTNode*) * 3);
    decls[0] = decl_a;
    decls[1] = decl_b;
    decls[2] = decl_c;
    
    ASTNode* program = ast_create_program(decls, 3);
    
    SemanticAnalyzer* analyzer = semantic_analyzer_create();
    bool success = semantic_analyze_program(analyzer, program);
    
    if (success) {
        printf("\n✓ 语义分析成功\n");
        symbol_table_print(analyzer->symbol_table);
    }
    
    semantic_analyzer_destroy(analyzer);
    ast_free(program);
    type_free(int_type);
}

void demo_error_detection() {
    printf("\n========================================\n");
    printf("演示6: 错误检测\n");
    printf("========================================\n");
    
    SemanticAnalyzer* analyzer = semantic_analyzer_create();
    Type* int_type = type_create_basic(TYPE_INT);
    
    // 错误1: 重复声明
    printf("\n[错误1: 重复声明变量]\n");
    printf("int x = 1;\n");
    printf("int x = 2;\n\n");
    
    ASTNode* decl1 = ast_create_var_decl("x", type_copy(int_type), 
                                         ast_create_int_literal(1, 1), 1);
    ASTNode* decl2 = ast_create_var_decl("x", type_copy(int_type), 
                                         ast_create_int_literal(2, 2), 2);
    
    ASTNode** decls1 = (ASTNode**)malloc(sizeof(ASTNode*) * 2);
    decls1[0] = decl1;
    decls1[1] = decl2;
    
    ASTNode* prog1 = ast_create_program(decls1, 2);
    semantic_analyze_program(analyzer, prog1);
    
    if (analyzer->has_errors) {
        printf("✓ 正确检测到重复声明错误\n");
    }
    
    ast_free(prog1);
    
    // 错误2: 使用未声明的变量
    printf("\n[错误2: 使用未声明的变量]\n");
    printf("int result = unknown_var + 1;\n\n");
    
    semantic_analyzer_destroy(analyzer);
    analyzer = semantic_analyzer_create();
    
    ASTNode* unknown = ast_create_identifier("unknown_var", 1);
    ASTNode* one = ast_create_int_literal(1, 1);
    ASTNode* add = ast_create_binary_op(OP_ADD, unknown, one, 1);
    ASTNode* decl3 = ast_create_var_decl("result", type_copy(int_type), add, 1);
    
    ASTNode** decls2 = (ASTNode**)malloc(sizeof(ASTNode*) * 1);
    decls2[0] = decl3;
    
    ASTNode* prog2 = ast_create_program(decls2, 1);
    semantic_analyze_program(analyzer, prog2);
    
    if (analyzer->has_errors) {
        printf("✓ 正确检测到未声明变量错误\n");
    }
    
    ast_free(prog2);
    semantic_analyzer_destroy(analyzer);
    type_free(int_type);
}

int main() {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║   C编译器 - 符号表与语义分析演示      ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    demo_symbol_table();
    demo_type_system();
    demo_ast_building();
    demo_type_checking();
    demo_semantic_analysis();
    demo_error_detection();
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║          所有演示完成!                 ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n");
    
    return 0;
}
