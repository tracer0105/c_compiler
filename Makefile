CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11
TARGET = compiler

DEMO_TARGET = demo
TEST_MAIN_TARGET = test_main
OBJS = main.o lexer.o ast.o symbol_table.o type_checker.o semantic_analyzer.o

DEMO_OBJS = demo.o ast.o symbol_table.o type_checker.o semantic_analyzer.o lexer.o
TEST_MAIN_OBJS = test_main.o ast.o symbol_table.o type_checker.o semantic_analyzer.o lexer.o

all: $(TARGET) $(DEMO_TARGET) $(TEST_MAIN_TARGET) 

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)


$(DEMO_TARGET): $(DEMO_OBJS)
	$(CC) $(CFLAGS) -o $(DEMO_TARGET) $(DEMO_OBJS)


$(TEST_MAIN_TARGET): $(TEST_MAIN_OBJS)
	$(CC) $(CFLAGS) -o $(TEST_MAIN_TARGET) $(TEST_MAIN_OBJS)


demo.o: demo.c ast.h symbol_table.h type_checker.h semantic_analyzer.h lexer.h
	$(CC) $(CFLAGS) -c demo.c -o demo.o


test_main.o: test_main.c ast.h symbol_table.h type_checker.h semantic_analyzer.h lexer.h
	$(CC) $(CFLAGS) -c test_main.c -o test_main.o

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
	rm -f $(OBJS) $(TARGET) $(DEMO_OBJS) $(DEMO_TARGET) $(TEST_MAIN_OBJS) $(TEST_MAIN_TARGET)  # 新增：清理demo和test_main的文件

test: $(TARGET)
	./$(TARGET) test_legal.c
	@echo ""
	@echo "=== 测试多错误文件 ==="
	./$(TARGET) test_multiple_errors.c || true

run-demo: $(DEMO_TARGET)
	./$(DEMO_TARGET)

run-test-main: $(TEST_MAIN_TARGET)
	./$(TEST_MAIN_TARGET)

.PHONY: all clean test run-demo run-test-main  