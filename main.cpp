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
	swar.open("test_files/ptc/SWAR_0.swar");
	
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
	bank.open("test_files/ptc/SBNK_0.sbnk");
	
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
	for (int i = 30; i < 100; ++i){
		SSEQ sseq{};
//		sseq.open("test_files/nsmb/BGM_AMB_CHIKA.sseq");
//		sseq.open("test_files/ptc/SSEQ_"+std::to_string(i)+".sseq");
//		sseq.mml(":0{AA=ABCDE}T180@224O4{AA}{AA}");
		
		// test MML taken from Village
		sseq.mml("T180{SA1=B8&<[F#R]7>}:0@V127V85@147L32O5R2R8[ER]3RRR40.R20R40.R8L16O3[[F#R]7R8]2[G#R]7R8<@145[G#R]3A#RR4.[@E127,127,127,127@147O2[{SA1}]2<C#8&[G#R]3C#8&[F#R]3>[{SA1}]3<C#8&[G#R]3D#8[A#R]3G#8&<[G#R]3>F#8&<[F#R]3L16>G#.R.G#R4[A#R]2R4.[F#R]7R8[F#R]7@144O5L16@E125,96,90,124D4C#8.DE4D8.EF#4E8.F#>B4B8.<C#D4C#8.DE4D8.EF#2Q7B4Q8@147@E110,96,80,127B8.<C#D4C#8.DE4D8.EF#4E8.F#>B4B8.<C#D4C#8.DE4D8.EC#2Q7F#4Q8R4]{SB1=F#8&<[D#R]7>}:1@V127V85@147L32O2[F#RF#RRR]2[F#RRR]2O5[F#R]3RRO2F#40.G#20A#40.L16O4[R8[D#R]7]2R8[ER]7R8<@145[ER]2[F#R]2R4.[@E127,127,127,127@147O3[{SB1}]2G#8&[BR]3F#8&[A#R]3[{SB1}]3G#8&[BR]3G8&<[D#R]3O3L32[BR<D#R>]8L16<E.R.ER4[F#R]2R4.>[D#R]7R8[D#R]7@144O4L16@E124,96,96,124G4G8.G<C#4>B8.<C#D4C#8.D>G4G8.AG4G8.G<C#4>B8.<C#D2Q6F#4Q8@147@E110,96,96,127G8.AG4G8.G<C#4>B8.<C#D4C#8.D>G4G8.AG4G8.G<C#4>B8.<C#>A#2Q6A#4Q8R4]{T1=O5L32B2&B8.R16B8&BR<D#RF#16.R@D64L16A#RAR@D0G#RL32F#8.&F#RD#8&D#R}{T2=L32F#RE8.&ERC#8&C#RD#RC#8.&C#RD#8&D#RC#R}{T3=L32O3ERBR<[GRER]3>AR<ER[ARER]3>DRAR<[F#RDR]3>GR<DR[GRDR]3}:2@V127V127@224O4L32[F#RF#RRR]2[F#RRR]2<<[C#R]3RRO3F#40.G#20A#40.O2[B6R2R3]2<C#6R2R3R8<[F#16R16]4R8<F#16.R16.F#R[{T1}{T2}>B2&B8&BR16.F#16.R16.F#R{T1}{T2}>B2&B8&BR16.R4B8&BRBR<C#RR16D#8.&D#R>BR16.<C#16.R>B16.RO2[B6R2R3]2{T3}{T3}{T3}>ERBR<[ER>BR<]3>AR<ER[ARER]3>F#R16.F#R<C#R>F#R16.F#R<C#RF#RR8.<F#16.R16.F#R]{NI1=Q5V64@E127,127,127,124O8C32R16.}{NI2=Q5V63@E127,127,127,124O7C32R16.}{NI3=Q5V64@E127,127,127,124O7F32R16.}{NI4=Q7V64@E127,120,127,125O7C64R32.}{NI5=Q7V64@E127,120,127,125O7F64R32.}{N1=[{NI2}{NI5}{NI5}]3[{NI4}{NI5}]2}:3@V127@151[R1]4R8[{NI1}]4R4.[[{N1}]8{NI5}R16[{NI4}]2{NI5}R16[{NI4}]3R16{NI4}R16R4[{N1}]9{NI5}R16[{NI4}]2{NI5}R16[{NI4}]2O8C32R16.R4.]");
		std::cout << i << ": " << sseq.info() << std::endl;
		
		seqstrm.reset();
		seqstrm.set_sseq(&sseq);
		seqstrm.play();
		sf::sleep(sf::seconds(60));
		seqstrm.stop(); //needed here (probably) because sseq gets destroyed after this point
	}
//	std::cout << sseq.info() << std::endl;
	
	return 0;
}
