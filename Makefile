CXXFLAGS := -std=c++11 -Wall -Wextra

SRC_SENDER := 01/sender1_network.cpp #02/sender1_network.cpp 03/sender1_network.cpp 01/sender1_network.cpp 05/sender1_network.cpp 06/sender1_network.cpp
SRC_RECEIVER := 01/receiver1_network.cpp #02/receiver1_network.cpp 03/receiver1_network.cpp 04/receiver1_network.cpp 05/receiver1_network.cpp 06/receiver1_network.cpp

TARGET_SENDER := $(SRC_SENDER:.cpp=)
TARGET_RECEIVER:= $(SRC_RECEIVER:.cpp=)

SRC_LIB := common/shared_library.cpp common/Log.cpp
OBJ_LIB := $(SRC_LIB:.cpp=.o)

RM := rm -rf


all: $(TARGET_SENDER) $(TARGET_RECEIVER)

$(TARGET_SENDER): $(SRC_SENDER) $(OBJ_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TARGET_RECEIVER): $(SRC_RECEIVER) $(OBJ_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean :
	$(RM) $(TARGET_SENDER) $(TARGET_RECEIVER) *.o

.PHONY : clean all
