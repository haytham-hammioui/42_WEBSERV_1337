NAME := webserv

CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98
INCLUDES := -I./http_layer -I./src -I./core

SRCS := \
	$(filter-out http_layer/main.cpp, $(wildcard http_layer/*.cpp)) \
	$(wildcard src/*.cpp) \
	$(wildcard core/*.cpp)

OBJS := $(SRCS:.cpp=.o)

GREEN  := \033[0;32m
BLUE   := \033[0;34m
YELLOW := \033[1;33m
RED    := \033[0;31m
CYAN   := \033[0;36m
RESET  := \033[0m

all: banner $(NAME)

banner:
	@echo "$(CYAN)"
	@echo "██╗    ██╗███████╗██████╗ ███████╗███████╗██████╗ ██╗   ██╗"
	@echo "██║    ██║██╔════╝██╔══██╗██╔════╝██╔════╝██╔══██╗██║   ██║"
	@echo "██║ █╗ ██║█████╗  ██████╔╝███████╗█████╗  ██████╔╝██║   ██║"
	@echo "██║███╗██║██╔══╝  ██╔══██╗╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝"
	@echo "╚███╔███╔╝███████╗██████╔╝███████║███████╗██║  ██║ ╚████╔╝ "
	@echo " ╚══╝╚══╝ ╚══════╝╚═════╝ ╚══════╝╚══════╝╚═╝  ╚═╝  ╚═══╝  "
	@echo "$(RESET)"
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(RESET)"
	@echo "$(YELLOW)Building WEBSERV...$(RESET)"
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(RESET)"

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo ""
	@echo "$(GREEN)✔ Compilation Finished [100%]$(RESET)"
	@echo "$(GREEN)✔ Executable created -> ./$(NAME)$(RESET)"

%.o: %.cpp
	@printf "$(CYAN)[100%%]$(RESET) %-45s\n" "$<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@rm -f $(OBJS)
	@echo "$(YELLOW)🧹 Object files cleaned.$(RESET)"

fclean: clean
	@rm -f $(NAME)
	@echo "$(RED)🗑 Executable removed.$(RESET)"

re: fclean all

.PHONY: all clean fclean re banner