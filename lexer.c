#include "lexer.h"
#include <string.h>
#include <ctype.h>

char *strdup(const char *s);

Token current_token; // 全局当前Token，供语法分析器使用
static int peek_char();

// 预读下一个Token并更新current_token
void lexer_peek_next_token() {
    // 仅当value非空时释放，避免空指针free
    if (current_token.value != NULL) {
        token_free(current_token);
        current_token.value = NULL; // 释放后置空，防止野指针
    }
    current_token = lexer_next_token();
}

// 全局变量
FILE* input_file;
char current_char;
// 关键字映射表
typedef struct {
    const char* keyword;
    TokenType type;
} KeywordEntry;

KeywordEntry keyword_table[] = {
    {"if", TOKEN_IF}, {"else", TOKEN_ELSE}, {"int", TOKEN_INT},
    {"return", TOKEN_RETURN}, {"void", TOKEN_VOID}, {"while", TOKEN_WHILE},
    {NULL, TOKEN_ERROR} // 结束标记
};

// 读取下一个字符
static void read_char() {
    int c = fgetc(input_file); // 用int接收
    if (c == EOF) {
        current_char = EOF; // 明确标记为EOF，而非乱码字符
    } else {
        current_char = (char)c;
    }
}


// 跳过空白符（空格/制表符/换行）
static void skip_whitespace() {
    while (current_char != EOF && isspace(current_char)) {
        read_char();
    }
}

// 处理注释
static void skip_comment() {
    if (current_char == '/') {
        int next_char = fgetc(input_file); // 先读取下一个字符
        // 单行注释
        if (next_char == '/') {
            while ((current_char = fgetc(input_file)) != EOF && current_char != '\n');
        }
        // 多行注释
        else if (next_char == '*') {
            int prev_char = 0;
            while ((current_char = fgetc(input_file)) != EOF) {
                if (prev_char == '*' && current_char == '/') {
                    current_char = fgetc(input_file); // 跳过 '/'
                    break;
                }
                prev_char = current_char;
            }
        }
        // 不是注释，回退字符
        else {
            ungetc(next_char, input_file); // 回退第二个字符
            current_char = '/'; // 恢复第一个字符为 '/'
        }
    }
}

// 解析标识符/关键字
static Token parse_identifier() {
    Token token;
    char buffer[256] = {0}; // 标识符最大长度256
    int pos = 0;

    // 读取字母/数字/下划线
    while (current_char != EOF && (isalnum(current_char) || current_char == '_')) {
        buffer[pos++] = current_char;
        read_char();
    }

    // 检查是否为关键字
    for (int i = 0; keyword_table[i].keyword; i++) {
        if (strcmp(buffer, keyword_table[i].keyword) == 0) {
            token.type = keyword_table[i].type;
            token.value = strdup(buffer);
            return token;
        }
    }

    // 不是关键字则为标识符
    token.type = TOKEN_IDENTIFIER;
    token.value = strdup(buffer);
    return token;
}

// 解析整型常量（十进制/八进制/十六进制）
static Token parse_int_const() {
    Token token;
    char buffer[32] = {0};
    int pos = 0;

    // 八进制（0开头）
    if (current_char == '0') {
        buffer[pos++] = '0';
        read_char();
        if (current_char >= '0' && current_char <= '7') {
            while (current_char != EOF && current_char >= '0' && current_char <= '7') {
                buffer[pos++] = current_char;
                read_char();
            }
        }
        // 十六进制（0x/0X）
        else if (current_char == 'x' || current_char == 'X') {
            buffer[pos++] = current_char;
            read_char();
            while (current_char != EOF && (isxdigit(current_char))) {
                buffer[pos++] = current_char;
                read_char();
            }
        }
    }
    // 十进制（1-9开头）
    else {
        while (current_char != EOF && isdigit(current_char)) {
            buffer[pos++] = current_char;
            read_char();
        }
    }

    token.type = TOKEN_INT_CONST;
    token.value = strdup(buffer);
    return token;
}

