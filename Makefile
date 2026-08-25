NAME     := webserv

CXX      := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98

# Auto-discover: every .cpp under src/, plus main.cpp. Add files freely —
# you never need to edit this Makefile.
SRCS     := main.cpp $(shell find src -name '*.cpp' 2>/dev/null)

# -I every source directory so headers can be included by bare filename
# (e.g. #include "Connection.hpp") regardless of which folder they live in.
INCLUDES := -Isrc $(addprefix -I,$(shell find src -type d 2>/dev/null))

# Object files mirror the source tree under build/ (keeps src/ clean).
OBJDIR   := build
OBJS     := $(SRCS:%.cpp=$(OBJDIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
