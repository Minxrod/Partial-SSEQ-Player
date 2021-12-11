#pragma once

#include <SFML/Audio.hpp>
#include <vector>

//https://www.romhacking.net/documents/%5B469%5Dnds_formats.htm#SDAT

struct SWAV {
	const static int TYPE = 0;
	const static int LOOP_SUPPORT = 1;
	const static int SAMPLE_RATE = 2; //2 bytes
	const static int DURATION = 4; //2 bytes
	const static int LOOP_START = 6; //6 bytes
	const static int LOOP_LENGTH = 8; //4 bytes
	const static int HEADER_SIZE = 12;
	
	const static int ADPCM_HEADER = 12; //4 bytes
	const static int ADPCM_INITIAL_SAMPLE = 0; //2 bytes
	const static int ADPCM_INITIAL_INDEX = 2; //1 byte
	const static int ADPCM_HEADER_SIZE = 4; //4 bytes
	
	const static int PCM8_TYPE = 0;
	const static int PCM16_TYPE = 1; //not intended to support this
	const static int ADPCM_TYPE = 2;
	
	
	int index;
	int type;
	bool loopSupport;
	int sampleRate;
	int duration; //useless
	int loopStart; //index into samples
	int loopLength; //index into samples
	std::vector<short> samples;
	
	std::string info();
};

struct SWAR {
	const int SAMPLE_COUNT = 0x38;
	const int SAMPLE_OFFSETS_START = 0x3c;
	
	std::vector<char> data;
	std::vector<int> offsets;
	std::vector<SWAV> swav;
	
	void open(std::string);
};

struct SBNK {
	void open();
};

class SWAVStream : public sf::SoundStream {
public:
	SWAVStream(SWAV&);

private:
	SWAV& wave;
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
