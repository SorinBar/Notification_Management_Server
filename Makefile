CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

TARGET_1 = server
TARGET_2 = subscriber

SRCS_1 = server.cpp utils.cpp fast_forward.cpp command.cpp clientsDatabase.cpp topicsDatabase.cpp
SRCS_2 = subscriber.cpp utils.cpp fast_forward.cpp command.cpp

$(TARGET_1): $(SRCS_1)
	$(CXX) $(CXXFLAGS) $(SRCS_1) -o $(TARGET_1)

$(TARGET_2): $(SRCS_2)
	$(CXX) $(CXXFLAGS) $(SRCS_2) -o $(TARGET_2)

.PHONY: clean
clean:
	rm -f $(TARGET_1) $(TARGET_2)