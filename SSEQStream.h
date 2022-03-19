#pragma once

#include <SFML/Audio.hpp>
#include <vector>

#include "SWAV.hpp"
#include "SBNK.hpp"

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

class SSEQStream : public sf::SoundStream {
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
