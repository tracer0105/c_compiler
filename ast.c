#include "ast.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* strdup(const char* s);

// ========== 类型系统实现 ==========

Type* type_create_basic(BaseType base) {
    Type* type = (Type*)malloc(sizeof(Type));
    if (!type) {
        perror("malloc failed for Type");
        exit(1);
    }
    type->base_type = base;
    return type;
}

Type* type_create_array(Type* element_type, int size) {
    Type* type = (Type*)malloc(sizeof(Type));
    if (!type) {
        perror("malloc failed for Type");
        exit(1);
    }
    type->base_type = TYPE_ARRAY;
    type->array_info.element_type = element_type;
    type->array_info.size = size;
    return type;
}

Type* type_create_function(Type* return_type, Type** param_types, int param_count) {
    Type* type = (Type*)malloc(sizeof(Type));
    if (!type) {
        perror("malloc failed for Type");
        exit(1);
    }
    type->base_type = TYPE_FUNCTION;
    type->func_info.return_type = return_type;
    type->func_info.param_types = param_types;
    type->func_info.param_count = param_count;
    return type;
}

Type* type_create_pointer(Type* pointed_type) {
    Type* type = (Type*)malloc(sizeof(Type));
    if (!type) {
        perror("malloc failed for Type");
        exit(1);
    }
    type->base_type = TYPE_POINTER;
    type->pointer_info.pointed_type = pointed_type;
    return type;
}

Type* type_copy(Type* type) {
    if (!type) return NULL;
    
    Type* copy = (Type*)malloc(sizeof(Type));
    if (!copy) {
        perror("malloc failed for Type");
        exit(1);
    }
    
    copy->base_type = type->base_type;
    
    switch (type->base_type) {
        case TYPE_ARRAY:
            copy->array_info.element_type = type_copy(type->array_info.element_type);
            copy->array_info.size = type->array_info.size;
            break;
        case TYPE_FUNCTION:
            copy->func_info.return_type = type_copy(type->func_info.return_type);
            copy->func_info.param_count = type->func_info.param_count;
            copy->func_info.param_types = (Type**)malloc(sizeof(Type*) * type->func_info.param_count);
            for (int i = 0; i < type->func_info.param_count; i++) {
                copy->func_info.param_types[i] = type_copy(type->func_info.param_types[i]);
            }
            break;
        case TYPE_POINTER:
            copy->pointer_info.pointed_type = type_copy(type->pointer_info.pointed_type);
            break;
        default:
            break;
    }
    
    return copy;
}

void type_free(Type* type) {
    if (!type) return;
    
    switch (type->base_type) {
        case TYPE_ARRAY:
            type_free(type->array_info.element_type);
            break;
        case TYPE_FUNCTION:
            type_free(type->func_info.return_type);
            for (int i = 0; i < type->func_info.param_count; i++) {
                type_free(type->func_info.param_types[i]);
            }
            free(type->func_info.param_types);
            break;
        case TYPE_POINTER:
            type_free(type->pointer_info.pointed_type);
            break;
        default:
            break;
    }
    
    free(type);
}

bool type_equals(Type* t1, Type* t2) {
    if (!t1 || !t2) return false;
    if (t1->base_type != t2->base_type) return false;
    
    switch (t1->base_type) {
        case TYPE_ARRAY:
            return type_equals(t1->array_info.element_type, t2->array_info.element_type) &&
                   t1->array_info.size == t2->array_info.size;
        case TYPE_FUNCTION:
            if (!type_equals(t1->func_info.return_type, t2->func_info.return_type))
                return false;
            if (t1->func_info.param_count != t2->func_info.param_count)
                return false;
            for (int i = 0; i < t1->func_info.param_count; i++) {
                if (!type_equals(t1->func_info.param_types[i], t2->func_info.param_types[i]))
                    return false;
            }
            return true;
        case TYPE_POINTER:
            return type_equals(t1->pointer_info.pointed_type, t2->pointer_info.pointed_type);
        default:
            return true;
    }
}

