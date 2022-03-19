#include "SWAR.hpp"

#include <fstream>

#include "binhelper.hpp"

const int indexTable[16] = {-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};

const int stepSizeTable[89] = {7, 8, 9, 10, 11, 12, 13, /* quantizer lookup table */
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
// this is very condensable...
void decode_single_sample(char s, int& sample, int& index){
	//int diff = s*stepSizeTable[index]/4 + stepSizeTable[index]/8;
	int diff = stepSizeTable[index]/8;
	if (s & 1)
		diff += stepSizeTable[index]/4;
	if (s & 2)
		diff += stepSizeTable[index]/2;
	if (s & 4)
		diff += stepSizeTable[index]/1;
	
	if ((s & 8) == 0)
		sample = std::min(sample+diff,0x7FFF);
	if ((s & 8) == 8)
		sample = std::max(sample-diff,-0x7FFF);

/*	if (s & 8)
		diff = -diff;
	sample += diff;
	if (sample < -32767)
		sample = -32767;
	else if (sample > 32767)
		sample = 32767;*/
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
		//NOTE: The first sample in a pair is actually the lower nybble, the second sample is the higher nybble.
		decode_single_sample(s2, sample, index);
		wave.samples[i]=sample;
		decode_single_sample(s1, sample, index);
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
		
		int data_size = offsets[i+1] - start - SWAV::HEADER_SIZE - (wave.type==SWAV::ADPCM_TYPE ? SWAV::ADPCM_HEADER_SIZE : 0);
		
		wave.loopSupport = data[start+SWAV::LOOP_SUPPORT];
		wave.sampleRate = binary_as_ushort(&data[start+SWAV::SAMPLE_RATE]);
		wave.duration = binary_as_ushort(&data[start+SWAV::DURATION]);
		wave.loopStart = 4 * binary_as_ushort(&data[start+SWAV::LOOP_START]);
		wave.loopLength = 4 * binary_as_int(&data[start+SWAV::LOOP_LENGTH]);
		
		if (wave.type == SWAV::ADPCM_TYPE){
//			data_size -= SWAV::ADPCM_HEADER_SIZE;
			data_size *= 2; //samples are 2 per byte
			wave.loopStart -= SWAV::ADPCM_HEADER_SIZE; 
			wave.loopStart *= 2; //2 per byte
//			wave.loopStart += 1;
			wave.loopLength *= 2;
//			wave.loopLength -= 1;
		} else if (wave.type == SWAV::PCM16_TYPE){
			data_size /= 2; //samples are 2 bytes each
			wave.loopStart /= 2; //2 byte each samples
			wave.loopLength /= 2;
		}
		wave.samples.resize(data_size);
		
		start+=SWAV::HEADER_SIZE;
		if (wave.type == SWAV::ADPCM_TYPE){
			decode_adpcm(&data[start], wave);
			wave.samples.insert(wave.samples.begin(), data[SWAV::ADPCM_INITIAL_SAMPLE]);
		}else if (wave.type == SWAV::PCM8_TYPE){
			decode_pcm8(&data[start], wave);
		}else if (wave.type == SWAV::PCM16_TYPE){
			decode_pcm16(&data[start], wave);
		}else 
			throw std::runtime_error{"Unknown format (Not ADPCM, PCM8, or PCM16)"};
	}
}
