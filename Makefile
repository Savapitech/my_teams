MAKE_FLAGS += -j

BIN_NAME := arcade

SRC := $(wildcard src/*.cpp)
SRC += $(wildcard src/*/*.cpp)

SRC_SFML := $(wildcard src/Graphic/SFML/*.cpp)
SRC_NCURSES := $(wildcard src/Graphic/Ncurses/*.cpp)
SRC_SDL2 := $(wildcard src/Graphic/SDL2/*.cpp)

SRC_PACMAN := $(wildcard src/Game/Pacman/*.cpp)
SRC_PACMAN += src/Game/Entity.cpp

BUILD_DIR := .build

CXXFLAGS += -Wall -Wextra -Werror=write-strings -g
CXXFLAGS += -Wno-unused-parameter -Wunused-result
CXXFLAGS += -Wp,-U_FORTIFY_SOURCE -Wcast-qual
CXXFLAGS += -Wformat=2 -Wshadow -fno-builtin -Wno-unused-command-line-argument 
CXXFLAGS += -Wstrict-aliasing=0 -Wunreachable-code
CXXFLAGS += -Wwrite-strings -Werror=format-nonliteral -Werror=return-type
CXXFLAGS += -std=c++20 -iquote src

SFML_LDFLAGS += $(shell pkg-config --cflags --libs sfml-graphics sfml-window sfml-system)
NCURSES_LDFLAGS += $(shell pkg-config --cflags --libs ncurses)
SDL2_LDFLAGS += $(shell pkg-config --cflags --libs sdl2 SDL2_image)

include utils.mk

.PHONY: _start all
_start: all

CXX = clang++

# call mk-profile release, SRC, additional CFLAGS
define mk-profile

NAME_$(strip $1) := $4
OBJ_$(strip $1) := $$($(strip $2):%.cpp=$$(BUILD_DIR)/$(strip $1)/%.o)

$$(BUILD_DIR)/$(strip $1)/%.o: %.cpp
	@ mkdir -p $$(dir $$@)
	@ mkdir -p lib
	@ $$(COMPILE.cpp) $$(CXXFLAGS) $(strip $3) $$< -o $$@
	@ $$(LOG_TIME) "$$(C_GREEN) CC $$(C_PURPLE) $$(notdir $$@) $$(C_RESET)"

$$(NAME_$(strip $1)): $$(OBJ_$(strip $1))
	@ $$(LINK.cpp) $$(OBJ_$(strip $1)) $$(LDFLAGS) $$(LDLIBS) $(strip $3) -o $$@
	@ $$(LOG_TIME) "$$(C_BLUE) LD $$(C_PURPLE) $$(notdir $$@) $$(C_RESET)"
	@ $$(LOG_TIME) "$$(C_GREEN) OK  Compilation finished $$(C_RESET)"

endef

$(eval $(call mk-profile, core, SRC, , $(BIN_NAME)))
$(eval $(call mk-profile, sfml, SRC_SFML, $(SFML_LDFLAGS) -shared -fPIC, lib/arcade_sfml.so))
$(eval $(call mk-profile, ncurses, SRC_NCURSES, $(NCURSES_LDFLAGS) -shared -fPIC, lib/arcade_ncurses.so))
$(eval $(call mk-profile, sdl2, SRC_SDL2, $(SDL2_LDFLAGS) -shared -fPIC, lib/arcade_sdl2.so))
$(eval $(call mk-profile, pacman, SRC_PACMAN, -shared -fPIC, lib/arcade_pacman.so))

core: $(NAME_core)

games: $(NAME_pacman)

graphicals: $(NAME_sfml) $(NAME_ncurses) $(NAME_sdl2)

all: core graphicals games

debug: CXXFLAGS += -D DEBUG_MODE
debug: all

tests: $(NAME_test)
	@ bash tests/run_all.sh

format:
	@ find ./ -name "*.cpp" -type f -exec clang-format -i {} ";"
	@ find ./ -name "*.hpp" -type f -exec clang-format -i {} ";"
	@ $(LOG_TIME) "$(C_BLUE) CF $(C_GREEN) Code formated  $(C_RESET)"

check_format:
	@ find ./ -name "*.cpp" -type f -exec clang-format --dry-run --Werror {} ";" 2>&1 | wc -m | grep 0 > /dev/null
	@ find ./ -name "*.hpp" -type f -exec clang-format --dry-run --Werror {} ";" 2>&1 | wc -m | grep 0 > /dev/null
	@ $(LOG_TIME) "$(C_BLUE) CF $(C_GREEN) Code formated  $(C_RESET)"

clean:
	@ $(RM) $(OBJ)
	@ $(LOG_TIME) "$(C_YELLOW) RM $(C_PURPLE) $(OBJ) $(C_RESET)"

fclean:
	@ $(RM) -r $(NAME_release) $(NAME_debug) $(BUILD_DIR) ./lib/
	@ $(LOG_TIME) "$(C_YELLOW) RM $(C_PURPLE) $(NAME_release) $(NAME_debug) \
		$(C_RESET)"

.NOTPARALLEL: re
re:	fclean all

.PHONY: all clean fclean re

PREFIX ?=
BINDIR ?= $(PREFIX)/bin
