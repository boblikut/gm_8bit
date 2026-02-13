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

	void BitCrush(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
		for (int i = 0; i < samples; i++) {
			//Signed shorts range from -32768 to 32767
			//Let's quantize that a bit
			float f = (float)sampleBuffer[i];
			f /= args.at(0);
			sampleBuffer[i] = (uint16_t)f;
			sampleBuffer[i] *= args.at(0);
			sampleBuffer[i] *= args.at(1);
		}
	}

	static uint16_t tempBuf[10 * 1024];
	void Desample(uint16_t* inBuffer, int& samples, std::vector<float> args) {
		assert(samples / (int)args.at(0) + 1 <= sizeof(tempBuf));
		int outIdx = 0;
		for (int i = 0; i < samples; i++) {
			if (i % (int)args.at(0) == 0) continue;

			tempBuf[outIdx] = inBuffer[i];
			outIdx++;
		}
		std::memcpy(inBuffer, tempBuf, outIdx * 2);
		samples = outIdx;
	}

	static uint16_t reverbBuf1[44100 / 10];  // ~100ms при 44.1kHz
	static uint16_t reverbBuf2[44100 / 8];   // ~125ms
	static uint16_t reverbBuf3[44100 / 6];   // ~166ms
	static uint16_t reverbBuf4[44100 / 5];   // ~200ms
	static uint16_t allPassBuf1[44100 / 20]; // ~50ms
	static uint16_t allPassBuf2[44100 / 25]; // ~40ms
	
	static int reverbPos1 = 0, reverbPos2 = 0, reverbPos3 = 0, reverbPos4 = 0;
	static int allPassPos1 = 0, allPassPos2 = 0;
	
	/**
	 * Reverb
	 * args[0] - room size (0-255)
	 * args[1] - fade-out (0-255)
	 * args[2] - wet/dry (0-255)
	 */
	void Reverb(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.size() < 3) return;
	    
	    uint8_t roomSize = (uint8_t)args.at(0);
	    uint8_t decay = (uint8_t)args.at(1);
	    uint8_t wetDry = (uint8_t)args.at(2);
	    
	    float feedback = 0.5f + (decay / 255.0f) * 0.4f;  // 0.5 - 0.9
	    float wetLevel = wetDry / 255.0f;
	    float dryLevel = 1.0f - wetLevel;
	    
	    int size1 = sizeof(reverbBuf1) / 2 * (roomSize / 255.0f);
	    int size2 = sizeof(reverbBuf2) / 2 * (roomSize / 255.0f);
	    int size3 = sizeof(reverbBuf3) / 2 * (roomSize / 255.0f);
	    int size4 = sizeof(reverbBuf4) / 2 * (roomSize / 255.0f);
	    int sizeAP1 = sizeof(allPassBuf1) / 2;
	    int sizeAP2 = sizeof(allPassBuf2) / 2;
	    
	    if (size1 < 10) size1 = 10;
	    if (size2 < 10) size2 = 10;
	    if (size3 < 10) size3 = 10;
	    if (size4 < 10) size4 = 10;
	    
	    for (int i = 0; i < samples; i++) {
	        uint16_t input = sampleBuffer[i];
	        
	        uint16_t comb1 = reverbBuf1[reverbPos1];
	        int32_t filtered1 = (comb1 * feedback) + input;
	        if (filtered1 > 65535) filtered1 = 65535;
	        if (filtered1 < 0) filtered1 = 0;
	        reverbBuf1[reverbPos1] = (uint16_t)filtered1;
	        
	        uint16_t comb2 = reverbBuf2[reverbPos2];
	        int32_t filtered2 = (comb2 * feedback) + input;
	        if (filtered2 > 65535) filtered2 = 65535;
	        if (filtered2 < 0) filtered2 = 0;
	        reverbBuf2[reverbPos2] = (uint16_t)filtered2;
	        
	        uint16_t comb3 = reverbBuf3[reverbPos3];
	        int32_t filtered3 = (comb3 * feedback) + input;
	        if (filtered3 > 65535) filtered3 = 65535;
	        if (filtered3 < 0) filtered3 = 0;
	        reverbBuf3[reverbPos3] = (uint16_t)filtered3;
	        
	        uint16_t comb4 = reverbBuf4[reverbPos4];
	        int32_t filtered4 = (comb4 * feedback) + input;
	        if (filtered4 > 65535) filtered4 = 65535;
	        if (filtered4 < 0) filtered4 = 0;
	        reverbBuf4[reverbPos4] = (uint16_t)filtered4;
	        
	        int32_t reverbSum = comb1 + comb2 + comb3 + comb4;
	        reverbSum /= 2;  // Нормализация
	        
	        uint16_t ap1In = (uint16_t)reverbSum;
	        uint16_t ap1Delay = allPassBuf1[allPassPos1];
	        
	        int32_t ap1Out = -ap1In + (ap1Delay * (255 - decay) / 255);
	        if (ap1Out > 65535) ap1Out = 65535;
	        if (ap1Out < 0) ap1Out = 0;
	        
	        int32_t ap1DelayNew = ap1In + (ap1Delay * decay / 255);
	        if (ap1DelayNew > 65535) ap1DelayNew = 65535;
	        if (ap1DelayNew < 0) ap1DelayNew = 0;
	        
	        allPassBuf1[allPassPos1] = (uint16_t)ap1DelayNew;
	        
	        uint16_t ap2In = (uint16_t)ap1Out;
	        uint16_t ap2Delay = allPassBuf2[allPassPos2];
	        
	        int32_t ap2Out = -ap2In + (ap2Delay * (255 - decay) / 255);
	        if (ap2Out > 65535) ap2Out = 65535;
	        if (ap2Out < 0) ap2Out = 0;
	        
	        int32_t ap2DelayNew = ap2In + (ap2Delay * decay / 255);
	        if (ap2DelayNew > 65535) ap2DelayNew = 65535;
	        if (ap2DelayNew < 0) ap2DelayNew = 0;
	        
	        allPassBuf2[allPassPos2] = (uint16_t)ap2DelayNew;
	        
	        int32_t output = (input * dryLevel) + (ap2Out * wetLevel);
	        if (output > 65535) output = 65535;
	        if (output < 0) output = 0;
	        
	        sampleBuffer[i] = (uint16_t)output;
	        
	        reverbPos1++;
	        if (reverbPos1 >= size1) reverbPos1 = 0;
	        
	        reverbPos2++;
	        if (reverbPos2 >= size2) reverbPos2 = 0;
	        
	        reverbPos3++;
	        if (reverbPos3 >= size3) reverbPos3 = 0;
	        
	        reverbPos4++;
	        if (reverbPos4 >= size4) reverbPos4 = 0;
	        
	        allPassPos1++;
	        if (allPassPos1 >= sizeAP1) allPassPos1 = 0;
	        
	        allPassPos2++;
	        if (allPassPos2 >= sizeAP2) allPassPos2 = 0;
	    }
	}
}
