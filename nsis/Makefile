CXX      = g++
CXXFLAGS = -O2 -Wall -Wextra -Wpedantic -DNDEBUG
LDFLAGS  = -s -static -static-libgcc -static-libstdc++

ifeq ($(OS),Windows_NT)
	EXT_EXECUTABLE = .exe
	RM = del /Q
else
	EXT_EXECUTABLE =
	RM = rm -f
endif

TARGET   = vlcrcedit$(EXT_EXECUTABLE)
SRC      = vlcrcedit.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	$(RM) $(TARGET)
