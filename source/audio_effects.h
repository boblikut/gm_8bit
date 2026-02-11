#pragma once
#include <cassert>
#include <cstdint>
#include <cstring>

namespace AudioEffects {
	enum {
		EFF_NONE,
		EFF_BITCRUSH,
		EFF_DESAMPLE,
		EFF_REVERB
	};

	void BitCrush(uint16_t* sampleBuffer, int samples, float quant, float gainFactor) {
		for (int i = 0; i < samples; i++) {
			//Signed shorts range from -32768 to 32767
			//Let's quantize that a bit
			float f = (float)sampleBuffer[i];
			f /= quant;
			sampleBuffer[i] = (uint16_t)f;
			sampleBuffer[i] *= quant;
			sampleBuffer[i] *= gainFactor;
		}
	}

void Reverb(uint16_t* sampleBuffer, int samples) {
    const int DELAY_SAMPLES = samples * 0.25;
    const float FEEDBACK = 0.6f;
    const float WET_DRY_MIX = 0.7f;
    
    uint16_t* delayBuffer = new uint16_t[samples + DELAY_SAMPLES];
    uint16_t* processedBuffer = new uint16_t[samples];
    
    std::memcpy(delayBuffer, sampleBuffer, samples * sizeof(uint16_t));
    std::memset(delayBuffer + samples, 0, DELAY_SAMPLES * sizeof(uint16_t));
    std::memcpy(processedBuffer, sampleBuffer, samples * sizeof(uint16_t));
    
    for (int i = 0; i < samples; i++) {
        if (i >= DELAY_SAMPLES) {
            float wet = delayBuffer[i - DELAY_SAMPLES] * FEEDBACK;
            float dry = processedBuffer[i];
            float mixed = dry * (1.0f - WET_DRY_MIX) + wet * WET_DRY_MIX;
            
            if (mixed > 65535) mixed = 65535;
            if (mixed < 0) mixed = 0;
            
            processedBuffer[i] = static_cast<uint16_t>(mixed);
            delayBuffer[i] = static_cast<uint16_t>(mixed);
        }
    }
    
    std::memcpy(sampleBuffer, processedBuffer, samples * sizeof(uint16_t));
    delete[] delayBuffer;
    delete[] processedBuffer;
}

	static uint16_t tempBuf[10 * 1024];
	void Desample(uint16_t* inBuffer, int& samples, int desampleRate = 2) {
		assert(samples / desampleRate + 1 <= sizeof(tempBuf));
		int outIdx = 0;
		for (int i = 0; i < samples; i++) {
			if (i % desampleRate == 0) continue;

			tempBuf[outIdx] = inBuffer[i];
			outIdx++;
		}
		std::memcpy(inBuffer, tempBuf, outIdx * 2);
		samples = outIdx;
	}
}
