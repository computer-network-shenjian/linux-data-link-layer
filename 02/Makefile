CXXFLAGS := -std=c++11 -Wall -Wextra

SRC_SENDER := sender2.cpp
SRC_RECEIVER := receiver2.cpp

TARGET_SENDER := $(SRC_SENDER:.cpp=)
TARGET_RECEIVER:= $(SRC_RECEIVER:.cpp=)

SRC_LIB := ../common/shared_library.cpp ../common/Log.cpp
HEADER_LIB := ../common/shared_conf.hpp ../common/status.hpp ../common/shared_library.hpp ../common/Log.h
OBJ_LIB := $(SRC_LIB:.cpp=.o)

RM := rm -f

all: $(OBJ_LIB) $(HEADER_LIB) $(TARGET_SENDER) $(TARGET_RECEIVER)

../common/Log.o: ../common/Log.cpp ../common/Log.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.cpp $(HEADER_LIB)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%: %.cpp $(OBJ_LIB) $(HEADER_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean :
	$(RM) $(TARGET_SENDER) $(TARGET_RECEIVER)
	$(RM) ../common/*.o
	$(RM) *.log

.PHONY : all clean
