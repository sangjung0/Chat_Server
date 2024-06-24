# 컴파일러 및 플래그
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude -Iinclude/common -Iinclude/client -Iinclude/server
LDFLAGS = -pthread

# 디렉토리
INCLUDE_DIR = include
COMMON_INCLUDE_DIR = $(INCLUDE_DIR)/common
CLIENT_INCLUDE_DIR = $(INCLUDE_DIR)/client
SERVER_INCLUDE_DIR = $(INCLUDE_DIR)/server
SRC_DIR = src
COMMON_SRC_DIR = $(SRC_DIR)/common
CLIENT_SRC_DIR = $(SRC_DIR)/client
SERVER_SRC_DIR = $(SRC_DIR)/server
LIB_DIR = lib
BIN_DIR = bin
BUILD_DIR = build
COMMON_BUILD_DIR = $(BUILD_DIR)/common
CLIENT_BUILD_DIR = $(BUILD_DIR)/client
SERVER_BUILD_DIR = $(BUILD_DIR)/server

# 라이브러리 및 실행 파일
LIB_COMMON = $(LIB_DIR)/libcommon.a
CLIENT_BIN = $(BIN_DIR)/client
SERVER_BIN = $(BIN_DIR)/server

# 소스 및 객체 파일
COMMON_SRCS = $(wildcard $(COMMON_SRC_DIR)/*.c)
COMMON_OBJS = $(COMMON_SRCS:$(COMMON_SRC_DIR)/%.c=$(COMMON_BUILD_DIR)/%.o)
CLIENT_SRCS = $(wildcard $(CLIENT_SRC_DIR)/*.c)
CLIENT_OBJS = $(CLIENT_SRCS:$(CLIENT_SRC_DIR)/%.c=$(CLIENT_BUILD_DIR)/%.o)
SERVER_SRCS = $(wildcard $(SERVER_SRC_DIR)/*.c)
SERVER_OBJS = $(SERVER_SRCS:$(SERVER_SRC_DIR)/%.c=$(SERVER_BUILD_DIR)/%.o)

# 타겟
.PHONY: all clean client server

all: $(CLIENT_BIN) $(SERVER_BIN)

# 공통 라이브러리
$(LIB_COMMON): $(COMMON_OBJS)
	@mkdir -p $(LIB_DIR)
	ar rcs $@ $^

# 클라이언트 실행 파일
$(CLIENT_BIN): $(CLIENT_OBJS) $(LIB_COMMON)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 서버 실행 파일
$(SERVER_BIN): $(SERVER_OBJS) $(LIB_COMMON)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 공통 객체 파일
$(COMMON_BUILD_DIR)/%.o: $(COMMON_SRC_DIR)/%.c
	@mkdir -p $(COMMON_BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# 클라이언트 객체 파일
$(CLIENT_BUILD_DIR)/%.o: $(CLIENT_SRC_DIR)/%.c
	@mkdir -p $(CLIENT_BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# 서버 객체 파일
$(SERVER_BUILD_DIR)/%.o: $(SERVER_SRC_DIR)/%.c
	@mkdir -p $(SERVER_BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# 클라이언트 빌드 타겟
client: $(CLIENT_BIN)

# 서버 빌드 타겟
server: $(SERVER_BIN)

clean:
	rm -rf $(BUILD_DIR) $(LIB_COMMON) $(CLIENT_BIN) $(SERVER_BIN)