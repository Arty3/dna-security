NAME    := fastq-analyzer

CC      := gcc
CFLAGS  := -std=gnu23 -march=native -Ofast -flto -Wall -Wextra -Werror
CFLAGS  += -Iinclude -DPROGRAM_NAME=\"$(NAME)\"
LDFLAGS :=

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

SRCS := $(addprefix $(SRC_DIR)/, \
	    main.c                   \
	    error.c                  \
	    reader.c                 \
	    validate.c               \
	    stats.c                  \
	    report.c)

OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
BIN  := $(BIN_DIR)/$(NAME)

all: $(BIN)

$(BIN): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	$(RM) -r $(OBJ_DIR)

fclean: clean
	$(RM) -r $(BIN_DIR)

re: fclean
	$(MAKE) --no-print-directory all

-include $(DEPS)

.PHONY: all clean fclean re
