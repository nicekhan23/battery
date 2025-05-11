# Makefile for Multi-channel Battery Charger testing

TEST_EXEC = test_battery
DEBUG = y
CFLAGS = -Wall -pedantic -std=c99

ifeq ($(DEBUG),y)
	CFLAGS+=-g
endif

INCLUDES = -I../includes
BIN_DIR = ../bin
TEST_BIN_DIR = $(BIN_DIR)/test
EXEC = test
LIBS = -lcmocka

TEST_OBJ_FILES=$(patsubst %.c,$(TEST_BIN_DIR)/%.o, $(wildcard *.c))
OBJS=$(filter-out $(BIN_DIR)/main.o,$(wildcard $(BIN_DIR)/*.o))
$(info test_objs:$(TEST_OBJ_FILES), objs: $(OBJS))

$(TEST_BIN_DIR)/$(EXEC): $(TEST_BIN_DIR) $(OBJS) $(TEST_OBJ_FILES)
	$(CC) -o $(TEST_BIN_DIR)/$(EXEC) $(OBJS) $(TEST_OBJ_FILES) $(LIBS) $(CFLAGS)

$(TEST_BIN_DIR)/%.o:%.c
	$(CC) -o $@ -c $< $(CFLAGS) $(INCLUDES)

$(TEST_BIN_DIR):
	mkdir -p $(TEST_BIN_DIR)

clean:
	$(RM) -r $(BIN_DIR)

all: $(BIN_DIR)/$(APP_NAME)

.PHONY: clean all