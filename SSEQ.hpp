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
		PITCH_BEND=0xc4,
		PITCH_BEND_RANGE=0xc5,
		MONO_POLY=0xc7, //1 byte
		MODULATION_DEPTH=0xca,
		MODULATION_SPEED=0xcb,
		MODULATION_TYPE=0xcc,
		MODULATION_RANGE=0xcd,
		TEMPO=0xe1, //2 byte
		RETURN=0xfd,
		TRACKS_ENABLED=0xfe,
		END_OF_TRACK=0xff;
	
	Event(unsigned char t):type{t}{}
	
	unsigned char type;
	int value1;
	int value2;
	
	void read(char* data);
	std::string info();
};

struct Channel {
	int id;
	int offset;
	bool enabled = false;
	
//	void read(char* data);
	std::string info();
	
	int instr = 0;
	int current_index; //event index in SSEQ
	
	int current_sample; //current sample (during playback)
	int max_samples; //length of this event (in samples; during playback)
	int next_process_delay; //how long to wait to start processing again
	Event* current_note_event;
	
	std::vector<int> call_stack{};
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
	
	int tempo; // processed during initial events
	
private:
	int variable_length(int& i);
};
