#include "SSEQStream.h"
#include "binhelper.hpp"

#include <iostream>
#include <istream>
#include <fstream>
#include <algorithm>
#include <cmath>

// will be used for adsr_update_rate when in decay or release mode
const int DECAY_UPDATE_LUT[]{
	1,1,3,5,7,10,12,14,
	16,19,21,23,25,27,29,31,
	33,35,38,39,40,42,44,46,
	47,49,51,52,53,55,56,58,
	59,60,61,62,64,65,66,67,
	68,69,69,71,72,72,74,74,
	74,75,75,76,77,77,78,78,
	78,78,79,79,79,79,80,79,
	80,79,80,80,80,80,79,79,
	79,78,78,78,77,75,76,75,
	77,77,75,75,76,75,77,76,
	77,76,77,76,77,76,77,77,
	80,80,80,81,80,81,81,81,
	80,81,81,81,81,81,81,82,
	82,82,82,83,83,83,84,84,
	85,86,87,88,91,95,106,159
};

int SSEQStream::index_from_offset(int offset){
	return std::distance(sseq.event_location.begin(), std::find(sseq.event_location.begin(), sseq.event_location.end(), offset));
}

SSEQStream::SSEQStream(SWAR& wave, SBNK& bank, SSEQ& sequence) : swar(wave), sbnk(bank), sseq(sequence){
	samples.resize(50000*2); //arbitrary
	note_events.resize(16);
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
	
	//auto& samples = samples;
	
	int added_samples = 16;
//	for (auto& channel : sseq.channels){
//		added_samples += channel.enabled ? 1 : 0;
//	}
//	int adsr_update_rate_global = 0;//220 * 240 / tempo;//std::size_t)PLAYBACK_SAMPLE_RATE/22050*2;
	//^I don't know what to do with this
	
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
//					std::cout << channel.id << " " << event.value1 << " " << event.value2 << std::endl;
					std::cout << sbnk.instruments[channel.instr].info() << std::endl;
				} else if (event.type == Event::JUMP) {
					channel.current_index = index_from_offset(event.value1);
				} else if (event.type == Event::CALL) {
					channel.call_stack.push_back(channel.current_index);
					channel.current_index = index_from_offset(event.value1);
				} else if (event.type == Event::RETURN) {
					channel.current_index = channel.call_stack.back()+1;
					channel.call_stack.pop_back();
				} else if (event.type <= Event::NOTE_HIGH) {
					NoteEvent* note = &note_events[channel.id]; //when all else fails, play on note corresponding to channel number
					for (auto& note_event : note_events){
						//find first note either not in use or in release phase in same channel
						if (note_event.channel == &channel && note_event.phase == NoteEvent::PHASE_RELEASE) {
							// Note owned by channel, but can be refreshed
							note = &note_event;
							break;
						} else if (note_event.phase == NoteEvent::PHASE_NONE){
							// Find note not in use
							note = &note_event;
							break;
						}
					}
//					note = note_events[channel.id];
					
					// Set the note information
					note->max_samples = duration(tempo, event.value2);
					note->current_sample = 0;
					note->phase_sample = 0;
					note->event = &event;
					note->phase = NoteEvent::PHASE_ATTACK;
					note->amplitude = NoteEvent::ADSR_MINIMUM;
					note->channel = &channel;
					note->adsr_sample = 1;
					note->adsr_update_rate = -1;
					
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
				} else if (event.type == Event::VOLUME_2){ //Also called "EXPRESSION" according to GBAtek
					// what does this do??
					std::cout << "UNIMPLEMENTED: " << event.info() << std::endl;
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
			
//			channel.current_sample++;
//			auto& event = sseq.events[channel.current_index];
			
			for (auto& note : note_events){
				if (note.phase != NoteEvent::PHASE_NONE && note.channel == &channel){
					note.phase_sample++;
					
					short sample = get_sample(channel, note);
					sample = apply_adsr(channel, note, sample) * (static_cast<double>(channel.volume) / 128);
					partial_sample = sample;
					
					int pan = channel.pan == 127 ? 128 : channel.pan;
					left_sample += partial_sample * (static_cast<double>(128 - pan)/128.0);
					right_sample += partial_sample * (static_cast<double>(pan)/128.0);
				}
			}
		}
		samples[2*(s-offset)] = 0;
		samples[2*(s-offset)+1] = 0;
		if (left_sample | right_sample){
			//doing this based on https://dsp.stackexchange.com/questions/3581/algorithms-to-mix-audio-signals-without-clipping
			//It seems to work well enough
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

short SSEQStream::apply_adsr(Channel& channel, NoteEvent& note, short sample){
	note.adsr_sample++;
	if (note.adsr_sample < note.adsr_update_rate){
		double adsr_amplitude = static_cast<double>(NoteEvent::ADSR_MINIMUM - note.amplitude) / NoteEvent::ADSR_MINIMUM * 50.0;
		//Note that the ADSR envelope decay is linear in decibels, not just waveform amplitude
		// adsr_amplitude is in range [0,50]=[quiet,loud]. Amplitude multiplier should be in [0,1], but with nonlinear scale 
		double modifier = std::pow(10, adsr_amplitude/10.0 - 5.0);
		
		return sample * modifier;
	}
	note.adsr_sample = 1; //starts at one because LUT starts from one
	
	auto& instr = sbnk.instruments[channel.instr];
	auto& note_def = instr.get_note_def(note.event->type);
	
	//immediately enter release phase if note has finished
	if (note.phase_sample > note.max_samples && note.phase != NoteEvent::PHASE_RELEASE){
		note.phase = NoteEvent::PHASE_RELEASE;
		note.adsr_update_rate = DECAY_UPDATE_LUT[127-note_def.release]; //sets release rate more correctly
//		std::cout << "Entering R:" << note.phase_sample << "," << note.amplitude << " Rr: "<< note_def.release << std::endl;
	}
	
	if (note.phase == NoteEvent::PHASE_ATTACK){
		note.amplitude = note_def.attack * note.amplitude / 255;
		if (note.amplitude >= NoteEvent::ADSR_MAXIMUM){
			note.amplitude = NoteEvent::ADSR_MAXIMUM;
			note.phase = NoteEvent::PHASE_DECAY;
			note.adsr_update_rate = DECAY_UPDATE_LUT[127-note_def.decay]; //sets decay rate more correctly
//			std::cout << "Entering D:" << note.phase_sample << "," << note.amplitude << " Dr: "<< note_def.decay << std::endl;
		}
	} else if (note.phase == NoteEvent::PHASE_DECAY) {
		note.amplitude -= note_def.decay;
		double level = 127 * static_cast<double>(NoteEvent::ADSR_MINIMUM - note.amplitude) / NoteEvent::ADSR_MINIMUM;
		if (level <= note_def.sustain){
			note.amplitude = ((127.0 - note_def.sustain) / 127.0) * NoteEvent::ADSR_MINIMUM;
			note.phase = NoteEvent::PHASE_SUSTAIN;
//			std::cout << "Entering S:" << note.phase_sample << "," << note.amplitude << " Sl: "<< note_def.sustain << std::endl;
		}
	} else if (note.phase == NoteEvent::PHASE_SUSTAIN){
		// do nothing, just hold current volume
	} else if (note.phase == NoteEvent::PHASE_RELEASE){
		note.amplitude -= note_def.release;
		if (note.amplitude <= NoteEvent::ADSR_MINIMUM){
			note.amplitude = NoteEvent::ADSR_MINIMUM;
			note.phase = NoteEvent::PHASE_NONE;
//			std::cout << "E:" << note.phase_sample << std::endl;
		}
	}
	
	double adsr_amplitude = static_cast<double>(NoteEvent::ADSR_MINIMUM - note.amplitude) / NoteEvent::ADSR_MINIMUM * 50.0;
	//Note that the ADSR envelope decay is linear in decibels, not just waveform amplitude
	// adsr_amplitude is in range [0,50]=[quiet,loud]. Amplitude multiplier should be in [0,1], but with nonlinear scale 
	double modifier = std::pow(10, adsr_amplitude/10.0 - 5.0);
	
	return sample * modifier;
}

short SSEQStream::get_sample(Channel& channel, NoteEvent& note_event){	
	static int current_noise = 0x7fff;
	double current_noise_frame = 0;
	
	int note = note_event.event->type;
//	std::size_t index = note_event.current_sample;
	
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
		note_event.current_sample += note_shift;
		
		std::size_t actual_index = static_cast<std::size_t>(note_event.current_sample);
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
		note_event.current_sample += note_shift * 2;
		double actual_index = static_cast<std::size_t>(note_event.current_sample);
		
		auto psg = [](int index, int cycle){ return (index % 128 < (cycle + 1) * 16) ? -32767.0 : 32767.0; };
		
		double sample = psg(actual_index, note_def.swav_no); // * frac_index + (1.0 - frac_index) * psg(lower_index+1, note_def.swav_no);
		
		return (int)sample;
	} else if (instr.f_record == Instrument::F_RECORD_NOISE){
		current_noise_frame += note_shift;
		//Taken from GBATEK
		//X=X SHR 1, IF carry THEN Out=LOW, X=X XOR 6000h ELSE Out=HIGH
		bool carry;
		while (current_noise_frame >= 1){
			carry = (current_noise & 0x01) > 0;
			current_noise >>= 1;
			if (carry){
				current_noise ^= 0x6000;
//				return -32767;
			} else {
//				return 32767;
			}
			current_noise_frame--;
		}
		return carry ? -32767 : 32767;
	}
	throw std::runtime_error{"f_record invalid type in SSEQStream::get_sample"};
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
