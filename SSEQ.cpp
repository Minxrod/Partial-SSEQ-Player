//useful reference:
//https://gota7.github.io/NitroStudio2/specs/sequence.html

#include "SSEQ.hpp"

#include "binhelper.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <numeric>

const unsigned char 
	Event::REST,
	Event::BANK,
	Event::ENABLED_TRACKS,
	Event::DEFINE_TRACK,
	Event::TEMPO,
	Event::PAN,
	Event::VOLUME,
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
int SSEQ::variable_length(int& i){
	int value = 0;
	do { 
		value <<= 7;
		value |= data[i] & 0x7f;
//		if (data[i] < 0){
//		}
		++i;
	} while (data[i-1] < 0);
	return value;
}

void SSEQ::open(std::string filename){
	std::ifstream is{filename, std::ios::binary | std::ios::in};
	data.resize(2*1024*1024);
	is.read(data.data(), 2*1024*1024);
	
//	channels.resize(16);
	
	int start_offset = binary_as_int(&data[START_OFFSET]);
	int filesize = 0x10 + binary_as_int(&data[SIZE]);
	int i = start_offset;
	
	while (i < filesize){
		unsigned char event = static_cast<unsigned char>(data[i]);
		event_location.push_back(i);
		++i; //data of event's index (or next event, if no data needed)
		
		if (event == Event::TRACKS_ENABLED){
			int tracks = binary_as_ushort(&data[i]);
//			for (int b = 0; b < 16; ++b){
//				if (tracks & (1<<b)){
//					channels[b].id = b;
//					channels[b].enabled = true; //channel 0 is not defined well??
//				}
//			}
			i+=2;
			events.emplace_back(event);
			events.back().value1 = tracks;
		} else if (event == Event::DEFINE_TRACK){
			int track = data[i];
			int offset = start_offset + binary_as_u24(&data[i+1]);
			
//			channels[track].offset = offset;
//			channels[track].enabled = true;
			i+=4;
			events.emplace_back(event);
			events.back().value1 = track;
			events.back().value2 = offset;
		} else if (event == Event::MONO_POLY){
			events.emplace_back(event);
			events.back().value1 = data[i];
			++i;
		} else if (event == Event::TEMPO){
			events.emplace_back(event);
			events.back().value1 = binary_as_ushort(&data[i]);
			tempo = events.back().value1;
			i+=2;
		} else if (event == Event::VOLUME){
			events.push_back(Event(event));
			events.back().value1 = data[i];
			++i;
		} else if (event == Event::PAN){
			events.push_back(Event(event));
			events.back().value1 = data[i];
			++i;
		} else if (event == Event::BANK){
			events.push_back(Event(event));
			int instr_bank = variable_length(i);
			events.back().value1 = instr_bank & 0x00ff; //instrument
			events.back().value2 = (instr_bank & 0x3f00) >> 8; //bank
		} else if (Event::NOTE_LOW <= event && event <= Event::NOTE_HIGH){
			events.push_back(Event(event));
			events.back().value1 = static_cast<unsigned char>(data[i]);
			++i;
			events.back().value2 = variable_length(i);
		} else if (event == Event::REST){
			events.push_back(Event(event));
			events.back().value1 = variable_length(i);
		} else if (event == Event::PITCH_BEND){
			events.push_back(Event(event));
			events.back().value1 = data[i];
			++i;
		} else if (event == Event::PITCH_BEND_RANGE){
			events.push_back(Event(event));
			events.back().value1 = data[i];
			++i;
		} else if (event == Event::PITCH_BEND_RANGE){
			events.push_back(Event(event));
			events.back().value1 = data[i];
			++i;
		} else if (event == Event::MODULATION_DEPTH){
			events.emplace_back(event);
			events.back().value1 = data[i];
			++i;
		} else if (event == Event::MODULATION_SPEED){
			events.emplace_back(event);
			events.back().value1 = data[i];
			++i;
		} else if (event == Event::MODULATION_TYPE){
			events.emplace_back(event);
			events.back().value1 = data[i];
			++i;
		} else if (event == Event::MODULATION_RANGE){
			events.emplace_back(event);
			events.back().value1 = data[i];
			++i;
		} else if (event == Event::PRIORITY){
			events.emplace_back(event);
			events.back().value1 = data[i];
			++i;
		} else if (event == Event::VOLUME_2){
			events.emplace_back(event);
			events.back().value1 = data[i];
			++i;
		} else if (event == Event::CALL){
			events.push_back(Event(event));
			events.back().value1 = binary_as_u24(&data[i]) + start_offset;
			 //offset is relative to start of sequence data (same for jump)
			i+=3;
		} else if (event == Event::JUMP){
			events.emplace_back(event);
			events.back().value1 = binary_as_u24(&data[i]) + start_offset;
			i+=3;
		} else if (event == Event::RETURN){
			events.emplace_back(event);
		} else if (event == Event::END_OF_TRACK){
			events.emplace_back(event);
		} else {
			std::cout << "Unknown type " << as_hex(event) << " at location " << as_hex(i-1) << std::endl;
			break; //early exit for unknown type
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
	
	while (index < (int)mml.length()){
		std::string cmd = std::string{mml[index]};
		index++;
		
		if (cmd == ":"){
			if (channel_defines.empty()){
				channel_defines.emplace_back(Event::TRACKS_ENABLED);
				channel_defines.back().value1 = 0x01;
			}
			//Channel select
			channel = read_digit(mml, index);
			channel_defines[0].value1 |= 1<<channel;
			
			channel_defines.emplace_back(Event::DEFINE_TRACK);
			channel_defines.back().value1 = channel;
			channel_defines.back().value2 = 0;
			
			length = 4;
			velocity = 127;
			octave = 4;
		} else if (cmd == "T"){
			int tempo = read_number(mml, index);
			
			channels[channel].emplace_back(Event::TEMPO);
			channels[channel].back().value1 = tempo;
		} else if (cmd == "L"){
			length = read_number(mml, index);
			// this is not an event, but defines the length value of future note events
		} else if (std::string::npos != NOTES.find(cmd)) {
			int note = octave * 12 + NOTES.find(cmd);
			if (index < (int)mml.length() && SEMITONE.find(mml[index]) != std::string::npos){
				int accidental = mml[index];
				index++;
				note += accidental == '-' ? -1 : 1;
			}
			int note_length = length;
			if (is_digit(mml[index])){
				note_length = read_number(mml, index);
			}
			//TODO: Add check for dots (ex. "C4D2.C4D2.")
			
			channels[channel].emplace_back(note);
			channels[channel].back().value1 = velocity;
			channels[channel].back().value2 = 192 / note_length;
			channels[channel].emplace_back(Event::REST);
			channels[channel].back().value1 = 192 / note_length;
		} else if (cmd == "<"){
			octave++;
		} else if (cmd == ">"){
			octave--;
		} else if (cmd == "R"){
			int note_length = length;
			if (is_digit(mml[index])){
				note_length = read_number(mml, index);
			}
			
			channels[channel].emplace_back(Event::REST);
			channels[channel].back().value1 = 192 / note_length;
		} else if (cmd == "N"){
			int note = read_number(mml, index);
			
			channels[channel].emplace_back(note);
			channels[channel].back().value1 = velocity;
			channels[channel].back().value2 = 192 / length;
			channels[channel].emplace_back(Event::REST);
			channels[channel].back().value1 = 192 / length;
		} else if (cmd == "O"){
			octave = read_digit(mml, index);
		} else if (cmd == "P"){
			int pan = read_number(mml, index);
			
			channels[channel].emplace_back(Event::PAN);
			channels[channel].back().value1 = pan;
		} else if (cmd == "V"){
			velocity = read_number(mml, index);
		}
	}
	
//	events.resize(std::accumulate(channels.begin(), channels.end(), channel_defines.size(), [](auto& a, auto& b){ return b.size() + a;}));
	std::copy(channel_defines.begin(), channel_defines.end(), std::back_inserter(events));
	
	for (int i = 0; i < 8; ++i){
		auto& c = channels[i];
		if (!c.empty()){
			int offset = events.size();
			int j = 1;
			while (i != events[j].value1){ // Assumes DEFINE_TRACK exists
				j++;
			}
			events[j].value2 = offset;
			
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
		info += " Note O" + std::to_string(type / 12) + notes.at(type % 12) + " [Velocity: " + std::to_string(value1) + ", Duration: " + std::to_string(value2) + "]";
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
	} else {
		info += " Type " + as_hex(type);
		info += " Data " + as_hex(value1) + ", " + as_hex(value2);
	}
	return info + "\n";
}
