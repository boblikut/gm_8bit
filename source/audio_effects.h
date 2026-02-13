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

	static uint16_t reverbBuffer[10 * 1024];
	static uint16_t delayLines[4][22050]; // 4 линии задержки по 0.5 сек при 44.1кГц
	static int writePositions[4] = {0, 0, 0, 0};
	void Reverb(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    const float ROOM_SIZE = std::max(0.1f, std::min(1.0f, args.at(0)));
	    const float DAMPING = std::max(0.0f, std::min(0.9f, args.at(1)));
	    const float WET_DRY_MIX = std::max(0.0f, std::min(1.0f, args.at(2)));
	    const int DELAY_TIMES[4] = {
	        static_cast<int>(150 * ROOM_SIZE),
	        static_cast<int>(317 * ROOM_SIZE),
	        static_cast<int>(487 * ROOM_SIZE),
	        static_cast<int>(613 * ROOM_SIZE)
	    };
	    const int SAMPLE_RATE = 44100;
	    const int DELAY_SAMPLES[4] = {
	        (DELAY_TIMES[0] * SAMPLE_RATE) / 1000,
	        (DELAY_TIMES[1] * SAMPLE_RATE) / 1000,
	        (DELAY_TIMES[2] * SAMPLE_RATE) / 1000,
	        (DELAY_TIMES[3] * SAMPLE_RATE) / 1000
	    };
	    uint16_t* outputBuffer = new uint16_t[samples];
	    
	    for (int i = 0; i < samples; i++) {
	        uint16_t inputSample = sampleBuffer[i];
	        float reverbSum = 0;
	        for (int line = 0; line < 4; line++) {
	            int readPos = writePositions[line] - DELAY_SAMPLES[line];
	            if (readPos < 0) readPos += sizeof(delayLines[line])/sizeof(uint16_t);
	            float delayedSample = delayLines[line][readPos];
	            float lastSample[4] = {0, 0, 0, 0};
	            delayedSample = lastSample[line] * DAMPING + delayedSample * (1.0f - DAMPING);
	            lastSample[line] = delayedSample;
	            reverbSum += delayedSample * 0.25f; // Нормализуем
	            float feedbackSample = inputSample + delayedSample * 0.7f;
	            feedbackSample = std::max(0.0f, std::min(65535.0f, feedbackSample));
	            delayLines[line][writePositions[line]] = static_cast<uint16_t>(feedbackSample); 
	            writePositions[line] = (writePositions[line] + 1) % 
	                                   (sizeof(delayLines[line])/sizeof(uint16_t));
	        }
	        float mixed = inputSample * (1.0f - WET_DRY_MIX) + reverbSum * WET_DRY_MIX;
	        mixed = std::max(0.0f, std::min(65535.0f, mixed));
	        outputBuffer[i] = static_cast<uint16_t>(mixed);
	    }
	    
	    std::memcpy(sampleBuffer, outputBuffer, samples * sizeof(uint16_t));
	    delete[] outputBuffer;
	}
}
