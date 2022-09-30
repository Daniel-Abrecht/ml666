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
CFLAGS  += -fsanitize=address
LDFLAGS += -fsanitize=address
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

.PHONY: all clean get-bin get-lib install uninstall shell test

all: bin/$(TYPE)/ml666-tokenizer-example \
     bin/$(TYPE)/ml666 \
     lib/$(TYPE)/libml666.a \
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

clean:
	rm -rf build/$(TYPE)/ bin/$(TYPE)/ lib/$(TYPE)/

install:
	cp "lib/$(TYPE)/lib$(SONAME).so" "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)"
	cp "bin/$(TYPE)/ml666" "$(DESTDIR)$(prefix)/bin/ml666"
	ln -sf "lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)" "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR).$(MINOR)"
	ln -sf "lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)" "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR)"
	ln -sf "lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)" "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so"
	ldconfig

uninstall:
	rm -f "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR).$(MINOR).$(PATCH)"
	rm -f "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR).$(MINOR)"
	rm -f "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so.$(MAJOR)"
	rm -f "$(DESTDIR)$(prefix)/lib/lib$(SONAME).so"

shell:
	PROMPT_COMMAND='if [ -z "$$PS_SET" ]; then PS_SET=1; PS1="(ml666) $$PS1"; fi' \
	LD_LIBRARY_PATH="$$PWD/lib/$(TYPE)/" \
	PATH="$$PWD/bin/$(TYPE)/:$$PATH" \
	  "$$SHELL"

test: build/test/example
