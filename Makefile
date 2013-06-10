# Makefile for Fedora platform only
# Modify it, if needed

CC= gcc
CFLAGS= -O2 -W -Wall -I. -fPIC -DUNQLITE_ENABLE_THREADS -D_GNU_SOURCE 
LDFLAGS= -shared
SYSLIB= -lpthread

C_FILES= \
	unqlite.c \
	./examples/unqlite_const_intro.c \
	./examples/unqlite_csr_intro.c \
	./examples/unqlite_doc_intro.c \
	./examples/unqlite_doc_io_intro.c \
	./examples/unqlite_func_intro.c \
	./examples/unqlite_hostapp_info.c \
	./examples/unqlite_huge_insert.c \
	./examples/unqlite_jx9_interp.c \
	./examples/unqlite_kv_intro.c \
	./examples/unqlite_mp3_tag.c \
	./examples/unqlite_tar.c

O_FILES= $(C_FILES:.c=.o)

LIB_UNQLITE= libunqlite.so

TESTS= \
	_unqlite_const_intro \
	_unqlite_csr_intro \
	_unqlite_doc_intro \
	_unqlite_doc_io_intro \
	_unqlite_func_intro \
	_unqlite_hostapp_info \
	_unqlite_huge_insert \
	_unqlite_jx9_interp \
	_unqlite_kv_intro \
	_unqlite_mp3_tag \
	_unqlite_tar

all: $(LIB_UNQLITE) $(TESTS)

test: all
	for t in $(TESTS); do echo "**** Runing $$t"; ./$$t || exit 1; done

$(LIB_UNQLITE): unqlite.o
	@echo -e "Building $@ ... \c"
	@$(CC) $(LDFLAGS) -o $@ $<
	@echo OK

_%: ./examples/%.o unqlite.o
	@$(CC) -o $@ $^ $(SYSLIB)
	@echo "Building $@ ... OK"

%.o: %.c
	@echo -e "$(CC) $< ... \c"
	@$(CC) -o $@ -c $(CFLAGS) $<
	@echo OK

.PHONY: clean

clean:
	rm -f $(TESTS) $(LIB_UNQLITE) *.o *~
