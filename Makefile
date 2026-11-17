Q2_CC ?= clang
Q2_LD ?= ld.lld
CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Werror -pedantic
Q2_CFLAGS ?= -Oz -Wall -Wextra -Werror
Q2_FLAGS = --target=mipsel-linux-gnu -march=mips32r2 -mabi=32 -mfp64 -mnan=2008 -mabs=2008 \
	-mno-abicalls -fno-pic -G0 -ffreestanding -fno-builtin -fno-stack-protector \
	-fno-unwind-tables -fno-asynchronous-unwind-tables
SOURCES = src/app.c src/render.c core/arcade_core.c core/breakout_core.c core/game_framework.c
OBJECTS = $(addprefix build/q2/,$(SOURCES:.c=.o) src/platform_q2.o)
CARD = dist/.q2-arcade

all: dist/q2-arcade.zip

build/q2/%.o: %.c src/app.h core/arcade_core.h core/breakout_core.h core/game_framework.h
	mkdir -p $(@D)
	$(Q2_CC) $(Q2_FLAGS) $(Q2_CFLAGS) -std=c11 -Isrc -Icore -c $< -o $@

$(CARD)/q2-arcade: $(OBJECTS)
	mkdir -p $(@D)
	$(Q2_LD) -m elf32ltsmip -static -e __start -s -z noexecstack -o $@ $^

$(CARD)/q2app: q2app
	mkdir -p $(@D)
	tr -d '\r' < $< > $@

$(CARD)/LICENSE.txt: LICENSE
	mkdir -p $(@D)
	cp $< $@

dist/q2-arcade.zip: $(CARD)/q2-arcade $(CARD)/q2app $(CARD)/LICENSE.txt
	cd dist && rm -f q2-arcade.zip && zip -q q2-arcade.zip $(patsubst dist/%,%,$^)

build/app-test: tests/app_test.c src/app.c $(wildcard core/*.c) src/app.h
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -std=c11 -Isrc -Icore -o $@ tests/app_test.c src/app.c core/arcade_core.c core/breakout_core.c

check: build/app-test
	./build/app-test
	sh tests/arcade_core.sh
	sh tests/breakout_core.sh
	sh tests/game_framework.sh

clean:
	rm -rf build dist

.PHONY: all check clean
