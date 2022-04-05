#include "SSEQStream.h"
#include "binhelper.hpp"

#include <iostream>
#include <istream>
#include <fstream>
#include <algorithm>
#include <cmath>


int SSEQStream::index_from_offset(int offset){
	return std::distance(sseq.event_location.begin(), std::find(sseq.event_location.begin(), sseq.event_location.end(), offset));
}

SSEQStream::SSEQStream(SWAR& wave, SBNK& bank, SSEQ& sequence) : swar(wave), sbnk(bank), sseq(sequence){
	samples1.resize(50000); //arbitrary
	samples2.resize(50000);
	initialize(1, PLAYBACK_SAMPLE_RATE);
	tempo = sseq.tempo;
	
	for (auto& c : sseq.channels){
		c.current_index = index_from_offset(c.offset);
		c.instr = 0;
		std::cout << c.enabled << " " << c.id << " -> " << c.current_index << std::endl;
		c.next_process_delay = 0;
	}
	//enable channel zero
	sseq.channels[0].current_index = 0;
	sseq.channels[0].enabled = true; //needed for SSEQ_5
}

//returns duration in terms of samplerate, not ticks
//60 = sec/min
int duration(double tempo, double ticks){
	return 22050.0 * 60.0 * ticks / 48.0 / tempo;
}

short calculate_sample(int sample, int velocity){
	return sample * velocity / 128;
}

//void SSEQStream::process_event(Channel& channel, Event& event){
////	channel.max_samples = -1;
////	channel.current_sample = 0;
//	if (event.type <= Event::NOTE_HIGH){
//		channel.max_samples = duration(tempo, event.value2);
//		channel.current_sample = 0;
//		channel.current_note_event = &event;
//	} else if (event.type == Event::REST) {
//		channel.next_process_delay = duration(tempo, event.value1);
////		channel.max_samples = duration(tempo, event.value1);
//		std::cout << "Delay " << channel.next_process_delay << " / ";
//	}
//	std::cout << as_hex(channel.current_index) << ": " << event.info() << " " << channel.current_sample << std::endl;
//}

bool SSEQStream::onGetData(Chunk& chunk){
	chunk.sampleCount = 50000;
	
	which = !which;
	auto& samples = which ? samples1 : samples2;
	
	int added_samples = 0;
	for (auto& channel : sseq.channels){
		added_samples += channel.enabled ? 1 : 0;
	}
	
	for (std::size_t s = offset; s < offset + chunk.sampleCount; ++s){
		int partial_sample = 0;
//		int added_samples = ;
		samples[s-offset] = 0; //Clear before next iteration
		for (auto& channel : sseq.channels){
			if (!channel.enabled)
				continue;
			if (!(1<<channel.id & (1<<11 | 0xffff)))
				continue;
			bool instant_event;
			channel.next_process_delay--;
			do {
				instant_event = false;
				if (channel.next_process_delay > 0)
					break;
					
				auto& event = sseq.events.at(channel.current_index);
				if (event.type == Event::BANK){
					instant_event = true;
					channel.instr = static_cast<unsigned char>(event.value2 * (128+event.value1)) % 128;
					std::cout << channel.id << " " << event.value1 << " " << event.value2 << std::endl;
				} else if (event.type == Event::JUMP) {
					instant_event = true;
					channel.current_index = index_from_offset(event.value1);
				} else if (event.type == Event::CALL) {
					instant_event = true;
					channel.call_stack.push_back(channel.current_index);
					channel.current_index = index_from_offset(event.value1);
				} else if (event.type == Event::RETURN) {
					instant_event = true;
					channel.current_index = channel.call_stack.back()+1;
					channel.call_stack.pop_back();
				} else if (event.type <= Event::NOTE_HIGH) {
					instant_event = true;
					if (channel.current_sample >= channel.max_samples){
						channel.max_samples = duration(tempo, event.value2);
						channel.current_sample = 0;
						channel.current_note_event = &event;
					}
				} else if (event.type == Event::REST) {
					channel.next_process_delay = duration(tempo, event.value1);
				} else if (event.type == Event::TEMPO) {
					tempo = event.value1; //hmmm
				} else if (event.type == Event::END_OF_TRACK){
					channel.enabled = false; // this channel now disabled (done playing)
				}
				else {
//					instant_event = true;
					// Displays unimplemented events
					//std::cout << "UNIMPLEMENTED: " << event.info() << std::endl;
				}
				if (event.type != Event::JUMP && event.type != Event::CALL && event.type != Event::RETURN) {
					channel.current_index++;
				} else {
					// We have already set the next event index, don't want to skip this event
				}
				
//				std::cout << as_hex(channel.current_index) << ": " << event.info() << " " << channel.current_sample << std::endl;
			} while (instant_event);
			
			channel.current_sample++;
//			auto& event = sseq.events[channel.current_index];
			
			if (channel.current_sample < channel.max_samples){
				partial_sample += get_sample(channel.instr, channel.current_note_event->type, channel.current_sample);
//				added_samples++;
			}
		}
		if (partial_sample){
			//doing this based on https://dsp.stackexchange.com/questions/3581/algorithms-to-mix-audio-signals-without-clipping
			//It seems to work well enough?
			samples[s-offset] = partial_sample/added_samples;
		}
	}
	std::cout << "EOM" << std::endl;
	
	chunk.samples = samples.data();
	offset += 50000;
	
	// only continue if channels are still going
	for (auto& channel : sseq.channels){
		if (channel.enabled)
			return true;
	}
	return false;
}

