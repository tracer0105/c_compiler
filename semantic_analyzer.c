#include "semantic_analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ========== 语义分析器创建和销毁 ==========

SemanticAnalyzer* semantic_analyzer_create() {
    SemanticAnalyzer* analyzer = (SemanticAnalyzer*)malloc(sizeof(SemanticAnalyzer));
    if (!analyzer) {
        perror("malloc failed for SemanticAnalyzer");
        exit(1);
    }
    
    analyzer->symbol_table = symbol_table_create();
    analyzer->type_checker = type_checker_create();
    analyzer->has_errors = false;
    analyzer->error_count = 0;
    analyzer->warning_count = 0;
    analyzer->in_loop = false;
    analyzer->in_function = false;
    analyzer->has_return = false;
    analyzer->enable_constant_folding = true;
    
    return analyzer;
}

void semantic_analyzer_destroy(SemanticAnalyzer* analyzer) {
    if (!analyzer) return;
    
    symbol_table_destroy(analyzer->symbol_table);
    type_checker_destroy(analyzer->type_checker);
    free(analyzer);
}

// ========== 错误和警告报告 ==========

void semantic_error(SemanticAnalyzer* analyzer, int line, const char* format, ...) {
    fprintf(stderr, "\033[31m语义错误 (行 %d): ", line);
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\033[0m\n");
    
    analyzer->has_errors = true;
    analyzer->error_count++;
}

void semantic_warning(SemanticAnalyzer* analyzer, int line, const char* format, ...) {
    fprintf(stderr, "\033[33m语义警告 (行 %d): ", line);
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\033[0m\n");
    
    analyzer->warning_count++;
}

// ========== 工具函数 ==========

bool is_constant_expr(ASTNode* node) {
    if (!node) return false;
    
    if (node->node_type == AST_LITERAL) {
        return true;
    }
    
    if (node->node_type == AST_BINARY_OP) {
        return is_constant_expr(node->data.binary_op.left) &&
               is_constant_expr(node->data.binary_op.right);
    }
    
    if (node->node_type == AST_UNARY_OP) {
        return is_constant_expr(node->data.unary_op.operand);
    }
    
    return false;
}

int evaluate_constant_expr(ASTNode* node) {
    if (!node) return 0;
    
    if (node->node_type == AST_LITERAL) {
        return node->data.literal.value.int_value;
    }
    
    if (node->node_type == AST_BINARY_OP) {
        int left = evaluate_constant_expr(node->data.binary_op.left);
        int right = evaluate_constant_expr(node->data.binary_op.right);
        
        switch (node->data.binary_op.op) {
            case OP_ADD: return left + right;
            case OP_SUB: return left - right;
            case OP_MUL: return left * right;
            case OP_DIV: 
                if (right == 0) return 0; // 除零错误已在类型检查中处理
                return left / right;
            case OP_MOD:
                if (right == 0) return 0;
                return left % right;
            case OP_LT: return left < right;
            case OP_LE: return left <= right;
            case OP_GT: return left > right;
            case OP_GE: return left >= right;
            case OP_EQ: return left == right;
            case OP_NE: return left != right;
            case OP_AND: return left && right;
            case OP_OR: return left || right;
            default: return 0;
        }
    }
    
    if (node->node_type == AST_UNARY_OP) {
        int operand = evaluate_constant_expr(node->data.unary_op.operand);
        
        switch (node->data.unary_op.op) {
            case OP_NEG: return -operand;
            case OP_NOT: return !operand;
            default: return 0;
        }
    }
    
    return 0;
}

// ========== 常量折叠 ==========

ASTNode* fold_binary_op(ASTNode* node) {
    if (!node || node->node_type != AST_BINARY_OP) return node;
    
    // 递归折叠子表达式
    node->data.binary_op.left = constant_fold(NULL, node->data.binary_op.left);
    node->data.binary_op.right = constant_fold(NULL, node->data.binary_op.right);
    
    // 如果两个操作数都是常量,计算结果
    if (is_constant_expr(node->data.binary_op.left) && 
        is_constant_expr(node->data.binary_op.right)) {
        
        // 检查除零
        if ((node->data.binary_op.op == OP_DIV || node->data.binary_op.op == OP_MOD) &&
            evaluate_constant_expr(node->data.binary_op.right) == 0) {
            return node; // 保留原节点,让类型检查器报错
        }
        
        int result = evaluate_constant_expr(node);
        
        // 创建新的字面量节点
        ASTNode* literal = ast_create_int_literal(result, node->line);
        
        // 释放原节点
        ast_free(node);
        
        return literal;
    }
    
    return node;
}

ASTNode* fold_unary_op(ASTNode* node) {
    if (!node || node->node_type != AST_UNARY_OP) return node;
    
    // 递归折叠子表达式
    node->data.unary_op.operand = constant_fold(NULL, node->data.unary_op.operand);
    
    // 如果操作数是常量,计算结果
    if (is_constant_expr(node->data.unary_op.operand)) {
        int result = evaluate_constant_expr(node);
        
        // 创建新的字面量节点
        ASTNode* literal = ast_create_int_literal(result, node->line);
        
        // 释放原节点
        ast_free(node);
        
        return literal;
    }
    
    return node;
}

ASTNode* constant_fold(SemanticAnalyzer* analyzer, ASTNode* node) {
    (void)analyzer;
    if (!node) return NULL;
    
    switch (node->node_type) {
        case AST_BINARY_OP:
            return fold_binary_op(node);
        case AST_UNARY_OP:
            return fold_unary_op(node);
        default:
            return node;
    }
}

// ========== 死代码检测 ==========

bool is_unreachable_after(ASTNode* node) {
    if (!node) return false;
    
    // return语句后的代码不可达
    if (node->node_type == AST_RETURN_STMT) {
        return true;
    }
    
    // if语句:如果两个分支都不可达,则之后的代码不可达
    if (node->node_type == AST_IF_STMT) {
        if (node->data.if_stmt.else_branch) {
            return is_unreachable_after(node->data.if_stmt.then_branch) &&
                   is_unreachable_after(node->data.if_stmt.else_branch);
        }
    }
    
    // 复合语句:检查最后一条语句
    if (node->node_type == AST_COMPOUND_STMT) {
        if (node->data.compound_stmt.stmt_count > 0) {
            return is_unreachable_after(
                node->data.compound_stmt.statements[node->data.compound_stmt.stmt_count - 1]
            );
        }
    }
    
    return false;
}

void detect_dead_code(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (!node || node->node_type != AST_COMPOUND_STMT) return;
    
    for (int i = 0; i < node->data.compound_stmt.stmt_count - 1; i++) {
        ASTNode* stmt = node->data.compound_stmt.statements[i];
        
        if (is_unreachable_after(stmt)) {
            semantic_warning(analyzer, 
                           node->data.compound_stmt.statements[i + 1]->line,
                           "检测到死代码:此语句之后的代码不可达");
            break;
        }
    }
}

// ========== 控制流分析 ==========

bool check_all_paths_return(ASTNode* node) {
    if (!node) return false;
    
    if (node->node_type == AST_RETURN_STMT) {
        return true;
    }
    
    if (node->node_type == AST_IF_STMT) {
        // if-else语句:两个分支都必须返回
        if (node->data.if_stmt.else_branch) {
            return check_all_paths_return(node->data.if_stmt.then_branch) &&
                   check_all_paths_return(node->data.if_stmt.else_branch);
        }
        return false;
    }
    
    if (node->node_type == AST_COMPOUND_STMT) {
        // 复合语句:检查是否有任何语句返回
        for (int i = 0; i < node->data.compound_stmt.stmt_count; i++) {
            if (check_all_paths_return(node->data.compound_stmt.statements[i])) {
                return true;
            }
        }
        return false;
    }
    
    return false;
}

void semantic_check_control_flow(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (!node) return;
    
    // 检查函数是否所有路径都有返回值
    if (node->node_type == AST_FUNC_DECL && node->data.func_decl.body) {
        // 如果返回类型不是void,检查是否所有路径都返回
        if (node->data.func_decl.return_type->base_type != TYPE_VOID) {
            if (!check_all_paths_return(node->data.func_decl.body)) {
                semantic_warning(analyzer, node->line,
                               "函数 '%s' 并非所有控制路径都有返回值",
                               node->data.func_decl.func_name);
            }
        }
    }
}

// ========== 具体的语义检查 ==========

void semantic_check_var_decl(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (!node || node->node_type != AST_VAR_DECL) return;
    
    // 检查数组大小
    if (node->data.var_decl.var_type->base_type == TYPE_ARRAY) {
        int size = node->data.var_decl.var_type->array_info.size;
        if (size <= 0) {
            semantic_error(analyzer, node->line, 
                         "数组 '%s' 的大小必须是正整数",
                         node->data.var_decl.var_name);
        }
    }
    
    // 检查void类型变量
    if (node->data.var_decl.var_type->base_type == TYPE_VOID) {
        semantic_error(analyzer, node->line,
                     "变量 '%s' 不能声明为void类型",
                     node->data.var_decl.var_name);
    }
}

void semantic_check_func_decl(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (!node || node->node_type != AST_FUNC_DECL) return;
    
    // 检查main函数签名
    if (strcmp(node->data.func_decl.func_name, "main") == 0) {
        if (node->data.func_decl.return_type->base_type != TYPE_INT) {
            semantic_warning(analyzer, node->line,
                           "main函数应该返回int类型");
        }
    }
    
    // 检查参数名称重复
    for (int i = 0; i < node->data.func_decl.param_count; i++) {
        for (int j = i + 1; j < node->data.func_decl.param_count; j++) {
            ASTNode* param1 = node->data.func_decl.params[i];
            ASTNode* param2 = node->data.func_decl.params[j];
            
            if (strcmp(param1->data.var_decl.var_name, 
                      param2->data.var_decl.var_name) == 0) {
                semantic_error(analyzer, node->line,
                             "函数 '%s' 有重复的参数名 '%s'",
                             node->data.func_decl.func_name,
                             param1->data.var_decl.var_name);
            }
        }
    }
    
    // 检查控制流
    semantic_check_control_flow(analyzer, node);
}

void semantic_check_assign_stmt(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (!node || node->node_type != AST_ASSIGN_STMT) return;
    
    // 检查是否给常量赋值
    if (node->data.assign_stmt.lvalue->node_type == AST_IDENTIFIER) {
        Symbol* symbol = symbol_table_lookup(analyzer->symbol_table,
                                            node->data.assign_stmt.lvalue->data.identifier.name);
        if (symbol && symbol->kind == SYMBOL_VAR && symbol->var_info.is_const) {
            semantic_error(analyzer, node->line,
                         "不能给常量 '%s' 赋值",
                         node->data.assign_stmt.lvalue->data.identifier.name);
        }
    }
}

void semantic_check_return_stmt(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (!node || node->node_type != AST_RETURN_STMT) return;
    
    if (!analyzer->in_function) {
        semantic_error(analyzer, node->line,
                     "return语句只能在函数内使用");
    }
    
    analyzer->has_return = true;
}

void semantic_check_binary_op(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (!node || node->node_type != AST_BINARY_OP) return;
    
    // 检查除零
    if (node->data.binary_op.op == OP_DIV || node->data.binary_op.op == OP_MOD) {
        if (is_constant_expr(node->data.binary_op.right)) {
            int divisor = evaluate_constant_expr(node->data.binary_op.right);
            if (divisor == 0) {
                semantic_error(analyzer, node->line, "除数不能为0");
            }
        }
    }
}

void semantic_check_unary_op(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (!node || node->node_type != AST_UNARY_OP) return;
    
    // 检查自增自减是否作用于左值
    if (node->data.unary_op.op == OP_INC || node->data.unary_op.op == OP_DEC) {
        if (!is_lvalue(node->data.unary_op.operand)) {
            semantic_error(analyzer, node->line,
                         "自增/自减运算符要求左值");
        }
    }
}

void semantic_check_func_call(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (!node || node->node_type != AST_FUNC_CALL) return;
    
    // 检查函数是否已声明
    Symbol* func_symbol = symbol_table_lookup(analyzer->symbol_table,
                                             node->data.func_call.func_name);
    
    if (!func_symbol) {
        semantic_error(analyzer, node->line,
                     "未声明的函数 '%s'",
                     node->data.func_call.func_name);
        return;
    }
    
    if (func_symbol->kind != SYMBOL_FUNC) {
        semantic_error(analyzer, node->line,
                     "'%s' 不是函数",
                     node->data.func_call.func_name);
        return;
    }
}

// ========== 未使用变量检测 ==========

void check_unused_variables(SemanticAnalyzer* analyzer) {
    (void)analyzer;
}

// ========== 主语义分析函数 ==========

void semantic_analyze_node(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (!node) return;
    
    switch (node->node_type) {
        case AST_VAR_DECL:
            semantic_check_var_decl(analyzer, node);
            if (node->data.var_decl.init_value) {
                semantic_analyze_node(analyzer, node->data.var_decl.init_value);
            }
            break;
            
        case AST_FUNC_DECL:
            semantic_check_func_decl(analyzer, node);
            if (node->data.func_decl.body) {
                bool saved_in_function = analyzer->in_function;
                bool saved_has_return = analyzer->has_return;
                
                analyzer->in_function = true;
                analyzer->has_return = false;
                
                semantic_analyze_node(analyzer, node->data.func_decl.body);
                
                analyzer->in_function = saved_in_function;
                analyzer->has_return = saved_has_return;
            }
            break;
            
        case AST_ASSIGN_STMT:
            semantic_check_assign_stmt(analyzer, node);
            semantic_analyze_node(analyzer, node->data.assign_stmt.lvalue);
            semantic_analyze_node(analyzer, node->data.assign_stmt.rvalue);
            break;
            
        case AST_RETURN_STMT:
            semantic_check_return_stmt(analyzer, node);
            if (node->data.return_stmt.return_value) {
                semantic_analyze_node(analyzer, node->data.return_stmt.return_value);
            }
            break;
            
        case AST_BINARY_OP:
            semantic_check_binary_op(analyzer, node);
            semantic_analyze_node(analyzer, node->data.binary_op.left);
            semantic_analyze_node(analyzer, node->data.binary_op.right);
            
            // 常量折叠
            if (analyzer->enable_constant_folding) {
                node = constant_fold(analyzer, node);
            }
            break;
            
        case AST_UNARY_OP:
            semantic_check_unary_op(analyzer, node);
            semantic_analyze_node(analyzer, node->data.unary_op.operand);
            
            // 常量折叠
            if (analyzer->enable_constant_folding) {
                node = constant_fold(analyzer, node);
            }
            break;
            
        case AST_FUNC_CALL:
            semantic_check_func_call(analyzer, node);
            for (int i = 0; i < node->data.func_call.arg_count; i++) {
                semantic_analyze_node(analyzer, node->data.func_call.args[i]);
            }
            break;
            
        case AST_IF_STMT:
            semantic_analyze_node(analyzer, node->data.if_stmt.condition);
            semantic_analyze_node(analyzer, node->data.if_stmt.then_branch);
            if (node->data.if_stmt.else_branch) {
                semantic_analyze_node(analyzer, node->data.if_stmt.else_branch);
            }
            break;
            
        case AST_WHILE_STMT: {
            bool saved_in_loop = analyzer->in_loop;
            analyzer->in_loop = true;
            
            semantic_analyze_node(analyzer, node->data.while_stmt.condition);
            semantic_analyze_node(analyzer, node->data.while_stmt.body);
            
            analyzer->in_loop = saved_in_loop;
            break;
        }
            
        case AST_COMPOUND_STMT:
            for (int i = 0; i < node->data.compound_stmt.stmt_count; i++) {
                semantic_analyze_node(analyzer, node->data.compound_stmt.statements[i]);
            }
            // 检测死代码
            detect_dead_code(analyzer, node);
            break;
            
        case AST_EXPR_STMT:
            if (node->data.expr_stmt.expr) {
                semantic_analyze_node(analyzer, node->data.expr_stmt.expr);
            }
            break;
            
        case AST_ARRAY_ACCESS:
            semantic_analyze_node(analyzer, node->data.array_access.array);
            semantic_analyze_node(analyzer, node->data.array_access.index);
            break;
            
        default:
            break;
    }
}

bool semantic_analyze_program(SemanticAnalyzer* analyzer, ASTNode* program) {
    if (!analyzer || !program || program->node_type != AST_PROGRAM) {
        return false;
    }
    
    printf("\n========== 开始语义分析 ==========\n");
    
    // 首先进行类型检查
    bool type_check_passed = type_check_program(analyzer->type_checker, program);
    
    if (!type_check_passed) {
        analyzer->has_errors = true;
        analyzer->error_count += analyzer->type_checker->error_count;
    }
    
    // 然后进行其他语义检查
    for (int i = 0; i < program->data.program.decl_count; i++) {
        semantic_analyze_node(analyzer, program->data.program.declarations[i]);
    }
    
    // 检查未使用的变量
    check_unused_variables(analyzer);
    
    printf("========== 语义分析完成 ==========\n");
    
    if (analyzer->has_errors) {
        printf("\033[31m发现 %d 个语义错误\033[0m\n", analyzer->error_count);
        return false;
    } else {
        printf("\033[32m语义分析通过");
        if (analyzer->warning_count > 0) {
            printf(" (有 %d 个警告)", analyzer->warning_count);
        }
        printf("\033[0m\n");
        return true;
    }
}
