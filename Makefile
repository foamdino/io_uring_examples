CPPFLAGS ?=
override CPPFLAGS += -D_GNU_SOURCE -I../src/include/
CFLAGS ?= -g -O2 -Wall
LDFLAGS ?= -luring

example_srcs := \
	cat/cat.c \
	cat/cat_io_uring.c \
	cat/cat_liburing.c \
	cp/cp_liburing.c \
	http/http_liburing.c

all_targets :=

example_targets := $(patsubst %.c,%,$(patsubst %.cc,%,$(example_srcs)))
all_targets += $(example_targets)


all: $(example_targets)

%: %.c
	$(QUIET_CC)$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	@rm -f $(all_targets)

.PHONY: all clean