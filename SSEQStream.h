#pragma once

#include <SFML/Audio.hpp>
#include <vector>

#include "SWAV.hpp"
#include "SWAR.hpp"
#include "SBNK.hpp"
#include "SSEQ.hpp"

//https://www.romhacking.net/documents/%5B469%5Dnds_formats.htm#SDAT

/*class SWAVStream : public sf::SoundStream {
public:
	SWAVStream(SWAV&);
	
	std::vector<short>& get_samples();
private:
	SWAV& wave;
	std::vector<short> samples;
	
	virtual bool onGetData(Chunk& c);
	virtual void onSeek(sf::Time t);
	
};*/

/*class InstrumentStream : public sf::SoundStream {
public:
	InstrumentStream(SWAV&, SBNK&);

private:
	SWAV& wave;
	SBNK& bank;
	std::vector<short> samples;
	
	virtual bool onGetData(Chunk& c);
	virtual void onSeek(sf::Time t);
};*/

/*class SampleStream : public sf::SoundStream {
public:
	void init(std::vector<short>*, unsigned int, unsigned int);
	
private:
	unsigned int ls = 0;
	std::vector<short>* d = nullptr;
	std::vector<sf::Int16> samps;
	virtual bool onGetData(Chunk& c);
	virtual void onSeek(sf::Time t);
	
	//virtual sf::Int64 onLoop();
};*/

struct Channel {
	int id;
	int offset;
	bool enabled = false;
	
	std::string info();
	
	int current_index; //event index in SSEQ
	
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

class SSEQStream : public sf::SoundStream {
public:
	SSEQStream(SWAR&, SBNK&, SSEQ&);
	const static int PLAYBACK_SAMPLE_RATE = 44100;
	const static int BUFFER_SIZE = 50000;
	const static int SFML_CHANNEL_COUNT = 2; //for enabling stereo. NOT related to Channel struct!
	const static int NDS_CHANNEL_COUNT = 16; //Number of channels for NDS SSEQ playback. Related to Channel struct.
	
private:
	int tempo;
	int offset = 0;
	
	std::vector<Channel> channels;
	
	SWAR& swar;
	SBNK& sbnk;
	SSEQ& sseq;
	
	std::vector<sf::Int16> samples;
	
	std::vector<NoteEvent> note_events;
	
	short get_sample(Channel& channel, NoteEvent& note_event);
	short apply_adsr(Channel& channel, NoteEvent& note, short sample);
	
	void process_event(Channel& channel, Event& event);
	int index_from_offset(int offset);
	
	// sf::SoundStream overrides
	virtual bool onGetData(Chunk& c) override;
	virtual void onSeek(sf::Time t) override;
};
