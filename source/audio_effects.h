#pragma once
#include <cassert>
#include <cstdint>
#include <cstring>

namespace AudioEffects {
	enum {
		EFF_NONE,
		EFF_BITCRUSH,
		EFF_DESAMPLE,
		EFF_LPF,
		EFF_HPF,
		EFF_NORMALIZE,
		EFF_COMPRESSOR,
		EFF_DELAY,
		EFF_DESTORTION,
		EFF_WAVESHAPER
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

	void LowPassFilter(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.empty()) return;
	    
	    float coef = args.at(0);
	    if (coef < 0.0f) coef = 0.0f;
	    if (coef > 1.0f) coef = 1.0f;
	    
	    uint16_t prev = sampleBuffer[0];
	    
	    for (int i = 1; i < samples; i++) {
	        uint16_t current = sampleBuffer[i];
	        float filtered = prev + coef * (current - prev);
	        sampleBuffer[i] = (uint16_t)filtered;
	        prev = sampleBuffer[i];
	    }
	}
	
	void HighPassFilter(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.empty()) return;
	    
	    float coef = args.at(0);
	    if (coef < 0.0f) coef = 0.0f;
	    if (coef > 1.0f) coef = 1.0f;
	    
	    uint16_t prevInput = sampleBuffer[0];
	    uint16_t prevOutput = 0;
	    
	    for (int i = 1; i < samples; i++) {
	        uint16_t input = sampleBuffer[i];
	        float output = coef * (prevOutput + input - prevInput);
	        if (output < 0) output = 0;
	        if (output > 65535) output = 65535;
	        
	        sampleBuffer[i] = (uint16_t)output;
	        prevInput = input;
	        prevOutput = (uint16_t)output;
	    }
	}
	
	void Normalize(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.empty() || samples == 0) return;
	    
	    float targetPeak = args.at(0);
	    if (targetPeak < 0.0f) targetPeak = 0.0f;
	    if (targetPeak > 1.0f) targetPeak = 1.0f;
	    
	    uint16_t maxVal = 0;
	    for (int i = 0; i < samples; i++) {
	        uint16_t val = sampleBuffer[i];
	        uint16_t dist = (val > 32768) ? (val - 32768) : (32768 - val);
	        if (dist > maxVal) maxVal = dist;
	    }
	    
	    if (maxVal == 0) return;
	    
	    float gain = (targetPeak * 32768) / maxVal;
	    
	    for (int i = 0; i < samples; i++) {
	        float val = sampleBuffer[i];
	        val -= 32768;
	        val *= gain;
	        val += 32768;
	        
	        if (val < 0) val = 0;
	        if (val > 65535) val = 65535;
	        
	        sampleBuffer[i] = (uint16_t)val;
	    }
	}
	
	void Compressor(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.size() < 2) return;
	    
	    uint16_t threshold = (uint16_t)args.at(0);
	    float ratio = args.at(1);
	    if (ratio < 1.0f) ratio = 1.0f;
	    
	    for (int i = 0; i < samples; i++) {
	        uint16_t val = sampleBuffer[i];
	        
	        if (val > threshold) {
	            uint16_t over = val - threshold;
	            over = (uint16_t)(over / ratio);
	            sampleBuffer[i] = threshold + over;
	        }
	    }
	}
	
	static uint16_t delayBuffer[44100];
	
	void Delay(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.size() < 2) return;
	    
	    int delaySamples = (int)args.at(0);
	    uint8_t feedback = (uint8_t)args.at(1);
	    
	    static int delayPos = 0;
	    
	    for (int i = 0; i < samples; i++) {
	        uint16_t delayed = delayBuffer[delayPos];
	        
	        uint16_t original = sampleBuffer[i];
	        uint32_t mixed = (original + delayed) / 2;
	        
	        uint32_t feedbackVal = (original * feedback) >> 8;
	        delayBuffer[delayPos] = (uint16_t)(feedbackVal);
	        
	        sampleBuffer[i] = (uint16_t)mixed;
	        
	        delayPos++;
	        if (delayPos >= delaySamples) delayPos = 0;
	    }
	}
	
	void Distortion(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.empty()) return;
	    
	    uint16_t threshold = (uint16_t)args.at(0);
	    uint16_t lowThreshold = 32768 - threshold;
	    uint16_t highThreshold = 32768 + threshold;
	    
	    for (int i = 0; i < samples; i++) {
	        uint16_t val = sampleBuffer[i];
	        
	        if (val > highThreshold) {
	            sampleBuffer[i] = highThreshold;
	        } else if (val < lowThreshold) {
	            sampleBuffer[i] = lowThreshold;
	        }
	    }
	}
	
	void WaveShaper(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.empty()) return;
	    
	    float intensity = args.at(0);
	    if (intensity < 0.0f) intensity = 0.0f;
	    if (intensity > 1.0f) intensity = 1.0f;
	    
	    for (int i = 0; i < samples; i++) {
	        float val = sampleBuffer[i];
	        val -= 32768;
	        
	        float absVal = (val < 0) ? -val : val;
	        val = val * (1.0f + intensity * absVal / 32768.0f);
	        
	        if (val > 32767) val = 32767;
	        if (val < -32768) val = -32768;
	        
	        val += 32768;
	        sampleBuffer[i] = (uint16_t)val;
	    }
	}
}
