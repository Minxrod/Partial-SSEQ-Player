CXX=g++
CXXFLAGS=-Wall -Werror -Wpedantic -Wextra -std=c++17 -fPIC -g

# build all objs by implicit rule
OBJECTS=binhelper.o SWAV.o SWAR.o SBNK.o SSEQ.o SSEQStream.o main.o

exec: $(OBJECTS)
	g++ $(CXXFLAGS) $(OBJECTS) -lsfml-audio -lsfml-system -lpthread -o SSEQTest

lib: $(OBJECTS)
	g++ $(CXXFLAGS) $(OBJECTS) -lsfml-audio -lsfml-system -lpthread -shared -o libSSEQPlayer.so

all: lib exec

clean:
	rm -f SSEQTest
	rm -f libSSEQPlayer.so

-include $(objs:%.o=%.d)
