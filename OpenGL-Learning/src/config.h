#pragma once

#define NOMINMAX
#define STRICT_TYPED_ITEMIDS
#include <Windows.h>
#include <shlwapi.h>

#include <yaml-cpp/yaml.h>
#include "glm/glm.hpp"

#include <string>
#include <iostream>

#define LOG(message) std::cout << message << std::endl
#define ASSERT(x) if((x)) __debugbreak();

enum class LOG_COLOR { LOG = 15, WARNING = 14, OK = 10, FAULT = 12, SPECIAL_A = 11, SPECIAL_B = 13 };

static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
inline void LOGC(const std::string& msg, LOG_COLOR color = LOG_COLOR::LOG) {
	SetConsoleTextAttribute(hConsole, (int)color);
	std::cout << msg << '\n';
	SetConsoleTextAttribute(hConsole, 15);
}

class Config {
private:
	const std::string m_Path;
public:
	Config(const std::string& path) 
		:m_Path(path)
	{
		Parse();
	}

	void Parse() {
		LOGC("Parsing Config", LOG_COLOR::SPECIAL_A);
		YAML::Node mainNode = YAML::LoadFile(m_Path);

		WIN_WIDTH = mainNode["config"]["window"]["Width"].as<unsigned int>();
		WIN_HEIGHT = mainNode["config"]["window"]["Height"].as<unsigned int>();
	}
	
	// Window
	unsigned int WIN_WIDTH = 0;
	unsigned int WIN_HEIGHT = 0;
};

extern Config conf;