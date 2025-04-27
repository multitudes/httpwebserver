# The @ symbol at the beginning of the line suppresses the echoing of the command to the terminal when it is executed.
# Without it, the command and its output would be printed to the terminal, which can make the output cleaner.
# -o $@:
# This specifies the output file to be produced. $@ is a special variable in Makefiles, representing
# the target of the rule. In this context, it specifies the output file name.
# -c $<:
# The -c flag indicates that the compiler should only produce an object file without linking.
# $< is another special variable, representing the first prerequisite (dependency) of the rule. It is the source file.

# Detect OS (Linux vs Mac)
UNAME_S := $(shell uname -s)

NAME 			= 	webserv
CXX				= 	c++

# CXXFLAGS 		= -Wall -Wextra -Werror 
#  CXXFLAGS        += -pedantic-errors
CXXFLAGS		+= -std=c++98 
CXXFLAGS 		+= -Wconversion -Wunreachable-code 
ifeq ($(CXX), clang++)
    CXXFLAGS += -fno-limit-debug-info
endif

CXXFLAGS		+= -g
CXXFLAGS 		+= -O0

# CXXFLAGS 			+=  -g3 
# CXXFLAGS 			+=  -DNDEBUG

# directories
OBJ_DIR			= 	obj/
SRC_DIR			= 	src/
INCLUDE_DIR		= 	include/


CPPFLAGS        += -I$(INCLUDE_DIR)
CPPFLAGS        += -I$(SRC_DIR) # Add src include path here


SRCS 			= $(addprefix $(SRC_DIR), main.cpp)
SRCS 			+= $(addprefix $(SRC_DIR), SocketUtils.cpp)
SRCS 			+= $(addprefix $(SRC_DIR), HTTPConnxData.cpp)
SRCS 			+= $(addprefix $(SRC_DIR), Config.cpp)
SRCS 			+= $(addprefix $(SRC_DIR), CGI.cpp)
SRCS 			+= $(addprefix $(SRC_DIR), HTTPServer.cpp)
SRCS 			+= $(addprefix $(SRC_DIR), Constants.cpp)
SRCS 			+= $(addprefix $(SRC_DIR), URLMatcher.cpp)
SRCS 			+= $(addprefix $(SRC_DIR), Responses.cpp)
SRCS 			+= $(addprefix $(SRC_DIR), Parser.cpp)
SRCS 			+= $(addprefix $(SRC_DIR), Utils.cpp)
SRCS 			+= $(addprefix $(SRC_DIR), ParserUtils.cpp)

OBJS 			= $(patsubst $(SRC_DIR)%.cpp,$(OBJ_DIR)%.o,$(SRCS))
HDRS 			= $(addprefix $(INCLUDE_DIR), debug.h )
HDRS 			+= $(addprefix $(SRC_DIR), )

all: $(NAME) test

# # Add PIE flags only for Linux
# ifeq ($(shell uname -s), Linux)
# 	CXXFLAGS	+= -fPIE
# 	LDFLAGS 	+= -pie
# else ifeq ($(shell uname -s), Darwin)
# 	@echo "Building on macOS (Darwin)"
# 	# No additional flags needed for macOS
# endif

$(NAME): $(OBJS) $(HDRS)
	$(CXX) $(CXXFLAGS)  $(CPPFLAGS) $(OBJS) $(LDFLAGS) -o $(NAME) 

# Static pattern rule for compilation - adding the .o files in the obj folder
$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
	rm -rf $(OBJ_DIR)

# Clean everything (including venv)
fclean: clean
	@rm -f $(NAME)
	@rm -rf $(VENV_DIR)
	@echo "Cleaned project and virtual environment"

re: clean all

# The idea is for this project to use run for production with extra flags to speed it up
# and optimize the binary size
ARGS = config/default.conf
run: all
	@echo
	@PATH=".$${PATH:+:$${PATH}}" && ./$(NAME) $(ARGS)

valrun: all
	@echo
	@PATH=".$${PATH:+:$${PATH}}" && valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./$(NAME) $(ARGS)


# Check if venv exists, create if not
# Makefile for webserver with automatic venv setup
# Makefiles always run commands in a subshell so activate is not possible
VENV_DIR = venv
PYTHON = python3
PIP = $(VENV_DIR)/bin/pip
PYTEST = $(VENV_DIR)/bin/pytest

# Virtual environment setup
venv:
	@echo "Setting up virtual environment..."
	@if [ ! -d "$(VENV_DIR)" ]; then \
		$(PYTHON) -m venv $(VENV_DIR) || $(PYTHON) -m virtualenv $(VENV_DIR); \
		$(PIP) install --upgrade pip; \
		$(PIP) install pytest; \
		if [ -f "tests/requirements.txt" ]; then $(PIP) install -r tests/requirements.txt; fi; \
		echo "Virtual environment ready"; \
	else \
		echo "Virtual environment already exists"; \
	fi

# Run tests in the venv
test: $(NAME) venv
	@echo "Running tests..."
	@$(PYTEST) tests/

.PHONY: all venv test clean fclean re run valrun