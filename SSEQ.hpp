#pragma once

#include <vector>
#include <string>

struct Event {
	const static unsigned char 
		NOTE_LOW=0,
		NOTE_HIGH=0x7f,
		REST=0x80,
		BANK=0x81,
		ENABLED_TRACKS=0xFE,
		DEFINE_TRACK=0x93,
		JUMP=0x94,
		CALL=0x95,
		PAN=0xc0,
		VOLUME=0xc1,
//		MASTER_VOLUME=0xc2,
		MODULATION_DEPTH=0xca,
		MODULATION_SPEED=0xcb,
		MODULATION_TYPE=0xcc,
		MODULATION_RANGE=0xcd,
		PITCH_BEND=0xc4,
		PITCH_BEND_RANGE=0xc5,
		MONO_POLY=0xc7, //1 byte
		TEMPO=0xe1, //2 byte
		RETURN=0xfd,
		TRACKS_ENABLED=0xfe,
		END_OF_TRACK=0xff;
	
	Event(unsigned char type):event_type{type}{}
	
	unsigned char event_type;
	int value1;
	int value2;
	
	void read(char* data);
	std::string info();
};

struct Channel {
	int id;
	int offset;
	bool enabled = false;
	
//	std::vector<Event> events;
	
	void read(char* data);
	std::string info();
	
	int current_index;
};

struct SSEQ {
	const int SIZE = 0x14;
	const int START_OFFSET = 0x18;
	
	std::vector<Channel> channels;
	std::vector<char> data;
	std::vector<int> event_location; //location in data
	std::vector<Event> events; //converted from data
	
	void open(std::string filename);
	std::string info();
	
private:
	int variable_length(int& i);
};
