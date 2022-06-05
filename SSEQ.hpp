#pragma once

#include <vector>
#include <string>

/**
 * Contains one SSEQ event.
 * 
 * A SSEQ event has a type, and [0-2] arguments.
 * These arguments vary in meaning by the event type. See
 * https://gota7.github.io/NitroStudio2/specs/sequence.html
 * or https://www.problemkaputt.de/gbatek.htm for some explanation.
 * Note that these sources disagree on some things, so I've used whatever
 * seemed to be most accurate based on my understanding.
 */
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
		PRIORITY=0xc6, //1 byte
		MONO_POLY=0xc7, //1 byte
		MODULATION_DEPTH=0xca,
		MODULATION_SPEED=0xcb,
		MODULATION_TYPE=0xcc,
		MODULATION_RANGE=0xcd,
		VOLUME_2=0xd5,
		TEMPO=0xe1, //2 byte
		RETURN=0xfd,
		TRACKS_ENABLED=0xfe,
		END_OF_TRACK=0xff;
	
//	Event(unsigned char t):type{t}{}
	Event(unsigned char t, int v1 = 0, int v2 = 0):type{t},value1{v1},value2{v2}{}
	
	// The type of the event. See the constants above for specific types that
	// have been implemented or should be eventually.
	unsigned char type;
	// First argument for the event.
	int value1 = 0;
	// Second argument for the event.
	int value2 = 0;
	
	void read(char* data);
	std::string info();
};

struct Channel;

struct NoteEvent
{
	const static int ADSR_MAXIMUM = 0;        //100%
	const static int ADSR_MINIMUM = -92544;   //0%
	
	const static int PHASE_NONE = 0;
	const static int PHASE_ATTACK = 1;
	const static int PHASE_DECAY = 2;
	const static int PHASE_SUSTAIN = 3;
	const static int PHASE_RELEASE = 4;	
	
	Event* event = nullptr;
	double current_sample; //increments by variable amount depending on pitch
	int phase_sample; //increments by one each sample 
	int max_samples;
	
	int amplitude;
	int phase;
	int adsr_update_rate; //in samples / update
	int adsr_sample;
	
	Channel* channel = nullptr;
};


struct SSEQ {
	const static int SIZE = 0x14;
	const static int START_OFFSET = 0x18;
	
	std::vector<int> event_location; //location in data
	std::vector<Event> events; //converted from data
	
	void open(std::string filename);
	void mml(std::string mml_data);
	std::string info();
	
	int tempo; // processed during initial events
	
	SSEQ& operator=(const SSEQ&) = default;
	
private:
	int variable_length(std::vector<char>& data, int& i);
};
