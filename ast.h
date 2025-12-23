#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stdlib.h>

// 前向声明
typedef struct ASTNode ASTNode;
typedef struct Type Type;

// ========== 类型系统 ==========

// 基础类型枚举
typedef enum {
    TYPE_VOID,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_BOOL,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_POINTER,
    TYPE_ERROR  // 类型错误标记
} BaseType;

// 类型结构体(支持复合类型)
struct Type {
    BaseType base_type;
    
    // 数组类型信息
    struct {
        Type* element_type;  // 数组元素类型
        int size;            // 数组大小(-1表示未指定)
    } array_info;
    
    // 函数类型信息
    struct {
        Type* return_type;   // 返回类型
        Type** param_types;  // 参数类型列表
        int param_count;     // 参数个数
    } func_info;
    
    // 指针类型信息
    struct {
        Type* pointed_type;  // 指向的类型
    } pointer_info;
};

// 类型操作函数
Type* type_create_basic(BaseType base);
Type* type_create_array(Type* element_type, int size);
Type* type_create_function(Type* return_type, Type** param_types, int param_count);
Type* type_create_pointer(Type* pointed_type);
Type* type_copy(Type* type);
void type_free(Type* type);
bool type_equals(Type* t1, Type* t2);
bool type_is_compatible(Type* t1, Type* t2);
const char* type_to_string(Type* type);

// ========== AST节点类型 ==========

typedef enum {
    // 表达式节点
    AST_BINARY_OP,      // 二元运算: +, -, *, /, <, >, ==, !=, etc.
    AST_UNARY_OP,       // 一元运算: -, !, ++, --, &, *
    AST_LITERAL,        // 字面量: 整数、浮点数、字符、字符串
    AST_IDENTIFIER,     // 标识符
    AST_ARRAY_ACCESS,   // 数组访问: arr[index]
    AST_FUNC_CALL,      // 函数调用: func(args)
    AST_CAST,           // 类型转换: (type)expr
    
    // 语句节点
    AST_COMPOUND_STMT,  // 复合语句: { stmts }
    AST_IF_STMT,        // if语句
    AST_WHILE_STMT,     // while循环
    AST_FOR_STMT,       // for循环
    AST_RETURN_STMT,    // return语句
    AST_EXPR_STMT,      // 表达式语句
    AST_DECL_STMT,      // 声明语句
    AST_ASSIGN_STMT,    // 赋值语句
    
    // 声明节点
    AST_VAR_DECL,       // 变量声明
    AST_FUNC_DECL,      // 函数声明
    AST_PARAM_DECL,     // 参数声明
    
    // 程序节点
    AST_PROGRAM         // 程序根节点
} ASTNodeType;

// 二元运算符类型
typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_LT, OP_LE, OP_GT, OP_GE, OP_EQ, OP_NE,
    OP_AND, OP_OR,
    OP_ASSIGN, OP_ADD_ASSIGN, OP_SUB_ASSIGN
} BinaryOp;

// 一元运算符类型
typedef enum {
    OP_NEG,      // 负号 -
    OP_NOT,      // 逻辑非 !
    OP_INC,      // 自增 ++
    OP_DEC,      // 自减 --
    OP_ADDR,     // 取地址 &
    OP_DEREF     // 解引用 *
} UnaryOp;

// ========== AST节点结构 ==========

struct ASTNode {
    ASTNodeType node_type;
    Type* type;  // 节点的类型(类型检查后填充)
    int line;    // 源代码行号
    
    union {
        // 二元运算节点
        struct {
            BinaryOp op;
            ASTNode* left;
            ASTNode* right;
        } binary_op;
        
        // 一元运算节点
        struct {
            UnaryOp op;
            ASTNode* operand;
        } unary_op;
        
        // 字面量节点
        struct {
            union {
                int int_value;
                float float_value;
                char char_value;
                char* string_value;
            } value;
        } literal;
        
        // 标识符节点
        struct {
            char* name;
        } identifier;
        
        // 数组访问节点
        struct {
            ASTNode* array;
            ASTNode* index;
        } array_access;
        
        // 函数调用节点
        struct {
            char* func_name;
            ASTNode** args;
            int arg_count;
        } func_call;
        
        // 类型转换节点
        struct {
            Type* target_type;
            ASTNode* expr;
        } cast;
        
        // 复合语句节点
        struct {
            ASTNode** statements;
            int stmt_count;
        } compound_stmt;
        
        // if语句节点
        struct {
            ASTNode* condition;
            ASTNode* then_branch;
            ASTNode* else_branch;  // 可为NULL
        } if_stmt;
        
        // while语句节点
        struct {
            ASTNode* condition;
            ASTNode* body;
        } while_stmt;
        
        // for语句节点
        struct {
            ASTNode* init;       // 可为NULL
            ASTNode* condition;  // 可为NULL
            ASTNode* increment;  // 可为NULL
            ASTNode* body;
        } for_stmt;
        
        // return语句节点
        struct {
            ASTNode* return_value;  // 可为NULL
        } return_stmt;
        
        // 表达式语句节点
        struct {
            ASTNode* expr;
        } expr_stmt;
        
        // 变量声明节点
        struct {
            char* var_name;
            Type* var_type;
            ASTNode* init_value;  // 初始值,可为NULL
        } var_decl;
        
        // 函数声明节点
        struct {
            char* func_name;
            Type* return_type;
            ASTNode** params;     // 参数列表
            int param_count;
            ASTNode* body;        // 函数体,可为NULL(函数声明)
        } func_decl;
        
        // 参数声明节点
        struct {
            char* param_name;
            Type* param_type;
        } param_decl;
        
        // 赋值语句节点
        struct {
            ASTNode* lvalue;
            ASTNode* rvalue;
        } assign_stmt;
        
        // 程序节点
        struct {
            ASTNode** declarations;  // 全局声明列表
            int decl_count;
        } program;
    } data;
};

// ========== AST节点创建函数 ==========

ASTNode* ast_create_binary_op(BinaryOp op, ASTNode* left, ASTNode* right, int line);
ASTNode* ast_create_unary_op(UnaryOp op, ASTNode* operand, int line);
ASTNode* ast_create_int_literal(int value, int line);
ASTNode* ast_create_identifier(const char* name, int line);
ASTNode* ast_create_array_access(ASTNode* array, ASTNode* index, int line);
ASTNode* ast_create_func_call(const char* func_name, ASTNode** args, int arg_count, int line);
ASTNode* ast_create_if_stmt(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch, int line);
ASTNode* ast_create_while_stmt(ASTNode* condition, ASTNode* body, int line);
ASTNode* ast_create_return_stmt(ASTNode* return_value, int line);
ASTNode* ast_create_var_decl(const char* var_name, Type* var_type, ASTNode* init_value, int line);
ASTNode* ast_create_func_decl(const char* func_name, Type* return_type, ASTNode** params, int param_count, ASTNode* body, int line);
ASTNode* ast_create_assign_stmt(ASTNode* lvalue, ASTNode* rvalue, int line);
ASTNode* ast_create_compound_stmt(ASTNode** statements, int stmt_count, int line);
ASTNode* ast_create_expr_stmt(ASTNode* expr, int line);
ASTNode* ast_create_program(ASTNode** declarations, int decl_count);

// ========== AST操作函数 ==========

void ast_free(ASTNode* node);
void ast_print(ASTNode* node, int indent);
const char* ast_node_type_str(ASTNodeType type);
const char* binary_op_str(BinaryOp op);
const char* unary_op_str(UnaryOp op);

#endif // AST_H
