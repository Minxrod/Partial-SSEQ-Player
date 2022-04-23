exec:
	g++ -Wall -Werror -Wpedantic -Wextra -std=c++17 -g binhelper.cpp SWAV.cpp SWAR.cpp SBNK.cpp SSEQ.cpp SSEQStream.cpp main.cpp -lsfml-audio -lsfml-system -lpthread -o SSEQTest

lib:
	g++ -Wall -Werror -Wpedantic -Wextra -std=c++17 -fPIC -g binhelper.cpp SWAV.cpp SWAR.cpp SBNK.cpp SSEQ.cpp SSEQStream.cpp -lsfml-audio -lsfml-system -lpthread -shared -o libSSEQPlayer.so

all: lib exec

clean:
	rm -f SSEQTest
	rm -f libSSEQPlayer.so
