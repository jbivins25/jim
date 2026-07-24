.DEFAULT_GOAL := debug

CC := gcc

ifeq ($(OS),Windows_NT)
TARGET := jim.exe
TERM_SRC := terminal_windows.c

DEBUG_DIR := build/debug
RELEASE_DIR := build/release

TARGET_DIR := $(USERPROFILE)\progs
SETUP_DIR := $(APPDATA)\jim

else
TARGET := jim
TERM_SRC := terminal_unix.c

DEBUG_DIR := build/debug
RELEASE_DIR := build/release

TARGET_DIR := $(HOME)/bin
SETUP_DIR := $(HOME)/.jim
endif

CFLAGS := -ggdb -std=c99 -Wall -Wextra -Wpedantic -Wno-strict-prototypes

SANITIZE :=
ifneq ($(OS),Windows_NT)
SANITIZE := -fsanitize=address
endif

SRCS := \
	ab.c \
	command.c \
	editor.c \
	fileio.c \
	find.c \
	jim.c \
	jimio.c \
	row.c \
	$(TERM_SRC) \
	ur.c \
	window.c

DEBUG_OBJS := $(SRCS:%.c=$(DEBUG_DIR)/%.o)
RELEASE_OBJS := $(SRCS:%.c=$(RELEASE_DIR)/%.o)

-include $(DEBUG_OBJS:.o=.d)
-include $(RELEASE_OBJS:.o=.d)

###########################################################################
# Debug
###########################################################################

debug: CFLAGS += $(SANITIZE)
debug: $(TARGET)

###########################################################################
# Release
###########################################################################

release: CFLAGS := -O2 -std=c99 -Wall -Wextra -Wpedantic -Wno-strict-prototypes
release: setup_env $(RELEASE_DIR)/$(TARGET)

ifeq ($(OS),Windows_NT)
	copy /Y "$(subst /,\,$(RELEASE_DIR)/$(TARGET))" "$(TARGET_DIR)"
else
	cp -f $(RELEASE_DIR)/$(TARGET) "$(TARGET_DIR)"
endif

	@echo Done!

###########################################################################
# Debug Release
###########################################################################

drelease: CFLAGS += $(SANITIZE)
drelease: setup_env $(DEBUG_DIR)/$(TARGET)

ifeq ($(OS),Windows_NT)
	copy /Y "$(subst /,\,$(DEBUG_DIR)/$(TARGET))" "$(TARGET_DIR)"
else
	cp -f $(DEBUG_DIR)/$(TARGET) "$(TARGET_DIR)"
endif

	@echo Done!

###########################################################################
# Linking
###########################################################################

$(TARGET): $(DEBUG_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(DEBUG_DIR)/$(TARGET): $(DEBUG_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(RELEASE_DIR)/$(TARGET): $(RELEASE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

###########################################################################
# Compilation
###########################################################################

$(DEBUG_DIR)/%.o: %.c | $(DEBUG_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(RELEASE_DIR)/%.o: %.c | $(RELEASE_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

###########################################################################
# Directories
###########################################################################

$(DEBUG_DIR):
ifeq ($(OS),Windows_NT)
	if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"
else
	mkdir -p "$@"
endif

$(RELEASE_DIR):
ifeq ($(OS),Windows_NT)
	if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"
else
	mkdir -p "$@"
endif

###########################################################################
# Install
###########################################################################

setup_env:
	@echo Installing jim...
	@echo Setting up environment...

ifeq ($(OS),Windows_NT)
	if not exist "$(TARGET_DIR)" mkdir "$(TARGET_DIR)"
	if not exist "$(SETUP_DIR)" mkdir "$(SETUP_DIR)"
	copy /Y jim_syn\jim_*.syn "$(SETUP_DIR)"
else
	mkdir -p "$(TARGET_DIR)"
	mkdir -p "$(SETUP_DIR)"
	cp jim_syn/jim_*.syn "$(SETUP_DIR)"
endif

###########################################################################
# Clean
###########################################################################

clean:
ifeq ($(OS),Windows_NT)
	-if exist build rmdir /s /q build
	-if exist "$(TARGET)" del /q "$(TARGET)"
else
	rm -rf build
	rm -f $(TARGET)
endif