// 解析运算符/分界符（优先处理复合运算符）
static Token parse_operator_delimiter() {
    Token token = {0}; // 初始化Token，避免野指针
    token.value = (char*)malloc(3 * sizeof(char)); // 复合运算符最多2字符+终止符
    if (token.value == NULL) { // 内存分配失败处理
        perror("malloc failed for token.value");
        token.type = TOKEN_ERROR;
        token.value = strdup("?");
        return token;
    }

    switch (current_char) {
        // 分界符
        case '{': 
            token.type = TOKEN_LBRACE; 
            token.value[0] = '{'; 
            token.value[1] = '\0'; 
            read_char(); 
            break;
        case '}': 
            token.type = TOKEN_RBRACE; 
            token.value[0] = '}'; 
            token.value[1] = '\0'; 
            read_char(); 
            break;
        case '(': 
            token.type = TOKEN_LPAREN; 
            token.value[0] = '('; 
            token.value[1] = '\0'; 
            read_char(); 
            break;
        case ')': 
            token.type = TOKEN_RPAREN; 
            token.value[0] = ')'; 
            token.value[1] = '\0'; 
            read_char(); 
            break;
        case ';': 
            token.type = TOKEN_SEMICOLON; 
            token.value[0] = ';'; 
            token.value[1] = '\0'; 
            read_char(); 
            break;
        case ',': 
            token.type = TOKEN_COMMA; 
            token.value[0] = ','; 
            token.value[1] = '\0'; 
            read_char(); 
            break;
        
        // 复合运算符
        case '>':
            read_char();
            if (current_char != EOF && current_char == '=') { // 避免EOF时调用ungetc
                token.type = TOKEN_GE; 
                token.value[0] = '>'; 
                token.value[1] = '='; 
                token.value[2] = '\0'; 
                read_char();
            } else { 
                token.type = TOKEN_GT; 
                token.value[0] = '>'; 
                token.value[1] = '\0'; 
                if (current_char != EOF) ungetc(current_char, input_file); // EOF不回退
            }
            break;
        case '<':
            read_char();
            if (current_char != EOF && current_char == '=') {
                token.type = TOKEN_LE; 
                token.value[0] = '<'; 
                token.value[1] = '='; 
                token.value[2] = '\0'; 
                read_char();
            } else { 
                token.type = TOKEN_LT; 
                token.value[0] = '<'; 
                token.value[1] = '\0'; 
                if (current_char != EOF) ungetc(current_char, input_file);
            }
            break;
        case '=':
            read_char();
            if (current_char != EOF && current_char == '=') {
                token.type = TOKEN_EQ; 
                token.value[0] = '='; 
                token.value[1] = '='; 
                token.value[2] = '\0'; 
                read_char();
            } else { 
                token.type = TOKEN_ASSIGN; 
                token.value[0] = '='; 
                token.value[1] = '\0'; 
                if (current_char != EOF) ungetc(current_char, input_file);
            }
            break;
        case '!':
            read_char();
            if (current_char != EOF && current_char == '=') {
                token.type = TOKEN_NE; 
                token.value[0] = '!'; 
                token.value[1] = '='; 
                token.value[2] = '\0'; 
                read_char();
            } else { 
                token.type = TOKEN_ERROR; 
                token.value[0] = '!'; 
                token.value[1] = '\0'; 
                if (current_char != EOF) ungetc(current_char, input_file);
            }
            break;
        case '+':
            read_char();
            if (current_char != EOF && current_char == '+') {
                token.type = TOKEN_INC; 
                token.value[0] = '+'; 
                token.value[1] = '+'; 
                token.value[2] = '\0'; 
                read_char();
            } else if (current_char != EOF && current_char == '=') {
                token.type = TOKEN_PLUS_EQ; 
                token.value[0] = '+'; 
                token.value[1] = '='; 
                token.value[2] = '\0'; 
                read_char();
            } else { 
                token.type = TOKEN_PLUS; 
                token.value[0] = '+'; 
                token.value[1] = '\0'; 
                if (current_char != EOF) ungetc(current_char, input_file);
            }
            break;
        case '-':
            read_char();
            if (current_char != EOF && current_char == '-') {
                token.type = TOKEN_DEC; 
                token.value[0] = '-'; 
                token.value[1] = '-'; 
                token.value[2] = '\0'; 
                read_char();
            } else if (current_char != EOF && current_char == '=') {
                token.type = TOKEN_MINUS_EQ; 
                token.value[0] = '-'; 
                token.value[1] = '='; 
                token.value[2] = '\0'; 
                read_char();
            } else { 
                token.type = TOKEN_MINUS; 
                token.value[0] = '-'; 
                token.value[1] = '\0'; 
                if (current_char != EOF) ungetc(current_char, input_file);
            }
            break;
        
        // 单目运算符：修复除法处理（与注释解耦）
        case '*': 
            token.type = TOKEN_MUL; 
            token.value[0] = '*'; 
            token.value[1] = '\0'; 
            read_char(); 
            break;
        case '/': 
            token.type = TOKEN_DIV; 
            token.value[0] = '/'; 
            token.value[1] = '\0'; 
            read_char(); 
            break;
        
        // 未知字符/EOF处理
        default:
            // 处理EOF：释放原malloc内存，避免泄漏
            if (current_char == EOF) {
                free(token.value); // 释放之前malloc的3字节
                token.type = TOKEN_EOF;
                token.value = strdup("EOF");
                read_char();
            } else {
                // 非法字符处理
                fprintf(stderr, "ERROR: 未知字符 [ASCII: %d, Char: '%c']\n", current_char, current_char);
                token.type = TOKEN_ERROR; 
                token.value[0] = isprint(current_char) ? current_char : '?'; 
                token.value[1] = '\0'; 
                read_char(); 
            }
            break;
    }
    return token;
}

