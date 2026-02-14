CC = x86_64-w64-mingw32-gcc
SDK_PATH = sdk
BIN_DIR = bin
OUTPUT_NAME = $(BIN_DIR)/libdiscordrpc_plugin.dll

CFLAGS = -Wall -Wextra -shared -O2 -DNDEBUG  -I"$(SDK_PATH)/include" -I"$(SDK_PATH)/include/vlc/plugins" -D__PLUGIN__ -D_FILE_OFFSET_BITS=64
LDFLAGS = -L"$(SDK_PATH)/lib" -lvlccore -s

SRCS = src/plugin.c src/discord.c src/discordipc.c src/metadata.c src/settings.c
OBJS = $(patsubst src/%.c, $(BIN_DIR)/%.o, $(SRCS))

all: $(BIN_DIR) $(OUTPUT_NAME)

$(OUTPUT_NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(OUTPUT_NAME) $(LDFLAGS)

$(BIN_DIR)/%.o: src/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -f $(OBJS) $(OUTPUT_NAME)

.PHONY: all clean
