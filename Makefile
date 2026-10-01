CC      = gcc
CFLAGS  = -Wall -Wextra -g
LDFLAGS = -pthread -lcrypto

BUILD_DIR = build
TARGETS   = $(BUILD_DIR)/server $(BUILD_DIR)/client
COMMON    = $(BUILD_DIR)/utils.o

all: $(TARGETS)

$(BUILD_DIR)/server: $(BUILD_DIR)/server.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/client: $(BUILD_DIR)/client.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c defs.h utils.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
