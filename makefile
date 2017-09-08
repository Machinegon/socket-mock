CXX=g++
OBJS=socket-mock.cpp
OUT=socket-mock
C_FLAGS= -std=c++11 -static-libstdc++

all:
	$(CXX) $(OBJS) -o $(OUT) $(C_FLAGS)