#include "SBNK.hpp"

#include <fstream>

#include "binhelper.hpp"

void SBNK::open(std::string filename){
	std::ifstream is{filename, std::ios::binary | std::ios::in};
	data.resize(2*1024*1024);
	is.read(data.data(), 2*1024*1024);
	
	int count = binary_as_int(&data[INSTRUMENT_COUNT]);
	
	instruments.resize(count);
	for (int i = 0; i < count * 4; i += 4){
		int ofs = binary_as_ushort(&data[INSTRUMENT_RECORDS_START + i + INSTRUMENT_OFFSET]);
		offsets.push_back(ofs);
		instruments[i/4].f_record = data[INSTRUMENT_RECORDS_START + i + INSTRUMENT_F_RECORD];
	}
	
	for (int i = 0; i < count; ++i){
		instruments[i].id = i;
		instruments[i].read(&data[offsets[i]]);
	}
}

void Instrument::read(char* data){
	if (f_record == F_RECORD_EMPTY)
		return; // no data
	if (f_record == F_RECORD_PCM || f_record == F_RECORD_PSG){
		notes.resize(1);
		notes[0].read(data);
	} else if (f_record == F_RECORD_RANGE){
		note_numbers = std::vector<int>{data[0], data[1]};
		
		notes.resize(data[1]-data[0]+1);
		for (int i = 0; i <= data[1]-data[0]; ++i){
			notes[i].read(&data[4+12*i]);
		}
	} else if (f_record == F_RECORD_REGIONAL){
		note_numbers = std::vector<int>{};
		
		int i = 0;
		do {
			note_numbers.push_back(data[i]);
			++i;
		} while (note_numbers.back() != 0x7f);
		i = 8 + 2;
		notes.resize(note_numbers.size());
		for (std::size_t j = 0; j < note_numbers.size(); ++j){
			notes[j].read(&data[i+12*j]);
		}
	}
}

std::string Instrument::info(){
	std::string info{};
	info += " id=" + std::to_string(id);
	info += " fRecord="+std::to_string(f_record);
	if (f_record == F_RECORD_PCM){
		info += notes[0].info();
	} else if (f_record == F_RECORD_PSG){
		info += " PSG duty " + std::to_string((notes[0].swav_no + 1)* 100.0 / 8.0) + "%";
		info += notes[0].info();
	} else if (f_record == F_RECORD_NOISE){
		info += " Noise";
	} else if (f_record == F_RECORD_RANGE){
		info += " Range[" + std::to_string(note_numbers[0]) + "," + std::to_string(note_numbers[1]) + "]\n";
		for (int i = 0; i <= note_numbers[1]-note_numbers[0]; ++i){
			info += "\t N=" + std::to_string(i+note_numbers[0]) + " Info=(" + notes[i].info() + ")\n";
		}
	} else if (f_record == F_RECORD_REGIONAL){
		info += " Regional\n";
		for (std::size_t i = 0; i < note_numbers.size(); ++i){
			info += "\t Notes[" + std::to_string(i==0 ? 0 : note_numbers[i-1]) + "," + std::to_string(note_numbers[i]) + "]";
			info += " Info=(" + notes[i].info() + ")\n";
		}
	}
	return info;
}

NoteDefinition& Instrument::get_note_def(int note){
	if (f_record == F_RECORD_REGIONAL){
		int region = 0;
		while (note >= note_numbers[region]){
			region++;
		}
		return notes[region];
	} else if (f_record == F_RECORD_RANGE){
		return notes[note];
	} else if (f_record == F_RECORD_EMPTY){
		throw std::runtime_error{"Instrument::get_note_def: Hit invalid type F_RECORD_EMPTY"};
	} else {
		throw std::runtime_error{"Instrument::get_note_def: unimplemented type"};
	}
}

void NoteDefinition::read(char* data){
	swav_no = binary_as_ushort(&data[SWAV_NUMBER]);
	swar_no = binary_as_ushort(&data[SWAR_NUMBER]);
	note = data[NOTE];
	attack = data[ATTACK];
	decay = data[DECAY];
	sustain = data[SUSTAIN];
	release = data[RELEASE];
	pan = data[PAN];
}

std::string NoteDefinition::info(){
	std::string info{};
	info += " SWAV=" + std::to_string(swav_no);
	info += " SWAR=" + std::to_string(swar_no);
	info += " Note=" + std::to_string(note);
	info += " A=" + std::to_string(attack);
	info += " D=" + std::to_string(decay);
	info += " S=" + std::to_string(sustain);
	info += " R=" + std::to_string(release);
	info += " Pan=" + std::to_string(pan);
	return info;
}
