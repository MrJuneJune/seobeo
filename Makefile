CC = gcc
# TODO: Remove jasson and use custom one.
REST_API_CFLAGS = -Iinclude -I./example/rest_api/third_party/include -L./example/rest_api/third_party/lib -Lbuild -L./example/rest_api -lpog_pool -lseobeo -lpq -ljansson
CFLAGS =  -Iinclude -Lbuild -lseobeo 
BIN_DIR = bin
SRC_DIR = src
BUILD_DIR = build
EXAMPLE_BUILD = example/build
MODEL_SRCS := $(wildcard example/rest_api/build/model_*.c)

# Find os specific
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	SERVER_SRC := $(BUILD_DIR)/server_mac.o
else
	SERVER_SRC := $(BUILD_DIR)/server_linux.o
endif

rest_api_example: example/rest_api/main.c auto_generate seobeo | $(BIN_DIR)
	$(CC) example/rest_api/main.c $(MODEL_SRCS) $(REST_API_CFLAGS) -o $(BIN_DIR)/rest_api_server && \
	cd example/rest_api && ../../$(BIN_DIR)/rest_api_server

stand_alone_example:  example/stand_alone/main.c seobeo | $(BIN_DIR)
	$(CC) example/stand_alone/main.c  $(CFLAGS) -o $(BIN_DIR)/stand_alone_server && \
	cd example/stand_alone && ../../$(BIN_DIR)/stand_alone_server

# Related to PogPool
auto_generate: | $(BIN_DIR)
	$(CC) example/rest_api/generate_models.c $(REST_API_CFLAGS) -o $(BIN_DIR)/auto_generate && \
	cd example/rest_api && ../../$(BIN_DIR)/auto_generate

seobeo: $(BUILD_DIR)/helper.o $(BUILD_DIR)/server.o $(SERVER_SRC)
	ar rcs $(BUILD_DIR)/libseobeo.a $^

$(BUILD_DIR)/server.o: $(SRC_DIR)/server.c | $(BUILD_DIR)
	$(CC) -c $< -o $@ -Iinclude

$(BUILD_DIR)/server_mac.o: $(SRC_DIR)/mac/server.c | $(BUILD_DIR)
	$(CC) -c $< -o $@ -Iinclude

$(BUILD_DIR)/server_linux.o: $(SRC_DIR)/linux/server.c | $(BUILD_DIR)
	$(CC) -c $< -o $@ -Iinclude

$(BUILD_DIR)/helper.o: $(SRC_DIR)/helper.c | $(BUILD_DIR)
	$(CC) -c $< -o $@ -Iinclude

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(EXAMPLE_BUILD):
	mkdir -p $(EXAMPLE_BUILD)

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR) $(EXAMPLE_BUILD)

