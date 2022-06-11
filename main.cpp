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
	swar.open("test_files/ptc/SWAR_0000.swar");
	
	// Test SWAR loading
//	for (int i = 0; i < (int)swar.swav.size(); ++i){
//		std::cout << swar.swav[i].info() << std::endl;
//		
//		SWAVStream swavstrm{swar.swav[i]};
//		swavstrm.play();
//		
//		sf::sleep(sf::seconds(1));
//		swavstrm.stop();
//	}
//	std::cout << swar.swav[230].info() << std::endl;
	
	SBNK bank;
	bank.open("test_files/ptc/SBNK_0000.sbnk");
	
//	for (auto& instr : bank.instruments){
//		std::cout << instr.info() << std::endl;
//	}
//	
//	SWAVStream strm2{swar.swav[279]};
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
	
	//20: implement 0xd5
	//23: implement 0xc6
	//76: implement 0xd5
	SSEQStream seqstrm{swar, bank};
	for (int i = 0; i < 30; ++i){
		SSEQ sseq{};
//		sseq.open("test_files/nsmb/BGM_AMB_CHIKA.sseq");
		sseq.open("test_files/ptc/SSEQ_"+std::to_string(i)+".sseq");
//		sseq.mml(":0T180@224O4[[CD]ER2.]0FG");
		std::cout << i << ": " << sseq.info() << std::endl;
		
		seqstrm.reset();
		seqstrm.set_sseq(&sseq);
		seqstrm.play();
		sf::sleep(sf::seconds(30));
		seqstrm.stop(); //needed here (probably) because sseq gets destroyed after this point
	}
//	std::cout << sseq.info() << std::endl;
	
	return 0;
}
