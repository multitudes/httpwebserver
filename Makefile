# The @ symbol at the beginning of the line suppresses the echoing of the command to the terminal when it is executed.
# Without it, the command and its output would be printed to the terminal, which can make the output cleaner.
# -o $@:
# This specifies the output file to be produced. $@ is a special variable in Makefiles, representing
# the target of the rule. In this context, it specifies the output file name.
# -c $<:
# The -c flag indicates that the compiler should only produce an object file without linking.
# $< is another special variable, representing the first prerequisite (dependency) of the rule. It is the source file.

NAME 			= 	webserv
CXX				= 	c++

# CXXFLAGS 		= -Wall -Wextra -Werror 
CXXFLAGS		+= -std=c++98
CXXFLAGS 		+= -Wconversion -Wunreachable-code 

CFLAGS 			+= -Iinclude
CFLAGS			+= -Isrc 

CFLAGS 			+=  -g3 
# CFLAGS 			+=  -DNDEBUG

# directories
OBJ_DIR			= 	obj/
SRC_DIR			= 	src/
INCLUDE_DIR		= 	include/


INCLUDES		=  	-I$(INCLUDE_DIR) 

SRCS 			= $(addprefix $(SRC_DIR), main.cpp)
SRCS 			+= $(addprefix $(SRC_DIR), SocketUtils.cpp)
SRCS 			+= $(addprefix $(SRC_DIR), HTTPConnection.cpp)

OBJS 			= $(patsubst $(SRC_DIR)%.cpp,$(OBJ_DIR)%.o,$(SRCS))
HDRS 			= $(addprefix $(INCLUDE_DIR), debug.h )
HDRS 			+= $(addprefix $(SRC_DIR), )

all: $(NAME)

$(NAME): $(OBJS) $(HDRS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJS) -o $(NAME)

# Static pattern rule for compilation - adding the .o files in the obj folder
$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS)
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

# The idea is for this project to use run for production with extra flags to speed it up
# and optimize the binary size
ARGS = config
run: all
	@echo
	@PATH=".$${PATH:+:$${PATH}}" && ./$(NAME) $(ARGS)

valrun: all
	@echo
	@PATH=".$${PATH:+:$${PATH}}" && valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./$(NAME) $(ARGS)

test: $(NAME)
	python3 -m venv venv && \
    . venv/bin/activate && pip install -r tests/requirements.txt && \
    . venv/bin/activate && pytest tests

.PHONY: all clean fclean re run valrun test
