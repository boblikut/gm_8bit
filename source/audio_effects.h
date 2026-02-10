#pragma once
#include <cassert>
#include <cstdint>
#include <cstring>

namespace AudioEffects {
	enum {
		EFF_NONE,
		EFF_BITCRUSH,
		EFF_DESAMPLE,
		EFF_GASMASK
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

	void GasMask(uint16_t* sampleBuffer, int samples, float muffled) {
	    float cutoff = 0.02f + (1.0f - muffled) * 0.08f;
	    float gain = 1.0f - muffled * 0.3f;
	    float prev = 0.0f;
	    for (int i = 0; i < samples; i++) {
	        float sample = (float)sampleBuffer[i];
	        sample = cutoff * sample + (1.0f - cutoff) * prev;
	        prev = sample;
	        sample *= gain;
	        sampleBuffer[i] = (uint16_t)sample;
	    }
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
