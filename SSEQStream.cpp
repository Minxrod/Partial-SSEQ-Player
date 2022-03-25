#include "SSEQStream.h"

#include <istream>
#include <fstream>
#include <algorithm>


SSEQStream::SSEQStream(SWAR& wave, SBNK& bank, SSEQ& sequence) : swar(wave), sbnk(bank), sseq(sequence){
	samples.resize(50000); //arbitrary
	initialize(1, PLAYBACK_SAMPLE_RATE);
	tempo = 147; //temp test value for SSEQ_0000
}

bool SSEQStream::onGetData(Chunk& chunk){
	//play one note of SSEQ...
	chunk.sampleCount = 50000;
	
	for (std::size_t s = 0; s < chunk.sampleCount; ++s){
		
		
		
	}
	
	
	return false;
}

// Does nothing, because it isn't needed.
void SSEQStream::onSeek(sf::Time){}

/*
 * Random testing stuff below!
 */

void SampleStream::init(std::vector<short>* dat, unsigned int sampleRate, unsigned int loopStart){
	initialize((unsigned int)1, (unsigned int)sampleRate);
	d = dat;
	ls = loopStart;
}

SWAVStream::SWAVStream(SWAV& swav) : wave{swav} {
	initialize((unsigned int)1, (unsigned int)wave.sampleRate);
}

InstrumentStream::InstrumentStream(SWAV& swav, SBNK& sbnk) : wave{swav}, bank{sbnk} {
	initialize((unsigned int)1, (unsigned int)wave.sampleRate);
}

bool InstrumentStream::onGetData(Chunk& c){
//	c.sampleCount = wave.samples.size();	
//	c.samples = wave.samples.data();
	c.sampleCount = 22050*10;
	samples.resize(c.sampleCount);
	std::copy_n(wave.samples.begin(), wave.samples.size(), samples.begin());
	if (wave.loopSupport){
		for (std::size_t ofs = wave.loopStart; ofs < samples.size(); ofs += wave.loopLength){
			for (int i = 0; i < wave.loopLength && ofs+i < samples.size(); ++i){
				samples[ofs+i] = wave.samples[wave.loopStart+i];
			}
		}
	}
	
	c.samples = samples.data();
	
	return true;
}


std::vector<short>& SWAVStream::get_samples(){
	return samples;
}


bool SWAVStream::onGetData(Chunk& c){
//	c.sampleCount = wave.samples.size();	
//	c.samples = wave.samples.data();
	c.sampleCount = 22050*10;
	samples.resize(c.sampleCount);
	std::copy_n(wave.samples.begin(), wave.samples.size(), samples.begin());
	if (wave.loopSupport){
		for (std::size_t ofs = wave.loopStart; ofs < samples.size(); ofs += wave.loopLength){
			for (int i = 0; i < wave.loopLength && ofs+i < samples.size(); ++i){
				samples[ofs+i] = wave.samples[wave.loopStart+i];
			}
		}
	}	
	c.samples = samples.data();
	
	return true;
}

bool SampleStream::onGetData(Chunk&){
	return false;
}

void InstrumentStream::onSeek(sf::Time){}
void SWAVStream::onSeek(sf::Time){}
void SampleStream::onSeek(sf::Time){}
