CXX      = g++
CXXFLAGS = -O2 -Wall -Wextra -Wpedantic -DNDEBUG
VERSION  = 1.2.1

WINDRES  = windres

ifeq ($(OS),Windows_NT)
	EXT_EXECUTABLE = .exe
	RES_OBJ = resources.o
	LDFLAGS = -s -static -static-libgcc -static-libstdc++ -lole32 -luuid
else
	LDFLAGS  += -Wl,-z,relro -Wl,-z,now
	CXXFLAGS += -g
	EXT_EXECUTABLE =
	RES_OBJ =
endif

TARGET   = vlcrcedit$(EXT_EXECUTABLE)
SRC      = vlcrcedit.cpp

all: $(TARGET)

$(RES_OBJ): ../src/resources.rc
	$(WINDRES) ../src/resources.rc $@

$(TARGET): $(SRC) $(RES_OBJ)
	$(CXX) $(CXXFLAGS) $(SRC) $(RES_OBJ) -o $(TARGET) $(LDFLAGS)

installer:
	cd .. && \
	mkdir -p releases/windows/$(VERSION) && \
	cp build/libdiscordrpc_plugin.dll releases/windows/$(VERSION)/libdiscordrpc_plugin.dll && \
	makensis //DVERSION=$(VERSION) installer.nsi

clean:
	rm -f $(TARGET) $(RES_OBJ)
