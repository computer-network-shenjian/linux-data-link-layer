CXXFLAGS := -std=c++11 -Wall -Wextra

SRC_SENDER := 01/sender1.cpp 02/sender2.cpp #03/sender3.cpp #04/sender4.cpp #05/sender5.cpp #06/sender6.cpp
SRC_RECEIVER := 01/receiver1.cpp 02/receiver2.cpp #03/receiver3.cpp #04/receiver4.cpp #05/receiver5.cpp #06/receiver6.cpp
SRC_SENDER_TEST := test/sender_physical_test.cpp
SRC_RECEIVER_TEST := test/receiver_physical_test.cpp

TARGET_SENDER := $(SRC_SENDER:.cpp=)
TARGET_RECEIVER:= $(SRC_RECEIVER:.cpp=)

TARGET_SENDER_TEST := $(SRC_SENDER_TEST:.cpp=)
TARGET_RECEIVER_TEST := $(SRC_RECEIVER_TEST:.cpp=)

SRC_LIB := common/shared_library.cpp common/Log.cpp
HEADER_LIB := common/shared_conf.hpp common/status.hpp common/shared_library.hpp common/Log.h
OBJ_LIB := $(SRC_LIB:.cpp=.o)

RM := rm -f

all: $(OBJ_LIB) $(HEADER_LIB) $(TARGET_SENDER) $(TARGET_RECEIVER)

common/Log.o: common/Log.cpp common/Log.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.cpp $(HEADER_LIB)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%: %.cpp $(OBJ_LIB) $(HEADER_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TARGET_SENDER_TEST) $(TARGET_RECEIVER_TEST)

clean :
	$(RM) $(TARGET_SENDER) $(TARGET_RECEIVER) $(TARGET_SENDER_TEST) $(TARGET_RECEIVER_TEST)
	$(RM) common/*.o
	$(RM) 01/*.log 02/*.log 03/*.log 04/*.log 05/*.log 06/*.log test/*.log ./*.log

.PHONY : all test clean
