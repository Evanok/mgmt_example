CFLAGS += -W -Wall -Wformat -Wformat-security -Wextra  -Wno-unused-result
CFLAGS += -Wextra -Wno-long-long -Wno-variadic-macros -Werror -Wno-clobbered
CFLAGS +=  -Wno-missing-field-initializers -O0
CFLAGS += -std=gnu99
CFLAGS += -DDEBUG_MODE="$(DEBUG_MODE)"
CFLAGS += -D__DIR__=\"$(strip $(lastword $(subst /, , $(dir $(abspath $(shell echo $< | tr a-z A-Z))))))\"
CFLAGS +=-Wno-unused-variable -Wno-unused-label -Wno-unused-parameter -Wno-unused-function
CFLAGS += -g

LDFLAGS += -lpthread
HEADERS +=

CC = gcc
AR = ar

BINARY = simple_mgmt
OBJECT_FILE =   mutex.o \
		simple_mgmt.o \


all: $(BINARY)

$(BINARY):	$(OBJECT_FILE)
		$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS) $(HEADERS)

clean:
	find . -path -prune -o -name "*.o" -print0 | xargs -0 rm -f
	rm -f $(BINARY)

valgrind:
	@echo "valgrind --leak-check=full --show-reachable=yes  --show-possibly-lost=yes --track-origins=yes" >/dev/null
	@echo "valgrind --leak-check=no --show-reachable=no  --show-possibly-lost=no --track-origins=yes nano_core"

.PHONY:
