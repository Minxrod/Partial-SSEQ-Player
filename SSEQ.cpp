//useful reference:
//https://gota7.github.io/NitroStudio2/specs/sequence.html

#include "SSEQ.hpp"

#include "binhelper.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <map>

const unsigned char 
	Event::REST,
	Event::BANK,
	Event::ENABLED_TRACKS,
	Event::DEFINE_TRACK,
	Event::TEMPO,
	Event::PAN,
	Event::VOLUME,
	Event::TIE,
	Event::LOOP_START,
	Event::LOOP_END,
	Event::TRACKS_ENABLED,
	Event::END_OF_TRACK;

// A variable length parameter. To read it, read a byte, binary and it 
// with 0x7F, and if binary anding the original value by 0x80 is 1, shift 
// that value left 7 bits, and binary or it with the same process you just did
// for the next bytes, until anding the value with 0x80 is 0
//
// (copied from https://gota7.github.io/NitroStudio2/specs/common.html)
//
// NOTE: This function modifies the i passed to it!
int SSEQ::variable_length(std::vector<char>& data, int& i){
	int value = 0;
	do { 
		value <<= 7;
		value |= data[i] & 0x7f;
		++i;
	} while (data[i-1] < 0);
	return value;
}

//type: event type
//data: pointer to start of event parameters (past type)
Event event_u16(unsigned char type, char* data){
	return Event(type, binary_as_ushort(data));
}

Event event_u8_s16(unsigned char type, char* data){
	return Event(type, static_cast<unsigned char>(data[0]), binary_as_short(&data[1]));
}

Event event_s8(unsigned char type, char* data){
	return Event(type, data[0]);
}

Event event_u8(unsigned char type, char* data){
	return Event(type, static_cast<unsigned char>(data[0]));
}

Event SSEQ::read_event(std::vector<char>& d, int& i){
	unsigned char type = static_cast<unsigned char>(d[i]);
	++i; //data of event's index (or next event, if no data needed)
	char* data = &d[i]; //parameter data (if exists)
	
	if (type == Event::TRACKS_ENABLED){
		i+=2; // advance past size of event
		return event_u16(type, data);
	} else if (type == Event::DEFINE_TRACK){
		int track = d[i];
		int offset = binary_as_u24(&d[i+1]);
		
		i+=4;
		return Event(type, track, offset);
	} else if (type == Event::MONO_POLY){
		++i;
		return event_u8(type, data);
	} else if (type == Event::TEMPO){
		i+=2;
		return event_u16(type, data);
	} else if (type == Event::VOLUME){
		++i;
		return event_u8(type, data);
	} else if (type == Event::PAN){
		++i;
		return event_u8(type, data);
	} else if (type == Event::BANK){
		int instr_bank = variable_length(d, i);
		int instrument = instr_bank & 0x00ff; //instrument
		int bank = (instr_bank & 0x3f00) >> 8; //bank
		return Event(type, instrument, bank);
	} else if (Event::NOTE_LOW <= type && type <= Event::NOTE_HIGH){
		int velocity = static_cast<unsigned char>(d[i]);
		++i;
		int duration = variable_length(d, i);
		return Event(type, velocity, duration);
	} else if (type == Event::REST){
		int duration = variable_length(d, i);
		return Event(type, duration);
	} else if (type == Event::PITCH_BEND){
		++i;
		return event_s8(type, data);
	} else if (type == Event::PITCH_BEND_RANGE){
		++i;
		return event_u8(type, data);
	} else if (type == Event::MODULATION_DEPTH){
		++i;
		return event_u8(type, data);
	} else if (type == Event::MODULATION_SPEED){
		++i;
		return event_u8(type, data);
	} else if (type == Event::MODULATION_TYPE){
		++i;
		return event_u8(type, data);
	} else if (type == Event::MODULATION_RANGE){
		++i;
		return event_u8(type, data);
	} else if (type == Event::PRIORITY){
		++i;
		return event_u8(type, data);
	} else if (type == Event::VOLUME_2){
		++i;
		return event_u8(type, data);
	} else if (type == Event::CALL){
		int offset = binary_as_u24(data);
		i+=3;
		return Event(type, offset);
	} else if (type == Event::JUMP){
		int offset = binary_as_u24(data);
		i+=3;
		return Event(type, offset);
	} else if (type == Event::RETURN){
		return Event(type);
	} else if (type == Event::END_OF_TRACK){
		return Event(type);
	/*
	Everything below this point isn't actually implemented in the player
	*/
	} else if (type == Event::SET_VARIABLE){
		i+=3;
		return event_u8_s16(type, data);
	} else if (type == Event::SUSTAIN){
		++i;
		return Event(type, d[i]);
	} else if (type == Event::SET_VARIABLE_RANDOM){
		i+=3;
		return event_u8_s16(type, data);
	} else if (type == Event::COMPARE_LE){
		i+=3;
		return event_u8_s16(type, data);
	} else if (type == Event::IF){
//		read_event(d, i);
		return Event(type);
	} else if (type == Event::TIE){
		++i;
		return event_u8(type, data);
	} else if (type == Event::RANDOM_RANGE){
//		read_event(d, i); //need to parse arg, but remove last argument...
		read_event(d, i);
		int lower = binary_as_short(&d[i+0]);
		int upper = binary_as_short(&d[i+2]);
		i+=4;
		return Event(type, lower, upper);
	} else {
		std::cout << "Unknown type " << as_hex(type) << " at location " << as_hex(i-1) << std::endl;
		throw std::runtime_error{"Unknown type!"};
	}
	
}

void SSEQ::open(std::string filename){
	std::ifstream is{filename, std::ios::binary | std::ios::in};
	std::vector<char> data;
	data.resize(2*1024*1024);
	is.read(data.data(), 2*1024*1024);
	
//	channels.resize(16);
	
	start_offset = binary_as_int(&data[START_OFFSET]);
	int filesize = 0x10 + binary_as_int(&data[SIZE]);
	int i = start_offset;
	
	while (i < filesize){
		event_location.push_back(i - start_offset);
		Event event = read_event(data, i);
		events.push_back(event);
		if (event.type == Event::RANDOM_RANGE){
//			int seq_command_loc = 1 + event_location.back();
			std::cout << "WARNING: all events beyond event at " << as_hex(event_location.back()) << " will be wrong" << std::endl;
		}
	}
}

bool is_digit(char character){
	return '0' <= character && character <= '9';
}

// Read one digit.
int read_digit(std::string data, int& index){
	int digit = data[index] - '0';
	index++;
	return digit;
}

// Reads a number of variable length (1-3 digits)
int read_number(std::string data, int& index){
	int old_index = index;
	while (is_digit(data[index])){
		index++;
	}
	return std::stoi(data.substr(old_index, index-old_index));
}

int read_dots(std::string data, int& index){
	int dots = 0;
	while (data[index] == '.'){
		index++;
		dots++;
	}
	return dots;
}

void SSEQ::mml(std::string mml){
	int index = 0;
	int channel = 0;
	int octave = 4; //default
	int length = 4;
	int velocity = 127;
	const std::string NOTES = "CCDDEFFGGAAB";
	const std::string SEMITONE = "-#+"; 
	std::vector<Event> channel_defines{};
	std::vector<std::vector<Event>> channels{8, std::vector<Event>{}};
	std::vector<int> loop_starts{};
	
	std::map<std::string, std::string> macros;
	bool is_tied = false;
	int gate_time = 8;
	
	while (index < (int)mml.length()){
		char cmd = mml[index];
		index++;
		auto& track = channels[channel];
		
		if (cmd == ':'){
			if (channel_defines.empty()){
				channel_defines.emplace_back(Event::TRACKS_ENABLED, 0x01);
			}
			//Channel select
			channel = read_digit(mml, index);
			channel_defines[0].value1 |= 1<<channel;
			
			if (channel > 0){
				channel_defines.emplace_back(Event::DEFINE_TRACK, channel, -1); // Offset will be calculated later
			}
			length = 4;
			velocity = 127;
			octave = 4;
		} else if (cmd == 'T'){
			int tempo = read_number(mml, index);
			track.emplace_back(Event::TEMPO, tempo);
		} else if (cmd == 'L'){
			length = read_number(mml, index);
			// this is not an event, but defines the length value of future note events
		} else if (std::string::npos != NOTES.find(cmd)) {
			int note = (octave + 1) * 12 + NOTES.find(cmd); //Might be off by an octave??
			if (index < (int)mml.length() && SEMITONE.find(mml[index]) != std::string::npos){
				int accidental = mml[index];
				index++;
				note += accidental == '-' ? -1 : 1;
			}
			int note_length = length;
			if (is_digit(mml[index])){
				note_length = read_number(mml, index);
			}
			note_length *= 2-std::pow(0.5, read_dots(mml, index));
			
			track.emplace_back(note, velocity, 192 / note_length * gate_time / 8);
			track.emplace_back(Event::REST, 192 / note_length);
			if (is_tied){
				track.emplace_back(Event::TIE, 0);
				is_tied = false;
			}
			
		} else if (cmd == '<'){
			octave++;
		} else if (cmd == '>'){
			octave--;
		} else if (cmd == 'R'){
			int note_length = length;
			if (is_digit(mml[index])){
				note_length = read_number(mml, index);
			}
			note_length *= 2-std::pow(0.5, read_dots(mml, index));
			
			track.emplace_back(Event::REST, 192 / note_length);
		} else if (cmd == 'N'){
			int note = read_number(mml, index);
			
			track.emplace_back(note, velocity, 192 / length * gate_time / 8);
			track.emplace_back(Event::REST, 192 / length);
		} else if (cmd == 'O'){
			octave = read_digit(mml, index);
		} else if (cmd == 'P'){
			int pan = read_number(mml, index);
			
			track.emplace_back(Event::PAN, pan);
		} else if (cmd == 'V'){
			velocity = read_number(mml, index);
		} else if (cmd == '@'){
			if (is_digit(mml[index])){
				int instrument = read_number(mml, index);
				track.emplace_back(Event::BANK, instrument, 0); //MML uses bank 0
			} else {
				char cmd2 = mml[index];
				index++;
				// command is one of:
				// @D, @E, @ER, @V, @MON, @MOF, @MA, @MP
				if (cmd2 == 'V'){
					int volume = read_number(mml, index);
					track.emplace_back(Event::VOLUME, volume);
				} else {
					std::cout << cmd << cmd2 << "* is unimplemented!" << std::endl;
				}
			}
		} else if (cmd == '[') {
			loop_starts.push_back(track.size()); //index of start event
			track.emplace_back(Event::LOOP_START, -1); // will be replaced when endpoint is found
		} else if (cmd == ']') {
			int loop_length = 0; // default is zero (infinite)
			if (is_digit(mml[index])){
				loop_length = read_number(mml, index);
			}
			
			int index = loop_starts.back();
			loop_starts.pop_back();
			track[index].value1 = loop_length;
			track.emplace_back(Event::LOOP_END);
		} else if (cmd == '{'){
			auto old_index = index;
			std::string label;
			std::string macro;
			char c = mml[index];
			bool is_new = false;
			int nest = 1;
			while (nest>=1){
				if (c == '{') nest++;
				if (c == '}') nest--;
				if (nest == 0){
					break;
				}
				
				is_new |= c == '=';
				if (!is_new)
					label += c;
				else if (c != '=')
					macro += c;
				++index;
				c = mml[index];
			}
			
			if (is_new){
				macros.insert({label, macro});
			} else {
				std::cout << mml << "\n";
				mml.replace(old_index-1, label.size()+2, macros.at(label));
				index = old_index-1;
				std::cout << mml << std::endl;
			}
		} else if (cmd == '&'){
			// set tie ON / after next note, disable it?
			track.emplace_back(Event::TIE, 1);
			is_tied	= true;
		} else if (cmd == 'Q'){
			// not a sequence command?
			gate_time = read_digit(mml, index);
		}
	}
	
//	events.resize(std::accumulate(channels.begin(), channels.end(), channel_defines.size(), [](auto& a, auto& b){ return b.size() + a;}));
	std::copy(channel_defines.begin(), channel_defines.end(), std::back_inserter(events));
	
	for (int i = 0; i < 8; ++i){
		auto& c = channels[i];
		if (!c.empty()){
			int offset = events.size();
			int j = 1;
			if (i > 0){ // Assumes DEFINE_TRACK exists, since channels >0 must be explicitly defined
				while (i != events[j].value1){
					j++;
				}
				events[j].value2 = offset;
			}
			
			std::copy(c.begin(), c.end(), std::back_inserter(events));
			events.emplace_back(Event::END_OF_TRACK);
		}
	}
	// locations for MML can just be indexes as there is no SSEQ file defined with fixed offsets
	event_location.resize(events.size());
	std::iota(event_location.begin(), event_location.end(), 0);
	
	std::cout << info();
}

