#include "SSEQStream.h"

#include <istream>
#include <fstream>

int indexTable[16] = {-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};

int stepSizeTable[89] = {7, 8, 9, 10, 11, 12, 13, /* quantizer lookup table */
14, 16, 17, 19, 21, 23, 25, 28,
31, 34, 37, 41, 45, 50, 55, 60, 
66, 73, 80, 88, 97, 107, 118, 
130, 143, 157, 173, 190, 209, 230, 
253, 279, 307, 337, 371, 408, 449, 
494, 544, 598, 658, 724, 796, 876, 
963, 1060, 1166, 1282, 1411, 1552, 
1707, 1878, 2066, 2272, 2499, 2749, 
3024, 3327, 3660, 4026, 4428, 4871, 
5358, 5894, 6484, 7132, 7845, 8630, 
9493, 10442, 11487, 12635, 13899, 
15289, 16818, 18500, 20350, 22385, 
24623, 27086, 29794, 32767};

int binary_as_int(char* data){
	/*
	* Reads 4 bytes as a single integer, little endian
	*/
	int number = static_cast<unsigned char>(data[0]);
	number += static_cast<unsigned char>(data[1]) << 8;
	number += static_cast<unsigned char>(data[2]) << 16;
	number += data[3] << 24;
	return number;
}

short binary_as_short(char* data){
	short number = static_cast<unsigned char>(data[0]);
	number += data[1] << 8;
	return number;
}

unsigned short binary_as_ushort(char* data){
	unsigned short number = static_cast<unsigned char>(data[0]);
	number += static_cast<unsigned char>(data[1]) << 8;
	return number;
}

std::string SWAV::info(){
	std::string info{};
	info += "Wave ID=" + std::to_string(index) + " "; 
	info += type ? "ADPCM " : "PCM8 ";
	info += loopSupport ? "Loops " : "";
	info +=	"Sample Rate=" + std::to_string(sampleRate) + " ";
	info +=	"Duration=" + std::to_string(duration) + " ";
	if (loopSupport){
		info +=	"Loop Start=" + std::to_string(loopStart) + " ";
		info +=	"Loop Length=" + std::to_string(loopLength) + " ";
	}
	info += "Samples=" + std::to_string(samples.size()) +" ";
	return info;
}

//void decode_pcm8(std::vector<char> d, std::vector<short>& res, int start, int size){
void decode_pcm8(char* data, SWAV& wave){
	for (std::size_t i = 0; i < wave.samples.size(); ++i){
		char s8 = data[i];
		wave.samples[i] = s8 * 256;
	}
}

void decode_pcm16(char* data, SWAV& wave){
	for (std::size_t i = 0; i < wave.samples.size(); ++i){
		short s16 = binary_as_short(&data[2*i]);
		wave.samples[i] = s16;
	}
}

// https://problemkaputt.de/gbatek.htm#dssoundfilesswavsoundwavedata
void decode_single_sample(char& s, int& sample, int& index){
	//int diff = s*stepSizeTable[index]/4 + stepSizeTable[index]/8;
	int diff = stepSizeTable[index]/8;
	if (s & 1)
		diff += stepSizeTable[index]/4;
	if (s & 2)
		diff += stepSizeTable[index]/2;
	if (s & 4)
		diff += stepSizeTable[index]/1;

	if (s & 8)
		diff = -diff;
	sample += diff;
	if (sample < -32767)
		sample = -32767;
	else if (sample > 32767)
		sample = 32767;
	index += indexTable[s & 7];
	if (index < 0)
		index = 0;
	else if (index > 88)
		index = 88;
}

/*void decode_adpcm(std::vector<char> d, std::vector<short>& res, int start, int size){*/
void decode_adpcm(char* data, SWAV& wave){
	/*
	* data - pointer to beginning of ADPCM data
	* wave - SWAV struct to store generated samples to
	*/
	int sample = binary_as_short(&data[SWAV::ADPCM_INITIAL_SAMPLE]);
	int index = data[SWAV::ADPCM_INITIAL_INDEX];
	
	int adpcm_start = SWAV::ADPCM_HEADER_SIZE;
	for (std::size_t i = 0; i < wave.samples.size(); i += 2){
		unsigned char sampleduo = static_cast<unsigned char>(data[adpcm_start + i/2]);
		char s1 = (sampleduo >> 4) & 0x0f;
		char s2 = (sampleduo & 0x0f);
		
		decode_single_sample(s1, sample, index);
		wave.samples[i]=sample;
		decode_single_sample(s2, sample, index);
		wave.samples[i+1]=sample;
	}
}

#include <iostream>
void SWAR::open(std::string fname){
	std::ifstream is{fname, std::ios::binary | std::ios::in};
	data.resize(2*1024*1024);
	is.read(data.data(), 2*1024*1024);
	
	int count = binary_as_ushort(&data[SWAR::SAMPLE_COUNT]);
	for (int i = 0; i < count * 4; i += 4){
		int ofs = binary_as_int(&data[SAMPLE_OFFSETS_START + i]);
		offsets.push_back(ofs);
	}
	
	swav.resize(offsets.size());
	for (int i = 0; i < (int)swav.size()-1; ++i){
		int start = offsets[i];
		
		SWAV& wave = swav[i];
		wave.index = i;
		wave.type = data[start+SWAV::TYPE];	
		
		int data_size = offsets[i+1]-start-SWAV::HEADER_SIZE;
		
		wave.loopSupport = data[start+SWAV::LOOP_SUPPORT];
		wave.sampleRate = binary_as_ushort(&data[start+SWAV::SAMPLE_RATE]);
		wave.duration = binary_as_ushort(&data[start+SWAV::DURATION]);
		wave.loopStart = 4 * binary_as_ushort(&data[start+SWAV::LOOP_START]);
		wave.loopLength = 4 * binary_as_int(&data[start+SWAV::LOOP_LENGTH]);

		if (wave.type == SWAV::ADPCM_TYPE){
			data_size -= SWAV::ADPCM_HEADER_SIZE;
			data_size *= 2; //samples are 2 per byte
			wave.loopStart -= 4;
			wave.loopStart *= 2; //2 per byte
			wave.loopLength *= 2;
		} else if (wave.type == SWAV::PCM16_TYPE){
			data_size /= 2; //samples are 2 bytes each
			wave.loopStart /= 2; //2 byte each samples
			wave.loopLength /= 2;
		}
		wave.samples.resize(data_size);
		
		if (wave.type == SWAV::ADPCM_TYPE){
			decode_adpcm(&data[start], wave);
		}else if (wave.type == SWAV::PCM8_TYPE){
			decode_pcm8(&data[start], wave);
		}else if (wave.type == SWAV::PCM16_TYPE){
			decode_pcm16(&data[start], wave);
		}else 
			throw std::runtime_error{"Unknown format (Not ADPCM, PCM8, or PCM16)"};
	}
}

void SSEQStream::init(std::vector<short>* dat, unsigned int sampleRate, unsigned int loopStart){
	initialize((unsigned int)1, (unsigned int)sampleRate);
	d = dat;
	ls = loopStart;
}

SWAVStream::SWAVStream(SWAV& swav) : wave{swav} {
	initialize((unsigned int)1, (unsigned int)wave.sampleRate);
}

#include <algorithm>
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

void SWAVStream::onSeek(sf::Time){
	
}

bool SSEQStream::onGetData(Chunk& c){
	return false;
}

void SSEQStream::onSeek(sf::Time){
	
}
