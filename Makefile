NAME = webserv

CXX = c++

CXXFLAGS = -Wall -Wextra -std=c++17 -g

OBJS_DIR = objs
SRCS = main.cpp
OBJS = $(OBJS_DIR)/$(SRCS:.cpp=.o)

all: $(NAME)

$(NAME) : $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJS) : $(SRCS)
	mkdir -p $(OBJS_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRCS) -o $@

clean: 
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -f $(NAME)
	
re: clean all

.PHONY: all clean fclean re
