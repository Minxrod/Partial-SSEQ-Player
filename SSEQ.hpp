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

struct NoteEvent
{
	Event* event;
	int current_sample;
	int max_samples;
};

struct Channel {
	int id;
	int offset;
	bool enabled = false;
	
//	void read(char* data);
	std::string info();

	int current_index; //event index in SSEQ
	
	const static int ADSR_MAXIMUM = 0;        //100%
	const static int ADSR_MINIMUM = -92544;   //0%
	
	const static int PHASE_NONE = 0;
	const static int PHASE_ATTACK = 1;
	const static int PHASE_DECAY = 2;
	const static int PHASE_SUSTAIN = 3;
	const static int PHASE_RELEASE = 4;	
	
	// Volume etc.
	int volume = 128;
	int pan = 64;
	
	// Pitch bending
	int pitch_bend_range = 1;
	int pitch_bend = 0;
	
	// ADSR related values
	int amplitude;
	int phase;
	
	int next_process_delay; //how long to wait to start processing again

	int current_sample;
	int max_samples;
	Event* current_note_event;
	std::vector<NoteEvent> note_events;
	//currently playing instrument
	int instr = 0;

	
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
