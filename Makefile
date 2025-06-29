CC = gcc
# TODO: Remove jasson and use custom one.
REST_API_CFLAGS = -Iinclude -I./example/rest_api/third_party/include -L./example/rest_api/third_party/lib -Lbuild -L./example/rest_api -lpog_pool -lseobeo -lpq -ljansson
CFLAGS =  -Iinclude -Lbuild -lseobeo 
BIN_DIR = bin
SRC_DIR = src
BUILD_DIR = build
EXAMPLE_BUILD = example/rest_api/build
MODEL_SRCS := $(wildcard example/rest_api/build/model_*.c)

# For rest api example, we are using libpq, pog_pool, and jansson
THIRD_PARTY_PREFIX := $(CURDIR)/example/rest_api/third_party
THIRD_PARTY_INCLUDE_DIR := $(THIRD_PARTY_PREFIX)/include
THIRD_PARTY_LIB_DIR := $(THIRD_PARTY_PREFIX)/lib

# Find os specific
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	SERVER_SRC := $(BUILD_DIR)/server_mac.o
  INSTALL_THIRD_PARTY_SRC := install_third_party_mac
else
	SERVER_SRC := $(BUILD_DIR)/server_linux.o
  INSTALL_THIRD_PARTY_SRC := install_third_party_linux
endif

# Examples
#
rest_api_example: install_third_party example/rest_api/main.c auto_generate seobeo | $(BIN_DIR)
	$(CC) example/rest_api/main.c $(MODEL_SRCS) $(REST_API_CFLAGS) -o $(BIN_DIR)/rest_api_server && \
	cd example/rest_api && ../../$(BIN_DIR)/rest_api_server

stand_alone_example:  example/stand_alone/main.c seobeo | $(BIN_DIR)
	$(CC) example/stand_alone/main.c  $(CFLAGS) -o $(BIN_DIR)/stand_alone_server && \
	cd example/stand_alone && ../../$(BIN_DIR)/stand_alone_server

# Related to PogPool
auto_generate: $(EXAMPLE_BUILD) | $(BIN_DIR)
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

# These are needed for the rest example
install_third_party: delete_third_party $(INSTALL_THIRD_PARTY_SRC)
	@if [ -d $(THIRD_PARTY_INCLUDE_DIR)/postgresql ] && [ -d  $(THIRD_PARTY_INCLUDE_DIR) ]; then \
		echo "Third party libs are already installed."; \
	else \
		read -p "Do you want to install postgresql via homebrew or apt? [y/N] " answer; \
		if [ "$$answer" = "y" ] || [ "$$answer" = "Y" ]; then \
		  $(MAKE) $(INSTALL_THIRD_PARTY_SRC); \
		else \
			echo "Skip installations. Please move libpq yourself."; \
		fi \
	fi

install_third_party_mac: prepare_dirs
	@echo "Installing libpq and jansson via Homebrew..."
	brew install libpq jansson
	@echo "Cloning and building pog_pool..."
	@if [ -d pog_pool ]; then \
		echo "Skip install pog_pool"; \
	else \
	  git clone https://github.com/MrJuneJune/pog_pool.git; \
	fi
	cd pog_pool && make release 
	@echo "Copying pog_pool headers and libs to third_party..."
	cp -r pog_pool/dist/include/* $(THIRD_PARTY_INCLUDE_DIR)/pog_pool
	cp -r pog_pool/dist/libpog_pool.a $(THIRD_PARTY_LIB_DIR)/
	rm -rf pog_pool
	@echo "Copying libpq headers and libs to third_party..."
	cp -r /opt/homebrew/opt/libpq/include/* $(THIRD_PARTY_INCLUDE_DIR)/postgresql/
	cp -r /opt/homebrew/opt/libpq/lib/* $(THIRD_PARTY_LIB_DIR)/
	@echo "Copying jansson headers and libs to third_party..."
	cp -r /opt/homebrew/opt/jansson/include/* $(THIRD_PARTY_INCLUDE_DIR)/
	cp -r /opt/homebrew/opt/jansson/lib/* $(THIRD_PARTY_LIB_DIR)/

install_third_party_linux: prepare_dirs
	@echo "Installing libpq and jansson via apt..."
	sudo apt-get update
	sudo apt-get install -y libpq-dev libjansson-dev
	@echo "Cloning and building pog_pool..."
	git clone https://github.com/MrJuneJune/pog_pool.git
	cd pog_pool && make release
	@echo "Copying pog_pool headers and libs to third_party..."
	cp -r pog_pool/dist/include/* $(THIRD_PARTY_INCLUDE_DIR)/
	cp -r pog_pool/dist/libpog_pool.a $(THIRD_PARTY_LIB_DIR)/
	cp -r pog_pool/include/* $(THIRD_PARTY_INCLUDE_DIR)/
	@echo "Copying libpq headers and libs to third_party..."
	cp -r /usr/include/postgresql/* $(THIRD_PARTY_INCLUDE_DIR)/postgresql/
	cp -r /usr/lib/x86_64-linux-gnu/*libpq.* $(THIRD_PARTY_LIB_DIR)/
	@echo "Copying jansson headers and libs to third_party..."
	cp -r /usr/include/jansson.h $(THIRD_PARTY_INCLUDE_DIR)/
	cp -r /usr/lib/x86_64-linux-gnu/*jansson.* $(THIRD_PARTY_LIB_DIR) 

prepare_dirs:
	mkdir -p $(THIRD_PARTY_INCLUDE_DIR)/postgresql
	mkdir -p $(THIRD_PARTY_INCLUDE_DIR)/pog_pool
	mkdir -p $(THIRD_PARTY_LIB_DIR)

# Need to do this due to ownership issues
delete_third_party:
	rm -rf $(THIRD_PARTY_PREFIX)
	rm -rf pog_pool

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(EXAMPLE_BUILD):
	mkdir -p $(EXAMPLE_BUILD)

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR) $(EXAMPLE_BUILD)

