CC      = gcc
CFLAGS  = -Wall -Wextra -g -pthread
LDFLAGS = -pthread -lcrypto

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
VENV_DIR = .venv
VENV_STAMP = $(VENV_DIR)/.installed
TARGETS   = $(BIN_DIR)/server $(BIN_DIR)/client $(VENV_STAMP)
COMMON    = $(BUILD_DIR)/utils.o

all: $(TARGETS)

server: $(BIN_DIR)/server

client: $(BIN_DIR)/client

$(BIN_DIR)/server: $(BUILD_DIR)/server.o $(COMMON) $(BUILD_DIR)/hashmap.o | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/client: $(BUILD_DIR)/client.o $(COMMON) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/defs.h $(SRC_DIR)/utils.h $(SRC_DIR)/hashmap.h Makefile | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

$(VENV_STAMP): web/requirements.txt
	python3 -m venv $(VENV_DIR)
	$(VENV_DIR)/bin/pip install -r web/requirements.txt
	touch $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(VENV_DIR)

.PHONY: all clean server client
