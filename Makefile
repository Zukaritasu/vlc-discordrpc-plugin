SRCS = src/plugin.c src/discord.c src/discordipc.c src/metadata.c src/settings.c

CC_WIN = x86_64-w64-mingw32-gcc
SDK_PATH_WIN = sdk64
BIN_DIR_WIN = bin64
OUTPUT_WIN = $(BIN_DIR_WIN)/libdiscordrpc_plugin.dll

CFLAGS_WIN = -Wall -Wextra -shared -O2 -DNDEBUG \
             -I"$(SDK_PATH_WIN)/include" -I"$(SDK_PATH_WIN)/include/vlc/plugins" \
             -D__PLUGIN__ -D_FILE_OFFSET_BITS=64
LDFLAGS_WIN = -L"$(SDK_PATH_WIN)/lib" -lvlccore -s

# Windows compatibility fix (Specific to native MinGW-w64/MSYS2):
# 	This fix addresses a conflict where the VLC SDK (vlc_threads.h) expects 
# 	POSIX poll() support, which is missing in native Windows environments
#
# While this compiles fine in WSL/Linux cross-compilers, native MinGW requires:
# 	1. Force-including winsock2 headers to provide missing socket definitions
# 	2. Mapping 'poll' and 'pollfd' to Windows-specific WSAPoll and WSAPOLLFD
# 	3. Setting _WIN32_WINNT to 0x0600 (Vista+) to enable WSAPoll in headers
ifeq ($(OS),Windows_NT)
	CFLAGS_WIN += -Dpoll=WSAPoll -Dpollfd=WSAPOLLFD \
				  -include winsock2.h
endif

OBJS_WIN = $(patsubst src/%.c, $(BIN_DIR_WIN)/%.o, $(SRCS))

win64: $(BIN_DIR_WIN) $(OUTPUT_WIN)

$(OUTPUT_WIN): $(OBJS_WIN)
	$(CC_WIN) $(CFLAGS_WIN) $(OBJS_WIN) -o $(OUTPUT_WIN) $(LDFLAGS_WIN)

$(BIN_DIR_WIN)/%.o: src/%.c | $(BIN_DIR_WIN)
	$(CC_WIN) $(CFLAGS_WIN) -c $< -o $@

$(BIN_DIR_WIN):
	mkdir -p $(BIN_DIR_WIN)


CC_LINUX = gcc
BIN_DIR_LINUX = bin_linux
OUTPUT_LINUX = $(BIN_DIR_LINUX)/libdiscordrpc_plugin.so

CFLAGS_LINUX = -Wall -Wextra -shared -fPIC -O2 -DNDEBUG \
               -I/usr/include/vlc/plugins -D__PLUGIN__ -D_FILE_OFFSET_BITS=64
LDFLAGS_LINUX = -lvlccore

OBJS_LINUX = $(patsubst src/%.c, $(BIN_DIR_LINUX)/%.o, $(SRCS))

linux64: $(BIN_DIR_LINUX) $(OUTPUT_LINUX)

$(OUTPUT_LINUX): $(OBJS_LINUX)
	$(CC_LINUX) $(CFLAGS_LINUX) $(OBJS_LINUX) -o $(OUTPUT_LINUX) $(LDFLAGS_LINUX)

$(BIN_DIR_LINUX)/%.o: src/%.c | $(BIN_DIR_LINUX)
	$(CC_LINUX) $(CFLAGS_LINUX) -c $< -o $@

$(BIN_DIR_LINUX):
	mkdir -p $(BIN_DIR_LINUX)

clean:
	rm -f $(OBJS_WIN) $(OUTPUT_WIN) $(OBJS_LINUX) $(OUTPUT_LINUX)

.PHONY: win64 linux64 clean
