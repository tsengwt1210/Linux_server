CXX ?= g++
CPPFLAGS ?= -D_FILE_OFFSET_BITS=64
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
PYTHON ?= python3

BIN := build/bin
SRC := ConsoleApplication
PROGRAMS := $(BIN)/tcp_server $(BIN)/tcp_client $(BIN)/http_server $(BIN)/cgi $(BIN)/multicast_chat

.PHONY: all test clean
all: $(PROGRAMS)

$(BIN):
	mkdir -p $@

$(BIN)/tcp_server: $(SRC)/ConsoleApplication/try.cpp $(SRC)/io_helpers.h | $(BIN)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(BIN)/tcp_client: $(SRC)/ClientApplication/client.cpp $(SRC)/io_helpers.h | $(BIN)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(BIN)/http_server: $(SRC)/Assignment2_WWW/WWW_server.cpp $(SRC)/io_helpers.h | $(BIN)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(BIN)/cgi: $(SRC)/Assignment2_WWW/cgi.cpp | $(BIN)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(BIN)/multicast_chat: $(SRC)/multicast_chat/multicast_chat.cpp | $(BIN)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

test: all
	$(PYTHON) -m unittest discover -s tests -v

clean:
	rm -rf build
