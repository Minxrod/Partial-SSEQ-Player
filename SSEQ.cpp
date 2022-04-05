//useful reference:
//https://gota7.github.io/NitroStudio2/specs/sequence.html

#include "SSEQ.hpp"

#include "binhelper.hpp"

#include <fstream>
#include <iostream>

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
	
	channels.resize(16);
	
	int start_offset = binary_as_int(&data[START_OFFSET]);
	int filesize = 0x10 + binary_as_int(&data[SIZE]);
	int i = start_offset;
	
	while (i < filesize){
		unsigned char event = static_cast<unsigned char>(data[i]);
		event_location.push_back(i);
		++i; //data of event's index (or next event, if no data needed)
		
		if (event == Event::TRACKS_ENABLED){
			int tracks = binary_as_ushort(&data[i]);
//			std::cout << "track bitfield = " << tracks << std::endl;
			for (int b = 0; b < 16; ++b){
				if (tracks & (1<<b)){
					channels[b].id = b;
//					channels[b].enabled = true; //channel 0 is not defined well??
				}
			}
			i+=2; //
			events.emplace_back(event);
			events.back().value1 = tracks;
		} else if (event == Event::DEFINE_TRACK){
			int track = data[i];
//			++i;
			int offset = start_offset + binary_as_u24(&data[i+1]);
			//+ variable_length(i);
			
			channels[track].offset = offset;
			channels[track].enabled = true;
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
			events.back().value1 = data[i];
			events.back().value2 = data[i+1];
			i+=2;
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

std::string SSEQ::info(){
	std::string info{};
	for (auto& channel : channels){
		if (channel.enabled){
			info += channel.info();
		}
	}
	for (std::size_t i = 0; i < events.size(); ++i){
		info += " " + as_hex(event_location[i]) + " " + events[i].info();
	}
	return info;
}

std::string Channel::info(){
	std::string info{};
	info += "Channel " + std::to_string(id) + ": (Offset: " + as_hex(offset) + ")\n"; 
/*	for (auto& event : events){
		info += event.info();
	}*/
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
	} else {
		info += " Type " + as_hex(type);
		info += " Data " + as_hex(value1) + ", " + as_hex(value2);
	}
	return info + "\n";
}