bool type_is_compatible(Type* t1, Type* t2) {
    if (!t1 || !t2) return false;
    
    // 相同类型兼容
    if (type_equals(t1, t2)) return true;
    
    // int和float可以隐式转换
    if ((t1->base_type == TYPE_INT && t2->base_type == TYPE_FLOAT) ||
        (t1->base_type == TYPE_FLOAT && t2->base_type == TYPE_INT)) {
        return true;
    }
    
    // char可以转换为int
    if ((t1->base_type == TYPE_CHAR && t2->base_type == TYPE_INT) ||
        (t1->base_type == TYPE_INT && t2->base_type == TYPE_CHAR)) {
        return true;
    }
    
    return false;
}

const char* type_to_string(Type* type) {
    if (!type) return "NULL";
    
    static char buffer[256];
    
    switch (type->base_type) {
        case TYPE_VOID: return "void";
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_CHAR: return "char";
        case TYPE_BOOL: return "bool";
        case TYPE_ERROR: return "ERROR";
        case TYPE_ARRAY:
            snprintf(buffer, sizeof(buffer), "%s[%d]", 
                    type_to_string(type->array_info.element_type),
                    type->array_info.size);
            return buffer;
        case TYPE_POINTER:
            snprintf(buffer, sizeof(buffer), "%s*", 
                    type_to_string(type->pointer_info.pointed_type));
            return buffer;
        case TYPE_FUNCTION:
            snprintf(buffer, sizeof(buffer), "%s(...)", 
                    type_to_string(type->func_info.return_type));
            return buffer;
        default:
            return "UNKNOWN";
    }
}

// ========== AST节点创建函数 ==========

ASTNode* ast_create_binary_op(BinaryOp op, ASTNode* left, ASTNode* right, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_BINARY_OP;
    node->type = NULL;
    node->line = line;
    node->data.binary_op.op = op;
    node->data.binary_op.left = left;
    node->data.binary_op.right = right;
    return node;
}

ASTNode* ast_create_unary_op(UnaryOp op, ASTNode* operand, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_UNARY_OP;
    node->type = NULL;
    node->line = line;
    node->data.unary_op.op = op;
    node->data.unary_op.operand = operand;
    return node;
}

ASTNode* ast_create_int_literal(int value, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_LITERAL;
    node->type = type_create_basic(TYPE_INT);
    node->line = line;
    node->data.literal.value.int_value = value;
    return node;
}

ASTNode* ast_create_identifier(const char* name, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_IDENTIFIER;
    node->type = NULL;
    node->line = line;
    node->data.identifier.name = strdup(name);
    return node;
}

ASTNode* ast_create_array_access(ASTNode* array, ASTNode* index, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_ARRAY_ACCESS;
    node->type = NULL;
    node->line = line;
    node->data.array_access.array = array;
    node->data.array_access.index = index;
    return node;
}

ASTNode* ast_create_func_call(const char* func_name, ASTNode** args, int arg_count, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_FUNC_CALL;
    node->type = NULL;
    node->line = line;
    node->data.func_call.func_name = strdup(func_name);
    node->data.func_call.args = args;
    node->data.func_call.arg_count = arg_count;
    return node;
}

ASTNode* ast_create_if_stmt(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_IF_STMT;
    node->type = NULL;
    node->line = line;
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_branch = then_branch;
    node->data.if_stmt.else_branch = else_branch;
    return node;
}

ASTNode* ast_create_while_stmt(ASTNode* condition, ASTNode* body, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_WHILE_STMT;
    node->type = NULL;
    node->line = line;
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;
    return node;
}

ASTNode* ast_create_return_stmt(ASTNode* return_value, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_RETURN_STMT;
    node->type = NULL;
    node->line = line;
    node->data.return_stmt.return_value = return_value;
    return node;
}

