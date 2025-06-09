CC = gcc
CFLAGS = -Iinclude -I./example/third_party/include -Lbuild -L./example/third_party -lpog_pool -lseobeo -lpq -ljansson
BIN_DIR = bin
SRC_DIR = src
BUILD_DIR = build
EXAMPLE_BUILD = example/build
MODEL_SRCS := $(wildcard example/build/model_*.c)

# Need to use find since we create these models. I auto set it to build path.
example: example/main.c auto_generate seobeo | $(BIN_DIR)
	$(CC) example/main.c $(MODEL_SRCS) -I./example/build  $(CFLAGS) -o $(BIN_DIR)/main && \
	cd example && \
	../$(BIN_DIR)/main

auto_generate: $(EXAMPLE_BUILD)
	$(CC) example/generate_models.c $(CFLAGS) -o $(BIN_DIR)/auto_generate
	cd example && \
	../$(BIN_DIR)/auto_generate

seobeo: helper.o server.o
	ar rcs $(BUILD_DIR)/libseobeo.a $(BUILD_DIR)/*.o

helper.o:  $(SRC_DIR)/helper.c | $(BUILD_DIR)
	$(CC) -c $(SRC_DIR)/helper.c -o $(BUILD_DIR)/helper.o -Iinclude

server.o:  $(SRC_DIR)/server.c | $(BUILD_DIR)
	$(CC) -c $(SRC_DIR)/server.c  -o $(BUILD_DIR)/server.o -Iinclude

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(EXAMPLE_BUILD):
	mkdir -p $(EXAMPLE_BUILD)

clean:
	rm -rf $(BIN_DIR) *.o $(LIB_DIR)/*.o
