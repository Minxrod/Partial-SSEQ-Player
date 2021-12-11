all:
	g++ -Wall -Werror -Wpedantic -Wextra -std=c++17 -g SSEQStream.cpp main.cpp -lsfml-audio -lsfml-system -lpthread -o SSEQTest
	
clean:
	rm SSEQTest