ASTNode* ast_create_var_decl(const char* var_name, Type* var_type, ASTNode* init_value, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_VAR_DECL;
    node->type = var_type;
    node->line = line;
    node->data.var_decl.var_name = strdup(var_name);
    node->data.var_decl.var_type = var_type;
    node->data.var_decl.init_value = init_value;
    return node;
}

ASTNode* ast_create_func_decl(const char* func_name, Type* return_type, ASTNode** params, int param_count, ASTNode* body, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_FUNC_DECL;
    node->type = return_type;
    node->line = line;
    node->data.func_decl.func_name = strdup(func_name);
    node->data.func_decl.return_type = return_type;
    node->data.func_decl.params = params;
    node->data.func_decl.param_count = param_count;
    node->data.func_decl.body = body;
    return node;
}

ASTNode* ast_create_assign_stmt(ASTNode* lvalue, ASTNode* rvalue, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_ASSIGN_STMT;
    node->type = NULL;
    node->line = line;
    node->data.assign_stmt.lvalue = lvalue;
    node->data.assign_stmt.rvalue = rvalue;
    return node;
}

ASTNode* ast_create_compound_stmt(ASTNode** statements, int stmt_count, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_COMPOUND_STMT;
    node->type = NULL;
    node->line = line;
    node->data.compound_stmt.statements = statements;
    node->data.compound_stmt.stmt_count = stmt_count;
    return node;
}

ASTNode* ast_create_expr_stmt(ASTNode* expr, int line) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_EXPR_STMT;
    node->type = NULL;
    node->line = line;
    node->data.expr_stmt.expr = expr;
    return node;
}

ASTNode* ast_create_program(ASTNode** declarations, int decl_count) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        perror("malloc failed for ASTNode");
        exit(1);
    }
    node->node_type = AST_PROGRAM;
    node->type = NULL;
    node->line = 0;
    node->data.program.declarations = declarations;
    node->data.program.decl_count = decl_count;
    return node;
}

// ========== AST操作函数 ==========

void ast_free(ASTNode* node) {
    if (!node) return;
    
    switch (node->node_type) {
        case AST_BINARY_OP:
            ast_free(node->data.binary_op.left);
            ast_free(node->data.binary_op.right);
            break;
        case AST_UNARY_OP:
            ast_free(node->data.unary_op.operand);
            break;
        case AST_IDENTIFIER:
            free(node->data.identifier.name);
            break;
        case AST_ARRAY_ACCESS:
            ast_free(node->data.array_access.array);
            ast_free(node->data.array_access.index);
            break;
        case AST_FUNC_CALL:
            free(node->data.func_call.func_name);
            for (int i = 0; i < node->data.func_call.arg_count; i++) {
                ast_free(node->data.func_call.args[i]);
            }
            free(node->data.func_call.args);
            break;
        case AST_IF_STMT:
            ast_free(node->data.if_stmt.condition);
            ast_free(node->data.if_stmt.then_branch);
            ast_free(node->data.if_stmt.else_branch);
            break;
        case AST_WHILE_STMT:
            ast_free(node->data.while_stmt.condition);
            ast_free(node->data.while_stmt.body);
            break;
        case AST_RETURN_STMT:
            ast_free(node->data.return_stmt.return_value);
            break;
        case AST_VAR_DECL:
            free(node->data.var_decl.var_name);
            type_free(node->data.var_decl.var_type);
            ast_free(node->data.var_decl.init_value);
            break;
        case AST_FUNC_DECL:
            free(node->data.func_decl.func_name);
            type_free(node->data.func_decl.return_type);
            for (int i = 0; i < node->data.func_decl.param_count; i++) {
                ast_free(node->data.func_decl.params[i]);
            }
            free(node->data.func_decl.params);
            ast_free(node->data.func_decl.body);
            break;
        case AST_ASSIGN_STMT:
            ast_free(node->data.assign_stmt.lvalue);
            ast_free(node->data.assign_stmt.rvalue);
            break;
        case AST_COMPOUND_STMT:
            for (int i = 0; i < node->data.compound_stmt.stmt_count; i++) {
                ast_free(node->data.compound_stmt.statements[i]);
            }
            free(node->data.compound_stmt.statements);
            break;
        case AST_EXPR_STMT:
            ast_free(node->data.expr_stmt.expr);
            break;
        case AST_PROGRAM:
            for (int i = 0; i < node->data.program.decl_count; i++) {
                ast_free(node->data.program.declarations[i]);
            }
            free(node->data.program.declarations);
            break;
        default:
            break;
    }
    
    if (node->type) type_free(node->type);
    free(node);
}

