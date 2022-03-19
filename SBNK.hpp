#pragma once

#include <string>
#include <vector>

//https://gota7.github.io/NitroStudio2/specs/bank.html
//also GBATek 

struct NoteDefinition {
	const int SWAV_NUMBER = 0; //size 2
	const int SWAR_NUMBER = 2; //size 2
	const int NOTE = 4; //size 1
	const int ATTACK = 5; //size 1
	const int DECAY = 6; //size 1
	const int SUSTAIN = 7; //size 1
	const int RELEASE = 8; //size 1
	const int PAN = 9; //size 1
	
	int swav_no;
	int swar_no;
	int note;
	int attack;
	int decay;
	int sustain;
	int release;
	int pan;
	
	void read(char* data);
	
	std::string info();
};

struct Instrument {
	const int F_RECORD_EMPTY = 0;
	const int F_RECORD_PCM = 1;
	const int F_RECORD_PSG = 2;
	const int F_RECORD_NOISE = 3;
	const int F_RECORD_RANGE = 16;
	const int F_RECORD_REGIONAL = 17;
	
	int id;
	int f_record;
	std::vector<int> note_numbers;
	std::vector<NoteDefinition> notes;
	
	void read(char* data);
	
	std::string info();
};

struct SBNK {
	const int INSTRUMENT_COUNT = 0x38;
	const int INSTRUMENT_RECORDS_START = 0x3c;
	const int INSTRUMENT_F_RECORD = 0; //size 1
	const int INSTRUMENT_OFFSET = 1; //size 2
	const int INSTRUMENT_RECORD_SIZE = 4; //1 unused byte (always 00?)
	
	std::vector<char> data;
	std::vector<int> offsets;
	std::vector<Instrument> instruments;
	
	void open(std::string name);
};
