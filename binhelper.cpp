#include "binhelper.hpp"

int binary_as_int(char* data){
	/*
	* Reads 4 bytes as a single integer, little endian
	*/
	int number = static_cast<unsigned char>(data[0]);
	number += static_cast<unsigned char>(data[1]) << 8;
	number += static_cast<unsigned char>(data[2]) << 16;
	number += data[3] << 24;
	return number;
}

short binary_as_short(char* data){
	short number = static_cast<unsigned char>(data[0]);
	number += data[1] << 8;
	return number;
}

unsigned short binary_as_ushort(char* data){
	unsigned short number = static_cast<unsigned char>(data[0]);
	number += static_cast<unsigned char>(data[1]) << 8;
	return number;
}

unsigned short binary_as_u24(char* data){
	unsigned short number = static_cast<unsigned char>(data[0]);
	number += static_cast<unsigned char>(data[1]) << 8;
	number += static_cast<unsigned char>(data[2]) << 16;
	return number;
}

std::string as_hex(int i){
	unsigned int j = static_cast<unsigned int>(i);
	if (j == 0)
		return "0x00";
	std::string digits{"0123456789abcdef"};
	std::string result{};
	while (j != 0){
		result = digits[j & 0x0f] + result;
		j >>= 4;
	}
	return "0x" + result;
}