void ast_print(ASTNode* node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    
    printf("%s", ast_node_type_str(node->node_type));
    if (node->type) {
        printf(" [type: %s]", type_to_string(node->type));
    }
    printf("\n");
    
    switch (node->node_type) {
        case AST_BINARY_OP:
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("op: %s\n", binary_op_str(node->data.binary_op.op));
            ast_print(node->data.binary_op.left, indent + 1);
            ast_print(node->data.binary_op.right, indent + 1);
            break;
        case AST_UNARY_OP:
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("op: %s\n", unary_op_str(node->data.unary_op.op));
            ast_print(node->data.unary_op.operand, indent + 1);
            break;
        case AST_LITERAL:
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("value: %d\n", node->data.literal.value.int_value);
            break;
        case AST_IDENTIFIER:
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("name: %s\n", node->data.identifier.name);
            break;
        case AST_VAR_DECL:
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("name: %s, type: %s\n", node->data.var_decl.var_name, 
                   type_to_string(node->data.var_decl.var_type));
            if (node->data.var_decl.init_value) {
                ast_print(node->data.var_decl.init_value, indent + 1);
            }
            break;
        case AST_ASSIGN_STMT:
            ast_print(node->data.assign_stmt.lvalue, indent + 1);
            ast_print(node->data.assign_stmt.rvalue, indent + 1);
            break;
        default:
            break;
    }
}

const char* ast_node_type_str(ASTNodeType type) {
    switch (type) {
        case AST_BINARY_OP: return "BinaryOp";
        case AST_UNARY_OP: return "UnaryOp";
        case AST_LITERAL: return "Literal";
        case AST_IDENTIFIER: return "Identifier";
        case AST_ARRAY_ACCESS: return "ArrayAccess";
        case AST_FUNC_CALL: return "FuncCall";
        case AST_IF_STMT: return "IfStmt";
        case AST_WHILE_STMT: return "WhileStmt";
        case AST_RETURN_STMT: return "ReturnStmt";
        case AST_VAR_DECL: return "VarDecl";
        case AST_FUNC_DECL: return "FuncDecl";
        case AST_ASSIGN_STMT: return "AssignStmt";
        case AST_COMPOUND_STMT: return "CompoundStmt";
        case AST_EXPR_STMT: return "ExprStmt";
        case AST_PROGRAM: return "Program";
        default: return "Unknown";
    }
}

const char* binary_op_str(BinaryOp op) {
    switch (op) {
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_MOD: return "%";
        case OP_LT: return "<";
        case OP_LE: return "<=";
        case OP_GT: return ">";
        case OP_GE: return ">=";
        case OP_EQ: return "==";
        case OP_NE: return "!=";
        case OP_AND: return "&&";
        case OP_OR: return "||";
        case OP_ASSIGN: return "=";
        case OP_ADD_ASSIGN: return "+=";
        case OP_SUB_ASSIGN: return "-=";
        default: return "?";
    }
}

const char* unary_op_str(UnaryOp op) {
    switch (op) {
        case OP_NEG: return "-";
        case OP_NOT: return "!";
        case OP_INC: return "++";
        case OP_DEC: return "--";
        case OP_ADDR: return "&";
        case OP_DEREF: return "*";
        default: return "?";
    }
}
