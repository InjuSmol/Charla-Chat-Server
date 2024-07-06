CC := gcc
AR := ar

SRCD := src
TSTD := tests
BLDD := build
BIND := bin
LIBD := lib
INCD := include
DEMOD := demo

EXEC := charla
TEST_EXEC := $(EXEC)_tests

MAIN := $(BLDD)/main.o
LIB := $(LIBD)/$(EXEC).a
LIB_DB := $(LIBD)/$(EXEC)_debug.a

ALL_SRCF := $(wildcard $(SRCD)/*.c)
ALL_TESTF := $(wildcard $(TSTD)/*.c)
ALL_OBJF := $(patsubst $(SRCD)/%, $(BLDD)/%, $(ALL_SRCF:.c=.o))
ALL_FUNCF := $(filter-out $(MAIN), $(ALL_OBJF))
UTIL_OBJF := $(patsubst $(UTILD)/%, $(UTILD)/%, $(ALL_SRCF:.c=.o))

INC := -I $(INCD)

CFLAGS := -Wall -Werror -Wno-unused-function -MMD -fcommon
DFLAGS := -g -DDEBUG -DCOLOR
PRINT_STAMENTS := -DERROR -DSUCCESS -DWARN -DINFO

STD := -std=gnu11
TEST_LIB := -lcriterion
LIBS := $(LIB) -lpthread -lm
LIBS_DB := $(LIB_DB) -lpthread -lm
EXCLUDES := $(INCD)/excludes.h

CFLAGS += $(STD)

.PHONY: clean all setup debug

all: setup $(BIND)/$(EXEC) $(EXCLUDES) $(BIND)/$(TEST_EXEC)

debug: CFLAGS += $(DFLAGS) $(PRINT_STAMENTS) $(COLORF)
debug: LIBS := $(LIBS_DB)
debug: all

setup: $(BIND) $(BLDD)
$(BIND):
	mkdir -p $(BIND)
$(BLDD):
	mkdir -p $(BLDD)

$(BIND)/$(EXEC): $(MAIN) $(ALL_FUNCF)
	$(CC) $^ -o $@ $(LIBS)

$(BIND)/$(TEST_EXEC): $(ALL_FUNCF) $(ALL_TESTF)
	$(CC) $(CFLAGS) $(INC) $(ALL_FUNCF) $(ALL_TESTF) $(TEST_LIB) $(LIBS) -o $@

$(BLDD)/%.o: $(SRCD)/%.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -rf $(BLDD) $(BIND)

$(EXCLUDES):
	touch $@

.PRECIOUS: $(BLDD)/*.d
-include $(BLDD)/*.d
