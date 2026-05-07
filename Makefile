.DEFAULT_GOAL = debug
CC = gcc
CFLAGS=-ggdb -std=c99 -Wall -Wno-strict-prototypes -Wextra -pedantic -fsanitize=address
SRCS=ab.c command.c editor.c fileio.c find.c jim.c jimio.c row.c terminal.c ur.c window.c
DEBUG_DIR = build/debug
RELEASE_DIR = build/release

DEBUG_OBJS = $(patsubst %.c, $(DEBUG_DIR)/%.o, $(SRCS))
RELEASE_OBJS = $(patsubst %.c, $(RELEASE_DIR)/%.o, $(SRCS))

-include $(DEBUG_OBJS:.o=.d)
-include $(RELEASE_OBJS:.o=.d)

debug: CFLAGS = -ggdb -std=c99 -Wall -Wno-strict-prototypes -Wextra -pedantic -fsanitize=address
debug: $(DEBUG_DIR)/jim
	cp $^ jim

release: CFLAGS = -O2 -std=c99 -Wall -Wno-strict-prototypes -Wextra -pedantic
release: setup_env $(RELEASE_DIR)/jim
	cp $(RELEASE_DIR)/jim ~/bin/.jim
	@echo Done!

drelease: CFLAGS=-ggdb -std=c99 -Wall -Wno-strict-prototypes -Wextra -pedantic -fsanitize=address
drelease: setup_env $(DEBUG_DIR)/jim
	cp $(DEBUG_DIR)/jim ~/bin/.jim
	@echo Done!

$(DEBUG_DIR)/jim: $(DEBUG_OBJS)
	gcc $(CFLAGS) -o $@ $^

$(RELEASE_DIR)/jim: $(RELEASE_OBJS)
	gcc $(CFLAGS) -o $@ $^

$(DEBUG_DIR)/%.o: %.c | $(DEBUG_DIR)
	gcc $(CFLAGS) -MMD -MP -c -o $@ $<

$(RELEASE_DIR)/%.o: %.c | $(RELEASE_DIR)
	gcc $(CFLAGS) -MMD -MP -c -o $@ $<

setup_env:
	@echo Installing jim...
	@echo Setting up environment...
	mkdir -p ~/bin
	mkdir -p ~/.jim
	cp jim_syn/jim_*.syn ~/.jim/.
	@echo Compiling...

$(DEBUG_DIR):
	mkdir -p $(DEBUG_DIR)

$(RELEASE_DIR):
	mkdir -p $(RELEASE_DIR)

clean:
	rm -rf jim build
