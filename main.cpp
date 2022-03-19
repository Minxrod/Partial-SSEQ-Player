#include <SFML/System.hpp>
#include <iostream>
#include <fstream>

#include "SWAR.hpp"
#include "SBNK.hpp"
#include "SSEQStream.h"


int main(){
	SWAR swar;
	swar.open("SWAR_0000.swar");
	
	// Test SWAR loading
	for (int i = 145; i < 152/*(int)swar.swav.size()*/; ++i){
		std::cout << swar.swav[i].info() << std::endl;
		
//		SWAVStream swavstrm{swar.swav[i]};
//		swavstrm.play();
//		
//		sf::sleep(sf::seconds(5));
//		swavstrm.stop();
	}
	
	SBNK bank;
	bank.open("SBNK_0000.sbnk");
	
//	for (auto& instr : bank.instruments){
		std::cout << bank.instruments[0].info() << std::endl;
//	}
//	SWAVStream strm2{swar.swav[150]};
//	strm2.play();
//	sf::sleep(sf::seconds(1));
//	strm2.stop();
	char tt = -0x16;
	std::cout << (int)(static_cast<unsigned char>(tt)) << std::endl;
	{
		int sampleIndex = 9;
		std::ofstream fout{"samples.txt"};
		for (std::size_t i = 0; i < swar.swav[sampleIndex].samples.size(); ++i){
			fout << swar.swav[sampleIndex].samples[i] << ((int)i == swar.swav[sampleIndex].loopStart ? "\n" : ",");
		}
		fout << std::endl;
	}
	
	for (int i : {150,146,148,149,150,149}){
		SWAVStream strm{swar.swav[i]};
		strm.play();
		sf::sleep(sf::seconds(2));
		strm.stop();
	}
	
	return 0;
}
