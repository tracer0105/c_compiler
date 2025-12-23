# C编译器 - 符号表与语义分析增强版

## 项目概述

本项目是一个C语言编译器的前端实现,包含词法分析、语法分析、符号表管理、类型检查和语义分析等核心功能。

## 新增功能

### 1. 完善的符号表系统 (`symbol_table.h/c`)

#### 核心特性
- **多层作用域支持**: 支持全局作用域和嵌套的局部作用域
- **哈希表实现**: 使用哈希表提高符号查找效率
- **符号类型**: 支持变量(SYMBOL_VAR)、函数(SYMBOL_FUNC)、参数(SYMBOL_PARAM)、类型定义(SYMBOL_TYPE)
- **作用域管理**: 
  - `symbol_table_enter_scope()`: 进入新作用域
  - `symbol_table_exit_scope()`: 退出当前作用域
  - `symbol_table_lookup()`: 向上查找符号(支持作用域链)
  - `symbol_table_lookup_current_scope()`: 仅在当前作用域查找

#### 符号表结构
```c
typedef struct Symbol {
    char* name;           // 符号名称
    SymbolKind kind;      // 符号类型
    Type* type;           // 数据类型
    int scope_level;      // 作用域层级
    bool is_defined;      // 是否已定义
    
    // 变量特有信息
    struct {
        bool is_const;    // 是否为常量
        int offset;       // 栈帧偏移
    } var_info;
    
    // 函数特有信息
    struct {
        Type** param_types;
        int param_count;
        bool is_declared;
    } func_info;
} Symbol;
```

### 2. 抽象语法树(AST)系统 (`ast.h/c`)

#### AST节点类型
- **表达式节点**: 
  - 二元运算(+, -, *, /, <, >, ==, !=, &&, ||)
  - 一元运算(-, !, ++, --, &, *)
  - 字面量(整数、浮点数、字符、字符串)
  - 标识符
  - 数组访问
  - 函数调用
  - 类型转换

- **语句节点**:
  - 复合语句
  - if语句
  - while循环
  - for循环
  - return语句
  - 表达式语句
  - 声明语句
  - 赋值语句

- **声明节点**:
  - 变量声明
  - 函数声明
  - 参数声明

#### AST操作函数
- `ast_create_*()`: 创建各种类型的AST节点
- `ast_free()`: 递归释放AST树
- `ast_print()`: 打印AST树结构(用于调试)

### 3. 类型系统 (`ast.h/c`)

#### 支持的类型
- **基础类型**: void, int, float, char, bool
- **复合类型**:
  - 数组类型: `int[10]`
  - 指针类型: `int*`
  - 函数类型: `int(int, float)`

#### 类型操作
- `type_create_basic()`: 创建基础类型
- `type_create_array()`: 创建数组类型
- `type_create_pointer()`: 创建指针类型
- `type_create_function()`: 创建函数类型
- `type_equals()`: 类型相等性检查
- `type_is_compatible()`: 类型兼容性检查(支持隐式转换)
- `type_to_string()`: 类型转字符串(用于错误报告)

### 4. 类型检查器 (`type_checker.h/c`)

#### 功能
- **表达式类型推论**: 根据操作数类型推断表达式结果类型
- **类型兼容性检查**: 检查赋值、函数调用等操作的类型兼容性
- **隐式类型转换**: 支持int↔float, char↔int等隐式转换
- **错误报告**: 详细的类型错误信息,包括行号和错误原因

#### 类型检查规则
- **算术运算**: 要求操作数为算术类型(int, float, char)
- **关系运算**: 要求操作数为算术类型,结果为bool
- **逻辑运算**: 要求操作数为布尔类型,结果为bool
- **赋值运算**: 检查左值和右值类型兼容性
- **函数调用**: 检查参数数量和类型匹配

#### 类型推论示例
```c
int a = 10;
float b = 3.14;
float c = a + b;  // a隐式转换为float, 结果为float
```

### 5. 语义分析器 (`semantic_analyzer.h/c`)

#### 语义检查项目
1. **变量声明检查**:
   - 重复声明检测
   - void类型变量检测
   - 数组大小合法性检查

2. **函数声明检查**:
   - 函数签名一致性
   - 参数名称重复检测
   - main函数签名检查

3. **表达式检查**:
   - 除零检测(编译时常量)
   - 左值检查(赋值、自增自减)
   - 未声明标识符检测
   - 未初始化变量使用警告

4. **控制流分析**:
   - 所有路径返回值检查
   - 死代码检测
   - return语句位置检查

5. **常量折叠优化**:
   - 编译时常量表达式求值
   - 简化AST树

#### 错误和警告
- **错误**: 阻止编译继续的严重问题
- **警告**: 可能的问题但不阻止编译

### 6. 完整的编译流程

```
源代码 → 词法分析(Lexer) → Token流
       ↓
    语法分析(Parser) → 抽象语法树(AST)
       ↓
    符号表构建 → 填充符号表
       ↓
    类型检查 → 类型标注的AST
       ↓
    语义分析 → 验证语义正确性
       ↓
    (后续: 中间代码生成、优化、目标代码生成)
```

## 文件结构

```
c_compiler/
├── lexer.h/c           # 词法分析器(原有)
├── parser.h/c          # 语法分析器(原有)
├── ast.h/c             # AST和类型系统(新增)
├── symbol_table.h/c    # 符号表(新增)
├── type_checker.h/c    # 类型检查器(新增)
├── semantic_analyzer.h/c # 语义分析器(新增)
├── test_main.c         # 单元测试
├── demo.c              # 功能演示程序
└── README_NEW.md       # 本文档
```

## 编译和运行

### 编译测试程序
```bash
gcc -Wall -g -std=c11 -o test_compiler test_main.c ast.c symbol_table.c type_checker.c semantic_analyzer.c
./test_compiler
```

### 编译演示程序
```bash
gcc -Wall -g -std=c11 -o demo demo.c ast.c symbol_table.c type_checker.c semantic_analyzer.c
./demo
```