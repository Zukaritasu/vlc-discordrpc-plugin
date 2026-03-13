CXX      = g++
CXXFLAGS = -Os -Wall -Wextra -Wpedantic -DNDEBUG
LDFLAGS  = -s -static -static-libgcc -static-libstdc++

TARGET   = vlcrcedit.exe
SRC      = vlcrcedit.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	@if exist $(TARGET) del $(TARGET)
