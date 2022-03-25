#include <SFML/System.hpp>
#include <iostream>
#include <fstream>
#include <cmath>

#include "SWAR.hpp"
#include "SBNK.hpp"
#include "SSEQ.hpp"
#include "SSEQStream.h"


int main(){
	SWAR swar;
	swar.open("SWAR_0000.swar");
	
	// Test SWAR loading
	//for (int i = 145; i < 152/*(int)swar.swav.size()*/; ++i){
	//	std::cout << swar.swav[i].info() << std::endl;
		
//		SWAVStream swavstrm{swar.swav[i]};
//		swavstrm.play();
//		
//		sf::sleep(sf::seconds(5));
//		swavstrm.stop();
//	}
	
	SBNK bank;
	bank.open("SBNK_0000.sbnk");
	
//	for (auto& instr : bank.instruments){
//		std::cout << instr.info() << std::endl;
//	}
	
//	SWAVStream strm2{swar.swav[150]};
//	strm2.play();
//	sf::sleep(sf::seconds(1));
//	strm2.stop();
	
	/*{
		int sampleIndex = 9;
		std::ofstream fout{"samples.txt"};
		for (std::size_t i = 0; i < swar.swav[sampleIndex].samples.size(); ++i){
			fout << swar.swav[sampleIndex].samples[i] << ((int)i == swar.swav[sampleIndex].loopStart ? "\n" : ",");
		}
		fout << std::endl;
	}*/
	
	/*for (int i : {145,146,148,149,150,149}){
		SWAVStream strm{swar.swav[i]};
		strm.play();
		sf::sleep(sf::seconds(2));
		strm.stop();
	}*/
	
//	auto& instr = bank.instruments[0];
/*	for (int note = 0; note < 128; ++note){
		auto& note_def = bank.instruments[0].get_note_def(note);
		
		int note_base = note_def.note;
		double note_pitch_shift = (note - note_base) / 12.0;
		SWAVStream s{swar.swav[note_def.swav_no]};
		s.setPitch(std::pow(2, note_pitch_shift));
		s.play();
		sf::sleep(sf::seconds(0.3));
		s.stop();
	}*/
	
	SSEQ sseq;
	sseq.open("SSEQ_0000.sseq");
	
	SSEQStream seqstrm{swar, bank, sseq};
	seqstrm.play();
	
	std::cout << sseq.info() << std::endl;
	
	return 0;
}
