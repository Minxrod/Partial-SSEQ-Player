#include "SSEQStream.h"
#include <SFML/System.hpp>
#include <iostream>

int main(){
	SWAR swar;
	swar.open("SWAR_0000.swar");
	
	for (int i = 20; i < (int)swar.swav.size(); ++i){
		std::cout << swar.swav[i].info() << std::endl;
		
		SWAVStream swavstrm{swar.swav[i]};
		swavstrm.play();

		sf::sleep(sf::seconds(2));
		swavstrm.stop();
	}
	
	return 0;
}
