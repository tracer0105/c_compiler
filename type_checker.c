#include "type_checker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ========== 类型检查器创建和销毁 ==========

TypeChecker* type_checker_create() {
    TypeChecker* checker = (TypeChecker*)malloc(sizeof(TypeChecker));
    if (!checker) {
        perror("malloc failed for TypeChecker");
        exit(1);
    }
    
    checker->symbol_table = symbol_table_create();
    checker->has_errors = false;
    checker->error_count = 0;
    checker->current_function_return_type = NULL;
    
    return checker;
}

void type_checker_destroy(TypeChecker* checker) {
    if (!checker) return;
    
    symbol_table_destroy(checker->symbol_table);
    free(checker);
}

// ========== 错误报告 ==========

void type_error(TypeChecker* checker, int line, const char* format, ...) {
    fprintf(stderr, "\033[31m类型错误 (行 %d): ", line);
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\033[0m\n");
    
    checker->has_errors = true;
    checker->error_count++;
}

void type_warning(TypeChecker* checker, int line, const char* format, ...) {
    (void)checker;
    fprintf(stderr, "\033[33m类型警告 (行 %d): ", line);
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\033[0m\n");
}

// ========== 工具函数 ==========

bool is_arithmetic_type(Type* type) {
    if (!type) return false;
    return type->base_type == TYPE_INT || 
           type->base_type == TYPE_FLOAT ||
           type->base_type == TYPE_CHAR;
}

bool is_integer_type(Type* type) {
    if (!type) return false;
    return type->base_type == TYPE_INT || type->base_type == TYPE_CHAR;
}

bool is_boolean_type(Type* type) {
    if (!type) return false;
    return type->base_type == TYPE_BOOL || is_integer_type(type);
}

bool is_lvalue(ASTNode* node) {
    if (!node) return false;
    return node->node_type == AST_IDENTIFIER || 
           node->node_type == AST_ARRAY_ACCESS ||
           (node->node_type == AST_UNARY_OP && node->data.unary_op.op == OP_DEREF);
}

// ========== 类型兼容性检查 ==========

bool check_type_compatibility(TypeChecker* checker, Type* expected, Type* actual, int line) {
    if (!expected || !actual) return false;
    
    if (type_equals(expected, actual)) {
        return true;
    }
    
    if (type_is_compatible(expected, actual)) {
        return true;
    }
    
    type_error(checker, line, "类型不兼容: 期望 '%s', 实际 '%s'",
               type_to_string(expected), type_to_string(actual));
    return false;
}

bool check_assignment_compatibility(TypeChecker* checker, Type* lvalue_type, Type* rvalue_type, int line) {
    if (!lvalue_type || !rvalue_type) return false;
    
    // 完全相同的类型
    if (type_equals(lvalue_type, rvalue_type)) {
        return true;
    }
    
    // 数值类型之间的隐式转换
    if (is_arithmetic_type(lvalue_type) && is_arithmetic_type(rvalue_type)) {
        // 警告: 可能的精度损失
        if (lvalue_type->base_type == TYPE_INT && rvalue_type->base_type == TYPE_FLOAT) {
            type_warning(checker, line, "从 float 到 int 的隐式转换可能导致精度损失");
        }
        return true;
    }
    
    type_error(checker, line, "赋值类型不兼容: 左值类型 '%s', 右值类型 '%s'",
               type_to_string(lvalue_type), type_to_string(rvalue_type));
    return false;
}

// ========== 类型推论 ==========

Type* infer_binary_op_type(TypeChecker* checker, BinaryOp op, Type* left_type, Type* right_type, int line) {
    if (!left_type || !right_type) {
        return type_create_basic(TYPE_ERROR);
    }
    
    switch (op) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
            // 算术运算要求两个操作数都是算术类型
            if (!is_arithmetic_type(left_type) || !is_arithmetic_type(right_type)) {
                type_error(checker, line, "算术运算符要求算术类型操作数");
                return type_create_basic(TYPE_ERROR);
            }
            
            // 类型提升: 如果有float则结果为float
            if (left_type->base_type == TYPE_FLOAT || right_type->base_type == TYPE_FLOAT) {
                return type_create_basic(TYPE_FLOAT);
            }
            return type_create_basic(TYPE_INT);
            
        case OP_LT:
        case OP_LE:
        case OP_GT:
        case OP_GE:
            // 关系运算要求算术类型
            if (!is_arithmetic_type(left_type) || !is_arithmetic_type(right_type)) {
                type_error(checker, line, "关系运算符要求算术类型操作数");
                return type_create_basic(TYPE_ERROR);
            }
            return type_create_basic(TYPE_BOOL);
            
        case OP_EQ:
        case OP_NE:
            // 相等性运算要求类型兼容
            if (!type_is_compatible(left_type, right_type)) {
                type_error(checker, line, "相等性运算符要求兼容类型");
                return type_create_basic(TYPE_ERROR);
            }
            return type_create_basic(TYPE_BOOL);
            
        case OP_AND:
        case OP_OR:
            // 逻辑运算要求布尔类型
            if (!is_boolean_type(left_type) || !is_boolean_type(right_type)) {
                type_error(checker, line, "逻辑运算符要求布尔类型操作数");
                return type_create_basic(TYPE_ERROR);
            }
            return type_create_basic(TYPE_BOOL);
            
        default:
            type_error(checker, line, "未知的二元运算符");
            return type_create_basic(TYPE_ERROR);
    }
}

Type* infer_unary_op_type(TypeChecker* checker, UnaryOp op, Type* operand_type, int line) {
    if (!operand_type) {
        return type_create_basic(TYPE_ERROR);
    }
    
    switch (op) {
        case OP_NEG:
            // 负号要求算术类型
            if (!is_arithmetic_type(operand_type)) {
                type_error(checker, line, "负号运算符要求算术类型操作数");
                return type_create_basic(TYPE_ERROR);
            }
            return type_copy(operand_type);
            
        case OP_NOT:
            // 逻辑非要求布尔类型
            if (!is_boolean_type(operand_type)) {
                type_error(checker, line, "逻辑非运算符要求布尔类型操作数");
                return type_create_basic(TYPE_ERROR);
            }
            return type_create_basic(TYPE_BOOL);
            
        case OP_INC:
        case OP_DEC:
            // 自增自减要求整数类型
            if (!is_integer_type(operand_type)) {
                type_error(checker, line, "自增/自减运算符要求整数类型操作数");
                return type_create_basic(TYPE_ERROR);
            }
            return type_copy(operand_type);
            
        case OP_ADDR:
            // 取地址运算符返回指针类型
            return type_create_pointer(operand_type);
            
        case OP_DEREF:
            // 解引用要求指针类型
            if (operand_type->base_type != TYPE_POINTER) {
                type_error(checker, line, "解引用运算符要求指针类型操作数");
                return type_create_basic(TYPE_ERROR);
            }
            return type_copy(operand_type->pointer_info.pointed_type);
            
        default:
            type_error(checker, line, "未知的一元运算符");
            return type_create_basic(TYPE_ERROR);
    }
}

// ========== 具体节点的类型检查 ==========

Type* type_check_binary_op(TypeChecker* checker, ASTNode* node) {
    if (!node || node->node_type != AST_BINARY_OP) return NULL;
    
    // 递归检查左右操作数
    Type* left_type = type_check_node(checker, node->data.binary_op.left);
    Type* right_type = type_check_node(checker, node->data.binary_op.right);
    
    // 推论结果类型
    Type* result_type = infer_binary_op_type(checker, node->data.binary_op.op, 
                                             left_type, right_type, node->line);
    
    // 设置节点类型
    node->type = result_type;
    
    return result_type;
}

Type* type_check_unary_op(TypeChecker* checker, ASTNode* node) {
    if (!node || node->node_type != AST_UNARY_OP) return NULL;
    
    // 递归检查操作数
    Type* operand_type = type_check_node(checker, node->data.unary_op.operand);
    
    // 检查自增自减是否作用于左值
    if (node->data.unary_op.op == OP_INC || node->data.unary_op.op == OP_DEC) {
        if (!is_lvalue(node->data.unary_op.operand)) {
            type_error(checker, node->line, "自增/自减运算符要求左值");
            return type_create_basic(TYPE_ERROR);
        }
    }
    
    // 推论结果类型
    Type* result_type = infer_unary_op_type(checker, node->data.unary_op.op, 
                                            operand_type, node->line);
    
    // 设置节点类型
    node->type = result_type;
    
    return result_type;
}

Type* type_check_identifier(TypeChecker* checker, ASTNode* node) {
    if (!node || node->node_type != AST_IDENTIFIER) return NULL;
    
    // 在符号表中查找标识符
    Symbol* symbol = symbol_table_lookup(checker->symbol_table, node->data.identifier.name);
    
    if (!symbol) {
        type_error(checker, node->line, "未声明的标识符 '%s'", node->data.identifier.name);
        return type_create_basic(TYPE_ERROR);
    }
    
    // 检查是否已定义
    if (!symbol->is_defined && symbol->kind == SYMBOL_VAR) {
        type_warning(checker, node->line, "使用了未初始化的变量 '%s'", node->data.identifier.name);
    }
    
    // 设置节点类型
    node->type = type_copy(symbol->type);
    
    return node->type;
}

Type* type_check_array_access(TypeChecker* checker, ASTNode* node) {
    if (!node || node->node_type != AST_ARRAY_ACCESS) return NULL;
    
    // 检查数组表达式
    Type* array_type = type_check_node(checker, node->data.array_access.array);
    
    if (array_type->base_type != TYPE_ARRAY) {
        type_error(checker, node->line, "下标运算符要求数组类型");
        return type_create_basic(TYPE_ERROR);
    }
    
    // 检查索引表达式
    Type* index_type = type_check_node(checker, node->data.array_access.index);
    
    if (!is_integer_type(index_type)) {
        type_error(checker, node->line, "数组下标必须是整数类型");
        return type_create_basic(TYPE_ERROR);
    }
    
    // 数组访问的结果类型是元素类型
    node->type = type_copy(array_type->array_info.element_type);
    
    return node->type;
}

Type* type_check_func_call(TypeChecker* checker, ASTNode* node) {
    if (!node || node->node_type != AST_FUNC_CALL) return NULL;
    
    // 在符号表中查找函数
    Symbol* func_symbol = symbol_table_lookup(checker->symbol_table, node->data.func_call.func_name);
    
    if (!func_symbol) {
        type_error(checker, node->line, "未声明的函数 '%s'", node->data.func_call.func_name);
        return type_create_basic(TYPE_ERROR);
    }
    
    if (func_symbol->kind != SYMBOL_FUNC) {
        type_error(checker, node->line, "'%s' 不是函数", node->data.func_call.func_name);
        return type_create_basic(TYPE_ERROR);
    }
    
    // 检查参数数量
    if (node->data.func_call.arg_count != func_symbol->func_info.param_count) {
        type_error(checker, node->line, "函数 '%s' 参数数量不匹配: 期望 %d, 实际 %d",
                   node->data.func_call.func_name,
                   func_symbol->func_info.param_count,
                   node->data.func_call.arg_count);
        return type_create_basic(TYPE_ERROR);
    }
    
    // 检查每个参数的类型
    for (int i = 0; i < node->data.func_call.arg_count; i++) {
        Type* arg_type = type_check_node(checker, node->data.func_call.args[i]);
        Type* param_type = func_symbol->func_info.param_types[i];
        
        if (!check_type_compatibility(checker, param_type, arg_type, node->line)) {
            type_error(checker, node->line, "函数 '%s' 第 %d 个参数类型不匹配",
                       node->data.func_call.func_name, i + 1);
        }
    }
    
    // 函数调用的结果类型是函数的返回类型
    node->type = type_copy(func_symbol->type);
    
    return node->type;
}

Type* type_check_var_decl(TypeChecker* checker, ASTNode* node) {
    if (!node || node->node_type != AST_VAR_DECL) return NULL;
    
    // 检查是否重复声明
    Symbol* existing = symbol_table_lookup_current_scope(checker->symbol_table, 
                                                         node->data.var_decl.var_name);
    if (existing) {
        type_error(checker, node->line, "重复声明的标识符 '%s'", node->data.var_decl.var_name);
        return type_create_basic(TYPE_ERROR);
    }
    
    // 插入符号表
    Symbol* symbol = symbol_table_insert(checker->symbol_table, 
                                        node->data.var_decl.var_name,
                                        SYMBOL_VAR,
                                        node->data.var_decl.var_type);
    
    if (!symbol) {
        type_error(checker, node->line, "插入符号表失败");
        return type_create_basic(TYPE_ERROR);
    }
    
    // 如果有初始值,检查初始值类型
    if (node->data.var_decl.init_value) {
        Type* init_type = type_check_node(checker, node->data.var_decl.init_value);
        
        if (!check_assignment_compatibility(checker, node->data.var_decl.var_type, 
                                           init_type, node->line)) {
            type_error(checker, node->line, "变量 '%s' 初始化类型不匹配",
                       node->data.var_decl.var_name);
        }
        
        // 标记为已定义
        symbol_update_definition(symbol, true);
    }
    
    return node->data.var_decl.var_type;
}

Type* type_check_func_decl(TypeChecker* checker, ASTNode* node) {
    if (!node || node->node_type != AST_FUNC_DECL) return NULL;
    
    // 检查是否重复声明
    Symbol* existing = symbol_table_lookup_current_scope(checker->symbol_table,
                                                         node->data.func_decl.func_name);
    
    if (existing) {
        // 如果已经声明过,检查签名是否一致
        if (existing->kind != SYMBOL_FUNC) {
            type_error(checker, node->line, "'%s' 已被声明为非函数", node->data.func_decl.func_name);
            return type_create_basic(TYPE_ERROR);
        }
        
        // 检查返回类型
        if (!type_equals(existing->type, node->data.func_decl.return_type)) {
            type_error(checker, node->line, "函数 '%s' 返回类型不一致", node->data.func_decl.func_name);
            return type_create_basic(TYPE_ERROR);
        }
        
        // 检查参数数量
        if (existing->func_info.param_count != node->data.func_decl.param_count) {
            type_error(checker, node->line, "函数 '%s' 参数数量不一致", node->data.func_decl.func_name);
            return type_create_basic(TYPE_ERROR);
        }
        
        // 如果有函数体,标记为已定义
        if (node->data.func_decl.body) {
            if (existing->is_defined) {
                type_error(checker, node->line, "函数 '%s' 重复定义", node->data.func_decl.func_name);
                return type_create_basic(TYPE_ERROR);
            }
            symbol_update_definition(existing, true);
        }
    } else {
        // 插入符号表
        Symbol* symbol = symbol_table_insert(checker->symbol_table,
                                            node->data.func_decl.func_name,
                                            SYMBOL_FUNC,
                                            node->data.func_decl.return_type);
        
        if (!symbol) {
            type_error(checker, node->line, "插入符号表失败");
            return type_create_basic(TYPE_ERROR);
        }
        
        // 设置函数参数信息
        Type** param_types = (Type**)malloc(sizeof(Type*) * node->data.func_decl.param_count);
        for (int i = 0; i < node->data.func_decl.param_count; i++) {
            param_types[i] = type_copy(node->data.func_decl.params[i]->type);
        }
        symbol_update_func_info(symbol, param_types, node->data.func_decl.param_count);
        
        // 如果有函数体,标记为已定义
        if (node->data.func_decl.body) {
            symbol_update_definition(symbol, true);
        }
    }
    
    // 如果有函数体,进入新作用域检查函数体
    if (node->data.func_decl.body) {
        symbol_table_enter_scope(checker->symbol_table);
        
        // 将参数加入符号表
        for (int i = 0; i < node->data.func_decl.param_count; i++) {
            ASTNode* param = node->data.func_decl.params[i];
            Symbol* param_symbol = symbol_table_insert(checker->symbol_table,
                                                       param->data.var_decl.var_name,
                                                       SYMBOL_PARAM,
                                                       param->data.var_decl.var_type);
            if (param_symbol) {
                symbol_update_definition(param_symbol, true);
            }
        }
        
        // 保存当前函数返回类型
        Type* saved_return_type = checker->current_function_return_type;
        checker->current_function_return_type = node->data.func_decl.return_type;
        
        // 检查函数体
        type_check_node(checker, node->data.func_decl.body);
        
        // 恢复之前的返回类型
        checker->current_function_return_type = saved_return_type;
        
        symbol_table_exit_scope(checker->symbol_table);
    }
    
    return node->data.func_decl.return_type;
}

Type* type_check_assign_stmt(TypeChecker* checker, ASTNode* node) {
    if (!node || node->node_type != AST_ASSIGN_STMT) return NULL;
    
    // 检查左值
    if (!is_lvalue(node->data.assign_stmt.lvalue)) {
        type_error(checker, node->line, "赋值运算符左侧必须是左值");
        return type_create_basic(TYPE_ERROR);
    }
    
    Type* lvalue_type = type_check_node(checker, node->data.assign_stmt.lvalue);
    Type* rvalue_type = type_check_node(checker, node->data.assign_stmt.rvalue);
    
    // 检查类型兼容性
    check_assignment_compatibility(checker, lvalue_type, rvalue_type, node->line);
    
    // 如果左值是标识符,标记为已定义
    if (node->data.assign_stmt.lvalue->node_type == AST_IDENTIFIER) {
        Symbol* symbol = symbol_table_lookup(checker->symbol_table,
                                            node->data.assign_stmt.lvalue->data.identifier.name);
        if (symbol) {
            symbol_update_definition(symbol, true);
        }
    }
    
    node->type = type_copy(lvalue_type);
    return node->type;
}

Type* type_check_if_stmt(TypeChecker* checker, ASTNode* node) {
    if (!node || node->node_type != AST_IF_STMT) return NULL;
    
    // 检查条件表达式
    Type* condition_type = type_check_node(checker, node->data.if_stmt.condition);
    
    if (!is_boolean_type(condition_type)) {
        type_error(checker, node->line, "if语句条件必须是布尔类型");
    }
    
    // 检查then分支
    symbol_table_enter_scope(checker->symbol_table);
    type_check_node(checker, node->data.if_stmt.then_branch);
    symbol_table_exit_scope(checker->symbol_table);
    
    // 检查else分支(如果存在)
    if (node->data.if_stmt.else_branch) {
        symbol_table_enter_scope(checker->symbol_table);
        type_check_node(checker, node->data.if_stmt.else_branch);
        symbol_table_exit_scope(checker->symbol_table);
    }
    
    return type_create_basic(TYPE_VOID);
}

Type* type_check_while_stmt(TypeChecker* checker, ASTNode* node) {
    if (!node || node->node_type != AST_WHILE_STMT) return NULL;
    
    // 检查条件表达式
    Type* condition_type = type_check_node(checker, node->data.while_stmt.condition);
    
    if (!is_boolean_type(condition_type)) {
        type_error(checker, node->line, "while语句条件必须是布尔类型");
    }
    
    // 检查循环体
    symbol_table_enter_scope(checker->symbol_table);
    type_check_node(checker, node->data.while_stmt.body);
    symbol_table_exit_scope(checker->symbol_table);
    
    return type_create_basic(TYPE_VOID);
}

Type* type_check_return_stmt(TypeChecker* checker, ASTNode* node) {
    if (!node || node->node_type != AST_RETURN_STMT) return NULL;
    
    if (!checker->current_function_return_type) {
        type_error(checker, node->line, "return语句只能在函数内使用");
        return type_create_basic(TYPE_ERROR);
    }
    
    // 检查返回值
    if (node->data.return_stmt.return_value) {
        Type* return_type = type_check_node(checker, node->data.return_stmt.return_value);
        
        if (!check_type_compatibility(checker, checker->current_function_return_type,
                                     return_type, node->line)) {
            type_error(checker, node->line, "return语句类型不匹配");
        }
    } else {
        // 没有返回值,检查函数返回类型是否为void
        if (checker->current_function_return_type->base_type != TYPE_VOID) {
            type_error(checker, node->line, "函数应该返回 '%s' 类型的值",
                       type_to_string(checker->current_function_return_type));
        }
    }
    
    return type_create_basic(TYPE_VOID);
}

Type* type_check_compound_stmt(TypeChecker* checker, ASTNode* node) {
    if (!node || node->node_type != AST_COMPOUND_STMT) return NULL;
    
    // 检查所有语句
    for (int i = 0; i < node->data.compound_stmt.stmt_count; i++) {
        type_check_node(checker, node->data.compound_stmt.statements[i]);
    }
    
    return type_create_basic(TYPE_VOID);
}

Type* type_check_expr_stmt(TypeChecker* checker, ASTNode* node) {
    if (!node || node->node_type != AST_EXPR_STMT) return NULL;
    
    if (node->data.expr_stmt.expr) {
        return type_check_node(checker, node->data.expr_stmt.expr);
    }
    
    return type_create_basic(TYPE_VOID);
}

// ========== 主类型检查函数 ==========

Type* type_check_node(TypeChecker* checker, ASTNode* node) {
    if (!node) return NULL;
    
    switch (node->node_type) {
        case AST_BINARY_OP:
            return type_check_binary_op(checker, node);
        case AST_UNARY_OP:
            return type_check_unary_op(checker, node);
        case AST_LITERAL:
            return node->type; // 字面量类型已在创建时设置
        case AST_IDENTIFIER:
            return type_check_identifier(checker, node);
        case AST_ARRAY_ACCESS:
            return type_check_array_access(checker, node);
        case AST_FUNC_CALL:
            return type_check_func_call(checker, node);
        case AST_VAR_DECL:
            return type_check_var_decl(checker, node);
        case AST_FUNC_DECL:
            return type_check_func_decl(checker, node);
        case AST_ASSIGN_STMT:
            return type_check_assign_stmt(checker, node);
        case AST_IF_STMT:
            return type_check_if_stmt(checker, node);
        case AST_WHILE_STMT:
            return type_check_while_stmt(checker, node);
        case AST_RETURN_STMT:
            return type_check_return_stmt(checker, node);
        case AST_COMPOUND_STMT:
            return type_check_compound_stmt(checker, node);
        case AST_EXPR_STMT:
            return type_check_expr_stmt(checker, node);
        default:
            type_error(checker, node->line, "未知的AST节点类型");
            return type_create_basic(TYPE_ERROR);
    }
}

bool type_check_program(TypeChecker* checker, ASTNode* program) {
    if (!checker || !program || program->node_type != AST_PROGRAM) {
        return false;
    }
    
    printf("\n========== 开始类型检查 ==========\n");
    
    // 检查所有全局声明
    for (int i = 0; i < program->data.program.decl_count; i++) {
        type_check_node(checker, program->data.program.declarations[i]);
    }
    
    printf("========== 类型检查完成 ==========\n");
    
    if (checker->has_errors) {
        printf("\033[31m发现 %d 个类型错误\033[0m\n", checker->error_count);
        return false;
    } else {
        printf("\033[32m类型检查通过,没有发现错误\033[0m\n");
        return true;
    }
}
