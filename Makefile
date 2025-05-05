CXX = g++
CXXFLAGS = -Wall -pthread -lncursesw

all: chat

chat: main.o ChatApp.o
	$(CXX) -o chat main.o ChatApp.o $(CXXFLAGS)

main.o: main.cpp ChatApp.h
	$(CXX) -c main.cpp

ChatApp.o: ChatApp.cpp ChatApp.h
	$(CXX) -c ChatApp.cpp

clean:
	rm -f *.o chat
