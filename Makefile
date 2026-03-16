## Binary name
NAME		:= ircserv

## Compiler config
DEPS		:= dependencies.d
CC			:= c++
CFLAGS		:= -Wall -Wextra -Werror -std=c++20 -MMD -MP $(CF) -I./include

## Sources
DEPS_DIR	:= deps
OBJ_DIR 	:= obj
SRC_DIR		:= src
VPATH		:= $(SRC_DIR)
SRC			:= main.cpp
OBJS 		:= $(SRC:%.cpp=$(OBJ_DIR)/%.o)

#╔════════════════════════════════════════════╗
#║           🛠️  Build Protocols             ║
#╚════════════════════════════════════════════╝

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)
	rm -f $(DEPS)

re: fclean all

.PHONY: all clean fclean re

-include $(OBJS:.o=.d)
