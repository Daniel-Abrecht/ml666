.SECONDARY:

HEADERS := $(shell find include -type f -name "*.h" -not -name ".*")
SOURCES := $(shell find src -type f -iname "*.c")

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

B-TS := bin/$(TYPE)/testsuite

BINS := $(patsubst src/main/%.c,bin/$(TYPE)/%,$(filter src/main/%.c,$(SOURCES)))

.PHONY: all bin lib docs clean get//bin get//lib install uninstall shell test clean//docs install//docs uninstall//docs

all: bin lib docs

bin: $(BINS)

lib: lib/$(TYPE)/lib$(SONAME).a \
     lib/$(TYPE)/lib$(SONAME).so

get//bin:
	@echo bin/$(TYPE)/

get//lib:
	@echo lib/$(TYPE)/

bin/$(TYPE)/%: build/$(TYPE)/o/src/main/%.c.o lib/$(TYPE)/lib$(SONAME).so
	mkdir -p $(dir $@)
	$(CC) -o $@ $(LDFLAGS) $< -Llib/$(TYPE)/ -l$(SONAME)

lib/$(TYPE)/lib$(SONAME).so: lib/$(TYPE)/lib$(SONAME).a
	ln -sf "lib$(SONAME).so" "$@.0"
	$(CC) -o $@ -Wl,--no-undefined -Wl,-soname,lib$(SONAME).so.$(MAJOR) --shared -fPIC $(LDFLAGS) -Wl,--whole-archive $^ -Wl,--no-whole-archive

lib/$(TYPE)/lib$(SONAME).a: $(filter-out build/$(TYPE)/o/src/main/%,$(OBJECTS))
	mkdir -p $(dir $@)
	rm -f $@
	$(AR) q $@ $^

build/$(TYPE)/o/%.c.o: %.c makefile $(HEADERS)
	mkdir -p $(dir $@)
	$(CC) -fPIC -c -o $@ $(DFLAGS) $(CFLAGS) $<

build/test/tokenizer/%: test/%.ml666 test/%.tokenizer-result bin/$(TYPE)/ml666-tokenizer-example
	mkdir -p $(dir $@)
	LD_LIBRARY_PATH="$$PWD/lib/$(TYPE)/" \
	bin/$(TYPE)/ml666-tokenizer-example <"$<" >"$@"

test//tokenizer/%: build/test/tokenizer/% test/%.tokenizer-result $(B-TS)
	$(B-TS) "$(@:test//tokenizer//%=%)" diff -q "$<" "$(word 2,$^)"

test//tokenizer: $(B-TS)
	$(B-TS) "tokenizer" $(MAKE) $(patsubst test/%.tokenizer-result,test//tokenizer//%,$(wildcard test/*.tokenizer-result))

build/test/json/%: test/%.ml666 test/%.json bin/$(TYPE)/ml666
	mkdir -p $(dir $@)
	# Normalizing json using jq. This way, changes to the output don't matter so long as it's still the same data
	LD_LIBRARY_PATH="$$PWD/lib/$(TYPE)/" \
	bin/$(TYPE)/ml666 <"$<" --output-format json | jq -c >"$@"

test//json//%: build/test/json/% test/%.json $(B-TS)
	$(B-TS) "$(@:test//json//%=%)" diff -q "$<" "$(word 2,$^)"

test//json: $(B-TS)
	$(B-TS) "JSON" $(MAKE) $(patsubst test/%.json,test//json//%,$(wildcard test/*.json test/**/*.json))

clean: clean//docs
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

install//docs:
	mkdir -p "$(DESTDIR)$(prefix)/share/man/man3/"
	rm -f "$(DESTDIR)$(prefix)/share/man/man3/"ml666*.3
	cp build/docs/api/man/man3/ml666*.3 "$(DESTDIR)$(prefix)/share/man/man3/"

uninstall//docs:
	rm -f "$(DESTDIR)$(prefix)/share/man/man3/"ml666*.3

shell:
	if [ -z "$$SHELL" ]; then SHELL="$$(getent passwd $$(id -u) | cut -d : -f 7)"; fi; \
	if [ -z "$$SHELL" ]; then SHELL="/bin/sh"; fi; \
	PROMPT_COMMAND='if [ -z "$$PS_SET" ]; then PS_SET=1; PS1="(ml666) $$PS1"; fi' \
	LD_LIBRARY_PATH="$$PWD/lib/$(TYPE)/" \
	PATH="$$PWD/bin/$(TYPE)/:scripts/:$$PATH" \
	MANPATH="$$PWD/build/docs/api/man/:$$(man -w)" \
	  "$$SHELL"

test: $(B-TS)
	$(B-TS) "ml666" $(MAKE) test//tokenizer test//json

docs: build/docs/api/.done

build/docs/api/.done: $(HEADERS) Doxyfile
	rm -rf build/docs/api/
	-doxygen -q
	-cp -r docs/. build/docs/api/html/
	-ln -sf . build/docs/api/html/docs
	-ln -sf . build/docs/api/html/test
	-cp test/example.ml666 build/docs/api/html/test/
	-touch "$@"

clean//docs:
	rm -rf build/docs/api/
