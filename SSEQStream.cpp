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
	samples1.resize(50000*2); //arbitrary
	samples2.resize(50000*2);
	initialize(2, PLAYBACK_SAMPLE_RATE);
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
	return SSEQStream::PLAYBACK_SAMPLE_RATE * 60.0 * ticks / 48.0 / tempo;
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
	chunk.sampleCount = 50000*2;
	
//	which = !which;
	auto& samples = which ? samples1 : samples2;
	
	int added_samples = 0;
	for (auto& channel : sseq.channels){
		added_samples += channel.enabled ? 1 : 0;
	}
	
	for (std::size_t s = offset; s < offset + (unsigned)50000; ++s){
		int partial_sample = 0;
		int left_sample = 0;
		int right_sample = 0;
//		samples[s-offset] = 0; //Clear before next iteration
		for (auto& channel : sseq.channels){
			if (!channel.enabled)
				continue;
			if (!(1<<channel.id & (1<<1 | 0xffff)))
				continue;
			bool instant_event;
			channel.next_process_delay--;
			do {
				instant_event = true;
				if (channel.next_process_delay > 0)
					break;
					
				auto& event = sseq.events.at(channel.current_index);
				if (event.type == Event::BANK){
					channel.instr = (static_cast<unsigned char>(event.value1) % 128) + 256 * event.value2;
					std::cout << channel.id << " " << event.value1 << " " << event.value2 << std::endl;
				} else if (event.type == Event::JUMP) {
					channel.current_index = index_from_offset(event.value1);
				} else if (event.type == Event::CALL) {
					channel.call_stack.push_back(channel.current_index);
					channel.current_index = index_from_offset(event.value1);
				} else if (event.type == Event::RETURN) {
					channel.current_index = channel.call_stack.back()+1;
					channel.call_stack.pop_back();
				} else if (event.type <= Event::NOTE_HIGH) {
					channel.max_samples = duration(tempo, event.value2);
					channel.current_sample = 0;
					channel.current_note_event = &event;
					channel.phase = Channel::PHASE_ATTACK;
					channel.amplitude = Channel::ADSR_MINIMUM;
				} else if (event.type == Event::REST) {
					instant_event = false;
					channel.next_process_delay = duration(tempo, event.value1);
				} else if (event.type == Event::TEMPO) {
					tempo = event.value1; //hmmm
				} else if (event.type == Event::END_OF_TRACK){
					instant_event = false; // otherwise, continues to read events (sometimes past valid end)
					channel.enabled = false; // this channel now disabled (done playing)
				} else if (event.type == Event::PITCH_BEND_RANGE){
					channel.pitch_bend_range = event.value1;
				} else if (event.type == Event::PITCH_BEND){
					channel.pitch_bend = event.value1;
				} else if (event.type == Event::VOLUME){
					channel.volume = event.value1;
				} else if (event.type == Event::PAN){
					channel.pan = event.value1;
				} else {
					instant_event = false;
					// Displays unimplemented events
					std::cout << "UNIMPLEMENTED: " << event.info() << std::endl;
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
			
			if (channel.phase != Channel::PHASE_NONE){
				short sample = get_sample(channel);
				sample = apply_adsr(channel, sample) * (static_cast<double>(channel.volume) / 128);
				partial_sample = sample;
				
				int pan = channel.pan == 127 ? 128 : channel.pan;
				left_sample += partial_sample * (static_cast<double>(128 - pan)/128.0);
				right_sample += partial_sample * (static_cast<double>(pan)/128.0);
			}
		}
		samples[2*(s-offset)] = 0;
		samples[2*(s-offset)+1] = 0;
		if (left_sample | right_sample){
			//doing this based on https://dsp.stackexchange.com/questions/3581/algorithms-to-mix-audio-signals-without-clipping
			//It seems to work well enough?
			samples[2*(s-offset)] = left_sample/added_samples;
			samples[2*(s-offset)+1] = right_sample/added_samples;
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

short SSEQStream::apply_adsr(Channel& channel, short sample){
	auto& instr = sbnk.instruments[channel.instr];
	auto& note_def = instr.get_note_def(channel.current_note_event->type);
	
	if (channel.phase == Channel::PHASE_ATTACK){
		channel.amplitude = note_def.attack * channel.amplitude / 255;
		if (channel.amplitude >= Channel::ADSR_MAXIMUM){
			channel.amplitude = Channel::ADSR_MAXIMUM;
			channel.phase = Channel::PHASE_DECAY;
		}
	} else if (channel.phase == Channel::PHASE_DECAY) {
		channel.amplitude -= note_def.decay;
		double level = 127 * static_cast<double>(Channel::ADSR_MINIMUM - channel.amplitude) / Channel::ADSR_MINIMUM;
		if (level <= note_def.sustain){
			channel.amplitude = (127 - note_def.sustain) * Channel::ADSR_MINIMUM;
			channel.phase = Channel::PHASE_SUSTAIN;
		}
	} else if (channel.phase == Channel::PHASE_SUSTAIN){
		if (channel.current_sample > channel.max_samples){
			channel.phase = Channel::PHASE_RELEASE;
		}
	} else if (channel.phase == Channel::PHASE_RELEASE){
		channel.amplitude -= note_def.release;
		if (channel.amplitude <= Channel::ADSR_MINIMUM){
			channel.amplitude = Channel::ADSR_MINIMUM;
			channel.phase = Channel::PHASE_NONE;
		}
	}
	
	return sample * static_cast<double>(Channel::ADSR_MINIMUM - channel.amplitude) / Channel::ADSR_MINIMUM;
}

short SSEQStream::get_sample(Channel& channel){	
	static int current_noise = 0x7fff;
	
	int note = channel.current_note_event->type;
	std::size_t index = channel.current_sample;
	
	auto& instr = sbnk.instruments[channel.instr];
	auto& note_def = instr.get_note_def(note);
	double note_shift;
	if (instr.f_record == Instrument::F_RECORD_PSG){
		note_shift = std::pow(2, (note - 69)/12.0);
	} else {
		note_shift = std::pow(2, (note - note_def.note)/12.0);
	}
	note_shift *= std::pow(2, static_cast<double>(channel.pitch_bend) * channel.pitch_bend_range / 128.0 / 12.0);
	
	if (instr.f_record == Instrument::F_RECORD_PCM ||
		instr.f_record == Instrument::F_RECORD_RANGE || 
		instr.f_record == Instrument::F_RECORD_REGIONAL) {
		auto& waveform = swar.swav[note_def.swav_no];
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
	} else if (instr.f_record == Instrument::F_RECORD_PSG){
		note_shift *= 28160.0 / static_cast<double>(PLAYBACK_SAMPLE_RATE);
		double actual_index = index * note_shift;
		
		auto psg = [](int index, int cycle){ return (index % 128 < (cycle + 1) * 16) ? -32767.0 : 32767.0; };
		
		double sample = psg(actual_index, note_def.swav_no); // * frac_index + (1.0 - frac_index) * psg(lower_index+1, note_def.swav_no);
		
		return (int)sample;
	} else if (instr.f_record == Instrument::F_RECORD_NOISE){
		//Taken from GBATEK
		//X=X SHR 1, IF carry THEN Out=LOW, X=X XOR 6000h ELSE Out=HIGH
		bool carry = (current_noise & 0x01) > 0;
		current_noise >>= 1;
		if (carry){
			current_noise ^= 0x6000;
			return -32767;
		} else {
			return 32767;
		}
	}
	return 0;
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
