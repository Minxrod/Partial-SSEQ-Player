all:
	g++ -Wall -Werror -Wpedantic -Wextra -std=c++17 -g binhelper.cpp SWAV.cpp SWAR.cpp SBNK.cpp SSEQ.cpp SSEQStream.cpp main.cpp -lsfml-audio -lsfml-system -lpthread -o SSEQTest
	
clean:
	rm SSEQTest
