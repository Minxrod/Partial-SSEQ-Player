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
	const static int PLAYBACK_SAMPLE_RATE = 44100;
	
private:
	int tempo;
	int offset = 0;
	
	SWAR& swar;
	SBNK& sbnk;
	SSEQ& sseq;
	
	bool which = false;
	std::vector<sf::Int16> samples1;
	std::vector<sf::Int16> samples2;
	
	short get_sample(int instrument, int note, std::size_t index);
	void process_event(Channel& channel, Event& event);
	int index_from_offset(int offset);
	
	// sf::SoundStream overrides
	virtual bool onGetData(Chunk& c) override;
	virtual void onSeek(sf::Time t) override;
};
