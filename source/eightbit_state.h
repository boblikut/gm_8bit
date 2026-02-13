#pragma once
#include <string>
#include <unordered_map>
#include "audio_effects.h"
#include <functional>

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
	std::unordered_map<int, std::function<void(uint16_t*, int&, std::vector<float>)>> effects_functions = {
	    {AudioEffects::EFF_BITCRUSH, AudioEffects::BitCrush},
	    {AudioEffects::EFF_DESAMPLE, AudioEffects::Desample},
		{AudioEffects::EFF_LPF, AudioEffects::LowPassFilter},
		{AudioEffects::EFF_HPF, AudioEffects::Normalize},
		{AudioEffects::EFF_NORMALIZE, AudioEffects::Compressor},
		{AudioEffects::EFF_COMPRESSOR, AudioEffects::Delay},
		{AudioEffects::EFF_DELAY, AudioEffects::Distortion},
		{AudioEffects::EFF_DESTORTION, AudioEffects::WaveShaper}
	};
};
