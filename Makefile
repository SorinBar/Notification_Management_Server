CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

TARGET_1 = server
TARGET_2 = subscriber

SRCS_1 = server.cpp
SRCS_2 = subscriber.cpp

$(TARGET_1): $(SRCS_1)
	$(CXX) $(CXXFLAGS) $(SRCS_1) -o $(TARGET_1)

$(TARGET_2): $(SRCS_2)
	$(CXX) $(CXXFLAGS) $(SRCS_2) -o $(TARGET_2)

.PHONY: clean run
clean:
	rm -f $(TARGET_1) $(TARGET_2)

run: server 
	valgrind ./server 9090