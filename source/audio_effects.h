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

void GasMask(uint16_t* sampleBuffer, int samples) {
    float lpState = 0.0f;
    float hpState = 0.0f;
    float prevInput = 0.0f;
    float resonanceState1 = 0.0f;
    float resonanceState2 = 0.0f;
    float envelope = 0.0f;
    static int noiseSeed = 12345;
    static float delayBuffer[1000] = {0};
    static int delayIndex = 0;
    
    for (int i = 0; i < samples; i++) {
        float input = sampleBuffer[i] / 32768.0f;
        float output = input;
        
        hpState = 0.02f * (hpState + output - prevInput);
        output = hpState;
        prevInput = input;
        
        lpState = lpState + 0.12f * (output - lpState);
        output = lpState;
        
        resonanceState1 = 0.8f * resonanceState1 + output;
        resonanceState2 = 0.7f * resonanceState2 + resonanceState1;
        output = output + 0.5f * (resonanceState1 - resonanceState2);
        
        float absOutput = std::abs(output);
        if (absOutput > envelope) {
            envelope += 0.01f * (absOutput - envelope);
        } else {
            envelope += 0.1f * (absOutput - envelope);
        }
        
        if (envelope > 0.3f) {
            float gainReduction = 0.3f + (envelope - 0.3f) / 4.0f;
            output = output * (gainReduction / envelope);
        }
        
        noiseSeed = noiseSeed * 1103515245 + 12345;
        float noise = ((noiseSeed >> 16) & 0x7FFF) / 32768.0f - 0.5f;
        output += noise * 0.03f;
        
        delayBuffer[delayIndex] = output;
        delayIndex = (delayIndex + 1) % 1000;
        int readIndex = (delayIndex - 882 + 1000) % 1000;
        output += delayBuffer[readIndex] * 0.3f;
        
        output = std::max(-1.0f, std::min(1.0f, output));
        
        sampleBuffer[i] = static_cast<uint16_t>(output * 32767.0f);
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
