CC      = gcc
CFLAGS  = -Wall -Wextra -g -pthread
LDFLAGS = -pthread -lcrypto

BUILD_DIR = build
VENV_DIR = .venv
VENV_STAMP = $(VENV_DIR)/.installed
TARGETS   = server client $(VENV_STAMP)
COMMON    = $(BUILD_DIR)/utils.o

all: $(TARGETS)

server: $(BUILD_DIR)/server.o $(COMMON) $(BUILD_DIR)/hashmap.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

client: $(BUILD_DIR)/client.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c defs.h utils.h hashmap.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

$(VENV_STAMP): requirements.txt
	python3 -m venv $(VENV_DIR)
	$(VENV_DIR)/bin/pip install -r requirements.txt
	touch $@

clean:
	rm -rf $(BUILD_DIR) $(VENV_DIR) server client

.PHONY: all clean