std::string SSEQ::info(){
	std::string info{};
//	for (auto& channel : channels){
//		if (channel.enabled){
//			info += channel.info();
//		}
//	}
	for (std::size_t i = 0; i < events.size(); ++i){
		info += " " + as_hex(event_location[i]) + " " + events[i].info();
	}
	return info;
}

std::string Event::info(){
	std::string info{};
	const std::vector<std::string> notes = {"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
	if (type == Event::PAN){
		info += " Pan " + std::to_string(value1);
	} else if (type == Event::VOLUME){
		info += " Volume " + std::to_string(value1);
	} else if (type == Event::BANK){
		info += " Bank [Prog.No:" + as_hex(value1) + ", Bank:" + std::to_string(value2) + "]";
	} else if (Event::NOTE_LOW <= type && type <= Event::NOTE_HIGH){
		info += " Note O" + std::to_string(type / 12 - 1) + notes.at(type % 12) + " [Velocity: " + std::to_string(value1) + ", Duration: " + std::to_string(value2) + "]";
	} else if (type == Event::REST){
		info += " Rest [Duration:" + std::to_string(value1) + "]";
	} else if (type == Event::PITCH_BEND){
		info += " Pitch bend [x/128?:" + std::to_string(value1) + "]";
	} else if (type == Event::PITCH_BEND_RANGE){
		info += " Pitch bend Range [Semitones?:" + std::to_string(value1) + "]";
	} else if (type == Event::MONO_POLY){
		info += " Mono? [" + as_hex(value1) + "]";
	} else if (type == Event::CALL){
		info += " Call [Offset:" + as_hex(value1) + "]";
	} else if (type == Event::JUMP){
		info += " Jump [Offset:" + as_hex(value1) + "]";
	} else if (type == Event::RETURN){
		info += " Return";
	} else if (type == Event::END_OF_TRACK){
		info += " End Track";
	} else if (type == Event::TEMPO){
		info += " Tempo [BPM:" + std::to_string(value1)+ "]";
	} else if (type == Event::DEFINE_TRACK){
		info += " Define track [Channel:" + std::to_string(value1) + " Offset:" + as_hex(value2) + "]";
	} else if (type == Event::TRACKS_ENABLED){
		info += " Tracks enabled: " + as_hex(value1);
	} else {
		info += " Type " + as_hex(type);
		info += " Data " + as_hex(value1) + ", " + as_hex(value2);
	}
	return info + "\n";
}
