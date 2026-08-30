CXX = g++
CXXFLAGS = -std=c++20 -Ofast

SRC = src/main.cpp
TARGET = bounce

all: $(TARGET)

$(TARGET): $(SRC)
	@$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

debug:
	@$(CXX) -Wpedantic $(CXXFLAGS) -ggdb3 $(SRC) -o $(TARGET)

format:
	@git diff -U0 | clang-format-diff -p1 -i -style "microsoft"

clean:
	@rm -f $(TARGET)
