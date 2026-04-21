CXX      = g++
CXXFLAGS = -O2 -Wall -Wextra -Wpedantic -DNDEBUG
VERSION   = 1.2.1

ifeq ($(OS),Windows_NT)
	EXT_EXECUTABLE = .exe
	LDFLAGS = -s -static -static-libgcc -static-libstdc++ -lole32 -luuid
else
	LDFLAGS  += -Wl,-z,relro -Wl,-z,now
	CXXFLAGS += -g
	EXT_EXECUTABLE =
endif

TARGET   = vlcrcedit$(EXT_EXECUTABLE)
SRC      = vlcrcedit.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

installer:
	cd .. && \
	mkdir -p releases/windows/$(VERSION) && \
	cp build/libdiscordrpc_plugin.dll releases/windows/$(VERSION)/libdiscordrpc_plugin.dll && \
	makensis //DVERSION=$(VERSION) installer.nsi

clean:
	rm -f $(TARGET)
