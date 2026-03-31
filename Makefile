## Binary name
NAME		:= ircserv

## Compiler config
DEPS		:= dependencies.d
CC			:= c++
INCLUDES	:= "-Isrc"
CFLAGS		:= -Wall -Wextra -Werror -std=c++20 -MMD -MP $(CF) $(INCLUDES)

## Sources
DEPS_DIR	:= deps
OBJ_DIR 	:= obj
SRC_DIR		:= src
VPATH		:= $(SRC_DIR) $(SRC_DIR)/server $(SRC_DIR)/utils $(SRC_DIR)/parser
SRC			:= main.cpp Logger.cpp IrcServer.cpp IOEventPoller.cpp RawCommandParser.cpp
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
