CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11
TARGET = compiler
OBJS = main.o lexer.o ast.o symbol_table.o type_checker.o semantic_analyzer.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c lexer.h ast.h symbol_table.h type_checker.h semantic_analyzer.h
	$(CC) $(CFLAGS) -c main.c

lexer.o: lexer.c lexer.h
	$(CC) $(CFLAGS) -c lexer.c

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c ast.c

symbol_table.o: symbol_table.c symbol_table.h ast.h
	$(CC) $(CFLAGS) -c symbol_table.c

type_checker.o: type_checker.c type_checker.h ast.h symbol_table.h
	$(CC) $(CFLAGS) -c type_checker.c

semantic_analyzer.o: semantic_analyzer.c semantic_analyzer.h ast.h symbol_table.h type_checker.h
	$(CC) $(CFLAGS) -c semantic_analyzer.c

clean:
	rm -f $(OBJS) $(TARGET)

test: $(TARGET)
	./$(TARGET) test_legal.c
	@echo ""
	@echo "=== 测试多错误文件 ==="
	./$(TARGET) test_multiple_errors.c || true

.PHONY: all clean test
