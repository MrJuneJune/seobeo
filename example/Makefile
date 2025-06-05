CC = gcc
CFLAGS = -I../lib -lpq
BIN_DIR = bin
TEST_DIR = test
LIB_DIR = ../lib
MODEL_SRCS := $(wildcard $(LIB_DIR)/model_*.c)

start: lib/connection.o auto_generate_sql
	gcc -I../include main.c ../src/*.c $(LIB_DIR)/connection.o  $(MODEL_SRCS) $(CFLAGS) -o $(BIN_DIR)/example_server && ./$(BIN_DIR)/example_server

lib/connection.o: $(LIB_DIR)/connection.c $(LIB_DIR)/connection.h
	$(CC) -c $(LIB_DIR)/connection.c -o $(LIB_DIR)/connection.o $(CFLAGS)

auto_generate_sql: $(LIB_DIR)/auto_generate.c $(LIB_DIR)/auto_generate.h
	$(CC) $(LIB_DIR)/auto_generate.c -o $(BIN_DIR)/auto_generate $(CFLAGS) && ./$(BIN_DIR)/auto_generate

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR) *.o $(LIB_DIR)/*.o


