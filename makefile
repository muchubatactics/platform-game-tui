CXX = g++
CXXFLAGS = -std=c++20

SRC = src/main.cpp
TARGET = bounce

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