// 获取下一个Token
Token lexer_next_token() {
    // 初始化循环：持续读取直到非空白/注释或EOF
    while (current_char != EOF) {
        skip_whitespace(); // 跳过所有空白字符（空格、换行、制表符等）
        if (current_char == '/') {
            // 先判断是否是注释，再决定是否处理除法
            if (peek_char() == '/' || peek_char() == '*') {
                skip_comment(); // 是注释则跳过
                continue;
            } else {
                break; // 不是注释，是除法运算符，退出循环处理
            }
        }
        // 非空白/注释，退出循环处理有效字符
        break;
    }

    // 处理文件末尾：直接返回EOF Token
    if (current_char == EOF) {
        Token eof_token = {0}; // 初始化Token
        eof_token.type = TOKEN_EOF;
        eof_token.value = strdup("EOF");
        return eof_token;
    }

    // 处理有效字符
    if (isalpha(current_char) || current_char == '_') {
        return parse_identifier();
    } else if (isdigit(current_char)) {
        return parse_int_const();
    } else {
        return parse_operator_delimiter();
    }
}


// 释放Token内存
void token_free(Token token) {
    if (token.value != NULL) {
        free(token.value);
        // 清空指针，避免后续重复释放
        token.value = NULL;
    }
}

// 初始化时读取第一个Token
int lexer_init(const char* filename) {
    input_file = fopen(filename, "r");
    if (!input_file) { 
        perror("Open file failed"); 
        return -1; 
    }
    // 初始化current_token
    read_char();
    current_token = lexer_next_token();
    return 0;
}

void lexer_close() {
    // 释放current_token
    token_free(current_token);
    if (input_file) fclose(input_file);
}

Token token_copy(Token src) {
    Token dest = {0};
    dest.type = src.type;
    // 深拷贝value：重新分配内存，避免指针共享
    if (src.value != NULL) {
        dest.value = strdup(src.value);
        if (dest.value == NULL) {
            perror("strdup failed");
            dest.type = TOKEN_ERROR;
            dest.value = strdup("?");
        }
    } else {
        dest.value = strdup("NULL");
    }
    return dest;
}

// 查看下一个字符（不移动文件指针）
static int peek_char() {
    int c = fgetc(input_file);
    if (c != EOF) {
        ungetc(c, input_file); // 回退字符
    }
    return c;
}