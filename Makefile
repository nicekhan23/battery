# Makefile for Multi-channel Battery charger

EXEC = battery
DEBUG = y
CFLAGS = -Wall -pedantic -std=c99

ifeq ($(DEBUG), y)
	CFLAGS+=-g
endif

CMOCKA_VERBOSE ?= y
ifeq ($(CMOCKA_VERBOSE),y)
CFLAGS+=-DCMOCKA_VERBOSE_OUTPUT
endif

INCLUDES = -I includes
SRC_DIR = src
TEST_DIR = test
BIN_DIR = bin
DOCS_DIR = docs
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))

$(info objs: $(OBJ_FILES))

$(BIN_DIR)/$(EXEC): $(BIN_DIR) $(OBJ_FILES)
	$(CC) -o $(BIN_DIR)/$(EXEC) $(OBJ_FILES) $(CFLAGS)

$(BIN_DIR)/%.o:$(SRC_DIR)/%.c
	$(CC) -o $@ -c $< $(INCLUDES) $(CFLAGS)
	
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

test: $(BIN_DIR)/$(EXEC)
	$(MAKE) -C test
	
docs:
	doxygen Doxyfile

clean:
	$(RM) -r $(BIN_DIR)
	$(RM) -r $(DOCS_DIR)

all: $(BIN_DIR)/$(EXEC)

.PHONY: clean docs test all
