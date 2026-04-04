MAKE_FLAGS += -j

SRC_SERVER := $(wildcard src/Server/*.cpp)
SRC_SERVER += $(wildcard src/Server/Commands/*.cpp)

SRC_CLIENT := $(wildcard src/Client/*.cpp)

BUILD_DIR := .build

CXXFLAGS += -Wall -Wextra -Werror=write-strings -g
CXXFLAGS += -Wno-unused-parameter -Wunused-result
CXXFLAGS += -Wp,-U_FORTIFY_SOURCE -Wcast-qual
CXXFLAGS += -Wformat=2 -Wshadow -fno-builtin -Wno-unused-command-line-argument 
CXXFLAGS += -Wstrict-aliasing=0 -Wunreachable-code
CXXFLAGS += -Wwrite-strings -Werror=format-nonliteral -Werror=return-type
CXXFLAGS += -std=c++20 -iquote src -iquote src/Server -iquote src/Client

LDLIBS := -L libs/

ifeq ($(shell uname -s),Darwin)
LDLIBS += -lmyteams_macos
CXXFLAGS += -D DARWIN_KERNEL
else
LDLIBS += -lmyteams -luuid
endif

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
	@ $$(COMPILE.cpp) $$(CXXFLAGS) $(strip $3) $$< -o $$@
	@ $$(LOG_TIME) "$$(C_GREEN) CC $$(C_PURPLE) $$(notdir $$@) $$(C_RESET)"

$$(NAME_$(strip $1)): $$(OBJ_$(strip $1))
	@ $$(LINK.cpp) $$(OBJ_$(strip $1)) $$(LDFLAGS) $$(LDLIBS) $(strip $3) -o $$@
	@ $$(LOG_TIME) "$$(C_BLUE) LD $$(C_PURPLE) $$(notdir $$@) $$(C_RESET)"
	@ $$(LOG_TIME) "$$(C_GREEN) OK  Compilation finished $$(C_RESET)"

endef

$(eval $(call mk-profile, server, SRC_SERVER, , myteams_server))
$(eval $(call mk-profile, client, SRC_CLIENT, , myteams_cli))

server: $(NAME_server)

client: $(NAME_client)

all: $(NAME_server) $(NAME_client)

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
	@ $(RM) -r $(NAME_server) $(NAME_client) $(BUILD_DIR)
	@ $(LOG_TIME) "$(C_YELLOW) RM $(C_PURPLE) $(NAME_server) $(NAME_client) $(BUILD_DIR) \
		$(C_RESET)"

launch_server: all
	@ export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:./libs"; ./$(NAME_server) 5290

.NOTPARALLEL: re
re:	fclean all

.PHONY: all clean fclean re

PREFIX ?=
BINDIR ?= $(PREFIX)/bin
