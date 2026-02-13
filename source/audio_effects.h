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

	inline float u16_to_float(uint16_t sample) {
	    int16_t signedSample = *reinterpret_cast<int16_t*>(&sample);
	    return signedSample / 32768.0f;
	}
	
	inline uint16_t float_to_u16(float sample) {
	    if (sample > 1.0f) sample = 1.0f;
	    if (sample < -1.0f) sample = -1.0f;
	    int16_t signedSample = static_cast<int16_t>(sample * 32767.0f);
	    return *reinterpret_cast<uint16_t*>(&signedSample);
	}

	void LowPassFilter(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.empty()) return;
	    
	    float coef = args.at(0);
	    if (coef < 0.0f) coef = 0.0f;
	    if (coef > 1.0f) coef = 1.0f;
	    
	    float prev = u16_to_float(sampleBuffer[0]);
	    
	    for (int i = 1; i < samples; i++) {
	        float current = u16_to_float(sampleBuffer[i]);
	        float filtered = prev + coef * (current - prev);
	        sampleBuffer[i] = float_to_u16(filtered);
	        prev = filtered;
	    }
	}
	
	void HighPassFilter(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.empty()) return;
	    
	    float coef = args.at(0);
	    if (coef < 0.0f) coef = 0.0f;
	    if (coef > 1.0f) coef = 1.0f;
	    
	    float prevInput = u16_to_float(sampleBuffer[0]);
	    float prevOutput = 0.0f;
	    
	    for (int i = 1; i < samples; i++) {
	        float input = u16_to_float(sampleBuffer[i]);
	        float output = coef * (prevOutput + input - prevInput);
	        sampleBuffer[i] = float_to_u16(output);
	        prevInput = input;
	        prevOutput = output;
	    }
	}
	
	void Normalize(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.empty() || samples == 0) return;
	    
	    float targetPeak = args.at(0);
	    if (targetPeak < 0.0f) targetPeak = 0.0f;
	    if (targetPeak > 1.0f) targetPeak = 1.0f;
	    
	    float maxVal = 0.0f;
	    for (int i = 0; i < samples; i++) {
	        float val = u16_to_float(sampleBuffer[i]);
	        float absVal = (val < 0) ? -val : val;
	        if (absVal > maxVal) maxVal = absVal;
	    }
	    
	    if (maxVal < 0.0001f) return;
	    
	    float gain = targetPeak / maxVal;
	    
	    for (int i = 0; i < samples; i++) {
	        float val = u16_to_float(sampleBuffer[i]);
	        val *= gain;
	        sampleBuffer[i] = float_to_u16(val);
	    }
	}
	
	void Compressor(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.size() < 2) return;
	    
	    float threshold = args.at(0) / 32768.0f;
	    float ratio = args.at(1);
	    if (ratio < 1.0f) ratio = 1.0f;
	    
	    for (int i = 0; i < samples; i++) {
	        float val = u16_to_float(sampleBuffer[i]);
	        float absVal = (val < 0) ? -val : val;
	        
	        if (absVal > threshold) {
	            float over = absVal - threshold;
	            float reduced = threshold + over / ratio;
	            if (val < 0) val = -reduced;
	            else val = reduced;
	        }
	        
	        sampleBuffer[i] = float_to_u16(val);
	    }
	}
	
	static float delayBuffer[44100];
	static int delayPos = 0;
	
	void Delay(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.size() < 2) return;
	    
	    int delaySamples = (int)args.at(0);
	    float feedback = args.at(1) / 255.0f;
	    
	    for (int i = 0; i < samples; i++) {
	        float input = u16_to_float(sampleBuffer[i]);
	        float delayed = delayBuffer[delayPos];
	        float mixed = (input + delayed) * 0.5f;
	        delayBuffer[delayPos] = input + delayed * feedback;
	        sampleBuffer[i] = float_to_u16(mixed);
	        delayPos++;
	        if (delayPos >= delaySamples) delayPos = 0;
	    }
	}
	
	void Distortion(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.empty()) return;
	    
	    float threshold = args.at(0) / 32768.0f;
	    if (threshold > 1.0f) threshold = 1.0f;
	    
	    for (int i = 0; i < samples; i++) {
	        float val = u16_to_float(sampleBuffer[i]);
	        if (val > threshold) val = threshold;
	        if (val < -threshold) val = -threshold;
	        sampleBuffer[i] = float_to_u16(val);
	    }
	}
	
	void WaveShaper(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.empty()) return;
	    
	    float intensity = args.at(0);
	    if (intensity < 0.0f) intensity = 0.0f;
	    if (intensity > 1.0f) intensity = 1.0f;
	    
	    for (int i = 0; i < samples; i++) {
	        float val = u16_to_float(sampleBuffer[i]);
	        float absVal = (val < 0) ? -val : val;
	        val = val * (1.0f + intensity * absVal);
	        if (val > 1.0f) val = 1.0f;
	        if (val < -1.0f) val = -1.0f;
	        sampleBuffer[i] = float_to_u16(val);
	    }
	}
}
