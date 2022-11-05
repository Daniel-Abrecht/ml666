.SECONDARY:

HEADERS := $(shell find include -type f -name "*.h" -not -name ".*")
SOURCES := $(shell find src -iname "*.c")

SONAME = ml666
MAJOR  = 0
MINOR  = 0
PATCH  = 0

prefix = /usr/local/

TYPE := release

ifdef debug
TYPE := debug
CFLAGS  += -O0 -g
LDFLAGS += -g
else
CFLAGS  += -O3
endif

ifdef asan
TYPE := $(TYPE)-asan
CFLAGS  += -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined
endif

CFLAGS  += --std=c17
CFLAGS  += -Iinclude
CFLAGS  += -Wall -Wextra -pedantic -Werror
CFLAGS  += -fstack-protector-all
CFLAGS  += -Wno-missing-field-initializers

CFLAGS  += -fvisibility=hidden -DML666_BUILD

ifndef debug
CFLAGS  += -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections
endif

OBJECTS := $(patsubst %,build/$(TYPE)/o/%.o,$(SOURCES))

.PHONY: all bin lib docs clean get-bin get-lib install uninstall shell test clean-docs install-docs uninstall-docs

all: bin lib docs

bin: bin/$(TYPE)/ml666-tokenizer-example \
     bin/$(TYPE)/ml666

lib: lib/$(TYPE)/libml666.a \
     lib/$(TYPE)/libml666.so

get-bin:
	@echo bin/$(TYPE)/

get-lib:
	@echo lib/$(TYPE)/

bin/$(TYPE)/%: build/$(TYPE)/o/src/main/%.c.o lib/$(TYPE)/libml666.so
	mkdir -p $(dir $@)
	$(CC) -o $@ $(LDFLAGS) $< -Llib/$(TYPE)/ -lml666

lib/$(TYPE)/lib$(SONAME).so: lib/$(TYPE)/libml666.a
	ln -sf "lib$(SONAME).so" "$@.0"
	$(CC) -o $@ -Wl,--no-undefined -Wl,-soname,lib$(SONAME).so.$(MAJOR) --shared -fPIC $(LDFLAGS) -Wl,--whole-archive $^ -Wl,--no-whole-archive

lib/$(TYPE)/lib$(SONAME).a: $(filter-out build/$(TYPE)/o/src/main/%,$(OBJECTS))
	mkdir -p $(dir $@)
	rm -f $@
	$(AR) q $@ $^

build/$(TYPE)/o/%.c.o: %.c makefile $(HEADERS)
	mkdir -p $(dir $@)
	$(CC) -fPIC -c -o $@ $(DFLAGS) $(CFLAGS) $<

build/test/%: test/%.ml666 test/%.result bin/$(TYPE)/ml666-tokenizer-example
	mkdir -p $(dir $@)
	LD_LIBRARY_PATH="$$PWD/lib/$(TYPE)/" \
	bin/$(TYPE)/ml666-tokenizer-example <"$<" >"$@.result"
	diff "$@.result" "$(word 2,$^)" && touch "$@"

clean: clean-docs
	rm -rf build/$(TYPE)/ bin/$(TYPE)/ lib/$(TYPE)/

install:
	cp "lib/$(TYPE)/lib$(SONAME).so" "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)"
	ln -sf "lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)" "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR).$(MINOR)"
	ln -sf "lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)" "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR)"
	ln -sf "lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)" "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so"
	cp "bin/$(TYPE)/ml666" "$(DESTDIR)$(prefix)/bin/ml666"
	cp "lib/$(TYPE)/lib$(SONAME).a" "$(DESTDIR)$(prefix)/lib/lib$(SONAME).a"
	cp -a include/ml666/./ "$(DESTDIR)$(prefix)/include/ml666/"
	ldconfig

uninstall:
	rm -f "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)"
	rm -f "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR).$(MINOR)"
	rm -f "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR)"
	rm -f "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so"
	rm -f "$(DESTDIR)$(prefix)/lib/lib$(SONAME).a"
	rm -f "$(DESTDIR)$(prefix)/bin/ml666"
	rm -rf "$(DESTDIR)$(prefix)/include/ml666/"

install-docs:
	mkdir -p "$(DESTDIR)$(prefix)/share/man/man3/"
	rm -f "$(DESTDIR)$(prefix)/share/man/man3/"ml666*.3
	cp build/docs/api/man/man3/ml666*.3 "$(DESTDIR)$(prefix)/share/man/man3/"

uninstall-docs:
	rm -f "$(DESTDIR)$(prefix)/share/man/man3/"ml666*.3

shell:
	if [ -z "$$SHELL" ]; then SHELL="$$(getent passwd $$(id -u) | cut -d : -f 7)"; fi; \
	if [ -z "$$SHELL" ]; then SHELL="/bin/sh"; fi; \
	PROMPT_COMMAND='if [ -z "$$PS_SET" ]; then PS_SET=1; PS1="(ml666) $$PS1"; fi' \
	LD_LIBRARY_PATH="$$PWD/lib/$(TYPE)/" \
	PATH="$$PWD/bin/$(TYPE)/:scripts/:$$PATH" \
	MANPATH="$$PWD/build/docs/api/man/:$$(man -w)" \
	  "$$SHELL"

test: build/test/example

docs: build/docs/api/.done

build/docs/api/.done: $(HEADERS) Doxyfile
	rm -rf build/docs/api/
	-doxygen -q
	touch "$@"

clean-docs:
	rm -rf build/docs/api/
