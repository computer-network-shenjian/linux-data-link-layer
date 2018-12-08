CXXFLAGS := -std=c++11 -Wall -Wextra

SRC_LIB := shared_library.cpp Log.cpp
HEADER_LIB := shared_conf.hpp status.hpp shared_library.hpp Log.h
OBJ_LIB := $(SRC_LIB:.cpp=.o)

RM := rm -f

all: $(OBJ_LIB) $(HEADER_LIB)

Log.o: Log.cpp Log.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.cpp $(HEADER_LIB)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean :
	$(RM) *.o

.PHONY : all clean
