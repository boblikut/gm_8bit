#pragma once
#include <string>
#include <unordered_map>
#include "audio_effects.h"

struct Effect {
	int eff_id;
	std::vector<float> eff_args;
};

struct EightbitState {
	int crushFactor = 350;
	float gainFactor = 1.2;
	bool broadcastPackets = false;
	int desampleRate = 2;
	uint16_t port = 4000;
	std::string ip = "127.0.0.1";
	std::unordered_map<int, std::tuple<IVoiceCodec*, std::vector<Effect>>> afflictedPlayers;
};
