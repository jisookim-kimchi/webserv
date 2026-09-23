NAME        = webserv

CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++17 -Iincludes

SRCS_DIR    = srcs
OBJS_DIR    = objs

SRCS        = $(SRCS_DIR)/main.cpp \
              $(SRCS_DIR)/Server.cpp \
              $(SRCS_DIR)/Client.cpp \
              $(SRCS_DIR)/ListenSocket.cpp \
              $(SRCS_DIR)/ConfigParser.cpp \
              $(SRCS_DIR)/ServerConfig.cpp \
              $(SRCS_DIR)/LocationConfig.cpp \
              $(SRCS_DIR)/HTTP_request.cpp \
              $(SRCS_DIR)/HttpResponse.cpp \
              $(SRCS_DIR)/CgiHandler.cpp \
              $(SRCS_DIR)/Utils.cpp

OBJS        = $(SRCS:$(SRCS_DIR)/%.cpp=$(OBJS_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.cpp | $(OBJS_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJS_DIR):
	mkdir -p $(OBJS_DIR)

clean:
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
