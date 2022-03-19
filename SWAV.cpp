#include "SWAV.hpp"

std::string SWAV::info(){
	std::string info{};
	info += "Wave ID=" + std::to_string(index) + " "; 
	info += type ? "ADPCM " : "PCM8 ";
	info += loopSupport ? "Loops " : "";
	info +=	"Sample Rate=" + std::to_string(sampleRate) + " ";
	info +=	"Duration=" + std::to_string(duration) + " ";
	if (loopSupport){
		info +=	"Loop Start=" + std::to_string(loopStart) + " ";
		info +=	"Loop Length=" + std::to_string(loopLength) + " ";
	}
	info += "Samples=" + std::to_string(samples.size()) +" ";
	return info;
}
