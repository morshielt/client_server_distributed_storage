CXX = g++
FLAGS = -std=c++17 -Wall -Werror -Wno-error=unused-variable -Wno-error=unused-but-set-variable
BOOST = -lboost_program_options -lboost_system -lboost_filesystem

all: netstore-server netstore-client

netstore-server: src/server.cpp
	$(CXX) $(FLAGS) $< $(BOOST) -o $@

netstore-client: src/client.cpp
	$(CXX) $(FLAGS) $< $(BOOST) -o $@

.PHONY: clean

clean:
	rm -f netstore-server netstore-client *.o *.d *~ *.bak