short SSEQStream::get_sample(int instrument, int note, std::size_t index){
	auto& note_def = sbnk.instruments[instrument].get_note_def(note);
	auto& waveform = swar.swav[note_def.swav_no];
	double note_shift = std::pow(2, (note - note_def.note)/12.0);
	note_shift *= (static_cast<double>(waveform.sampleRate) / static_cast<double>(PLAYBACK_SAMPLE_RATE));
	
	std::size_t actual_index = note_shift * index;
	if (actual_index < waveform.samples.size()){
		return waveform.samples[actual_index];
	}
	if (waveform.loopSupport){
		actual_index -= waveform.loopStart;
		actual_index %= waveform.loopLength;
		actual_index += waveform.loopStart;
		return waveform.samples[actual_index];
	} else {
		return 0;
	}
}


// Does nothing, because it isn't needed.
void SSEQStream::onSeek(sf::Time){}

/*
 * Random testing stuff below!
 */

void SampleStream::init(std::vector<short>* dat, unsigned int sampleRate, unsigned int loopStart){
	initialize((unsigned int)1, (unsigned int)sampleRate);
	d = dat;
	ls = loopStart;
}

SWAVStream::SWAVStream(SWAV& swav) : wave{swav} {
	initialize((unsigned int)1, (unsigned int)wave.sampleRate);
}

InstrumentStream::InstrumentStream(SWAV& swav, SBNK& sbnk) : wave{swav}, bank{sbnk} {
	initialize((unsigned int)1, (unsigned int)wave.sampleRate);
}

bool InstrumentStream::onGetData(Chunk& c){
//	c.sampleCount = wave.samples.size();	
//	c.samples = wave.samples.data();
	c.sampleCount = 22050*10;
	samples.resize(c.sampleCount);
	std::copy_n(wave.samples.begin(), wave.samples.size(), samples.begin());
	if (wave.loopSupport){
		for (std::size_t ofs = wave.loopStart; ofs < samples.size(); ofs += wave.loopLength){
			for (int i = 0; i < wave.loopLength && ofs+i < samples.size(); ++i){
				samples[ofs+i] = wave.samples[wave.loopStart+i];
			}
		}
	}
	
	c.samples = samples.data();
	
	return true;
}


std::vector<short>& SWAVStream::get_samples(){
	return samples;
}


bool SWAVStream::onGetData(Chunk& c){
//	c.sampleCount = wave.samples.size();	
//	c.samples = wave.samples.data();
	c.sampleCount = 22050*10;
	samples.resize(c.sampleCount);
	std::copy_n(wave.samples.begin(), wave.samples.size(), samples.begin());
	if (wave.loopSupport){
		for (std::size_t ofs = wave.loopStart; ofs < samples.size(); ofs += wave.loopLength){
			for (int i = 0; i < wave.loopLength && ofs+i < samples.size(); ++i){
				samples[ofs+i] = wave.samples[wave.loopStart+i];
			}
		}
	}	
	c.samples = samples.data();
	
	return true;
}

bool SampleStream::onGetData(Chunk&){
	return false;
}

void InstrumentStream::onSeek(sf::Time){}
void SWAVStream::onSeek(sf::Time){}
void SampleStream::onSeek(sf::Time){}
