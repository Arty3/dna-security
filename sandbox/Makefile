NAME     := vuln
CC       := gcc
ANALYZER := ../analyzer

CFLAGS   := -std=gnu23 -O0 -g3
CFLAGS   += -fno-stack-protector -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
CFLAGS   += -fno-pie -fcf-protection=none
CFLAGS   += -Wall -Wextra
CFLAGS   += -I$(ANALYZER)/include -DPROGRAM_NAME=\"fastq-analyzer\"

LDFLAGS  := -no-pie -z norelro

# Real pipeline, plus the sandbox's vulnerable stats.c in place of the real one.
SRCS     := $(ANALYZER)/src/main.c     \
            $(ANALYZER)/src/report.c   \
            $(ANALYZER)/src/reader.c   \
            $(ANALYZER)/src/error.c    \
            src/validate.c             \
            src/stats.c


all: $(NAME)
$(NAME): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

run: $(NAME)
	setarch $$(uname -m) -R ./$(NAME) inputs/overflow.fastq

clean:
	$(RM) $(NAME)

.PHONY: all run clean
