CXXFLAGS := -std=c++11 -Wall -Wextra

SEND_TEST := sender_physical_test.cpp
RECV_TEST := receiver_physical_test.cpp
SEND_TARGET := $(SEND_TEST:.cpp=)
RECV_TARGET := $(RECV_TEST:.cpp=)

SRC_LIB := ../common/shared_library.cpp
HEADER_LIB := $(SRC_LIB:.cpp=.hpp) ../common/shared_conf.hpp ../common/status.hpp
OBJ_LIB := $(SRC_LIB:.cpp=.o)

RM := rm -f

# all: $(SEND_TARGET) $(RECV_TARGET)
all : s r

$(SEND_TARGET): $(SEND_TEST) $(SRC_LIB) $(HEADER_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(RECV_TARGET): $(RECV_TEST) $(SRC_LIB) $(HEADER_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $^

s: $(SEND_TEST) $(SRC_LIB) $(HEADER_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $^

r: $(RECV_TEST) $(SRC_LIB) $(HEADER_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean :
	$(RM) $(OBJ_LIB) $(SEND_TARGET) $(RECV_TARGET) s r

.PHONY : clean all