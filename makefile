CXX = g++
CXXFLAGS = -std=c++20 -Ofast

SRC = src/main.cpp
TARGET = bounce

all: $(TARGET)

$(TARGET): $(SRC)
	@$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

debug:
	@$(CXX) $(CXXFLAGS) -ggdb3 $(SRC) -o $(TARGET)

clean:
	@rm -f $(TARGET)
