#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "ast.h"
#include "symbol_table.h"
#include <stdbool.h>

// ========== 类型检查器结构 ==========

typedef struct TypeChecker {
    SymbolTable* symbol_table;
    bool has_errors;
    int error_count;
    Type* current_function_return_type;  // 当前函数的返回类型(用于检查return语句)
} TypeChecker;

// ========== 类型检查器操作函数 ==========

TypeChecker* type_checker_create();
void type_checker_destroy(TypeChecker* checker);

// 主要的类型检查函数
bool type_check_program(TypeChecker* checker, ASTNode* program);
Type* type_check_node(TypeChecker* checker, ASTNode* node);

// 具体节点的类型检查
Type* type_check_binary_op(TypeChecker* checker, ASTNode* node);
Type* type_check_unary_op(TypeChecker* checker, ASTNode* node);
Type* type_check_identifier(TypeChecker* checker, ASTNode* node);
Type* type_check_array_access(TypeChecker* checker, ASTNode* node);
Type* type_check_func_call(TypeChecker* checker, ASTNode* node);
Type* type_check_var_decl(TypeChecker* checker, ASTNode* node);
Type* type_check_func_decl(TypeChecker* checker, ASTNode* node);
Type* type_check_assign_stmt(TypeChecker* checker, ASTNode* node);
Type* type_check_if_stmt(TypeChecker* checker, ASTNode* node);
Type* type_check_while_stmt(TypeChecker* checker, ASTNode* node);
Type* type_check_return_stmt(TypeChecker* checker, ASTNode* node);
Type* type_check_compound_stmt(TypeChecker* checker, ASTNode* node);
Type* type_check_expr_stmt(TypeChecker* checker, ASTNode* node);

// 类型推论函数
Type* infer_binary_op_type(TypeChecker* checker, BinaryOp op, Type* left_type, Type* right_type, int line);
Type* infer_unary_op_type(TypeChecker* checker, UnaryOp op, Type* operand_type, int line);

// 类型兼容性检查
bool check_type_compatibility(TypeChecker* checker, Type* expected, Type* actual, int line);
bool check_assignment_compatibility(TypeChecker* checker, Type* lvalue_type, Type* rvalue_type, int line);

// 错误报告
void type_error(TypeChecker* checker, int line, const char* format, ...);
void type_warning(TypeChecker* checker, int line, const char* format, ...);

// 工具函数
bool is_arithmetic_type(Type* type);
bool is_integer_type(Type* type);
bool is_boolean_type(Type* type);
bool is_lvalue(ASTNode* node);

#endif // TYPE_CHECKER_H
