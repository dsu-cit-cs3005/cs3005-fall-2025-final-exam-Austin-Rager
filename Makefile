# Compiler
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic

# Targets
all: test_robot main

RobotBase.o: RobotBase.cpp RobotBase.h
	$(CXX) $(CXXFLAGS) -fPIC -c RobotBase.cpp

Arena.o: Arena.cpp Arena.h RobotBase.h
	$(CXX) $(CXXFLAGS) -fPIC -c Arena.cpp

test_robot: test_robot.cpp RobotBase.o
	$(CXX) $(CXXFLAGS) test_robot.cpp RobotBase.o -ldl -o test_robot

main: main.cpp Arena.o RobotBase.o
	$(CXX) $(CXXFLAGS) main.cpp Arena.o RobotBase.o -ldl -o RobotWarz

clean:
	rm -f *.o test_robot RobotWarz *.so
