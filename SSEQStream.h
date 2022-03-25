#pragma once

#include <SFML/Audio.hpp>
#include <vector>

#include "SWAV.hpp"
#include "SWAR.hpp"
#include "SBNK.hpp"
#include "SSEQ.hpp"

//https://www.romhacking.net/documents/%5B469%5Dnds_formats.htm#SDAT


class SWAVStream : public sf::SoundStream {
public:
	SWAVStream(SWAV&);
	
	std::vector<short>& get_samples();
private:
	SWAV& wave;
	std::vector<short> samples;
	
	virtual bool onGetData(Chunk& c);
	virtual void onSeek(sf::Time t);
	
};

class InstrumentStream : public sf::SoundStream {
public:
	InstrumentStream(SWAV&, SBNK&);

private:
	SWAV& wave;
	SBNK& bank;
	std::vector<short> samples;
	
	virtual bool onGetData(Chunk& c);
	virtual void onSeek(sf::Time t);
};

class SampleStream : public sf::SoundStream {
public:
	void init(std::vector<short>*, unsigned int, unsigned int);
	
private:
	unsigned int ls = 0;
	std::vector<short>* d = nullptr;
	std::vector<sf::Int16> samps;
	virtual bool onGetData(Chunk& c);
	virtual void onSeek(sf::Time t);
	
	//virtual sf::Int64 onLoop();
};

class SSEQStream : public sf::SoundStream {
public:
	SSEQStream(SWAR&, SBNK&, SSEQ&);
	
private:
	const static int PLAYBACK_SAMPLE_RATE = 22050;
	int tempo;
	
	SWAR& swar;
	SBNK& sbnk;
	SSEQ& sseq;
	
	std::vector<sf::Int16> samples;
	
	virtual bool onGetData(Chunk& c);
	virtual void onSeek(sf::Time t);
};
