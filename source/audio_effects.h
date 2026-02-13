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
		EFF_DISTORTION,
		EFF_WAVESHAPER,
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
	    if (args.empty() || samples <= 0) return;
	    
	    float intensity = args.at(0);
	    if (intensity < 0.0f) intensity = 0.0f;
	    if (intensity > 1.0f) intensity = 1.0f;
	    
	    for (int i = 0; i < samples; i++) {
	        int16_t* signedSample = reinterpret_cast<int16_t*>(&sampleBuffer[i]);
	        float val = *signedSample / 32768.0f;
	        
	        float absVal = (val < 0) ? -val : val;
	        float newVal = val * (1.0f + intensity * absVal);
	        
	        if (newVal > 1.0f) newVal = 1.0f;
	        if (newVal < -1.0f) newVal = -1.0f;
	        
	        *signedSample = static_cast<int16_t>(newVal * 32767.0f);
	    }
	}

	static float reverbBuf1[4410];
	static float reverbBuf2[5512];
	static float reverbBuf3[7350];
	static float reverbBuf4[8820];
	static float allPassBuf1[2205];
	static float allPassBuf2[1764];
	
	static int reverbPos1 = 0, reverbPos2 = 0, reverbPos3 = 0, reverbPos4 = 0;
	static int allPassPos1 = 0, allPassPos2 = 0;
	
	void Reverb(uint16_t* sampleBuffer, int& samples, std::vector<float> args) {
	    if (args.size() < 3) return;
	    
	    float roomSize = args.at(0) / 255.0f;
	    float decay = args.at(1) / 255.0f;
	    float wetDry = args.at(2) / 255.0f;
	    
	    float feedback = 0.5f + decay * 0.4f;
	    
	    int size1 = (int)(sizeof(reverbBuf1) / sizeof(float) * roomSize);
	    int size2 = (int)(sizeof(reverbBuf2) / sizeof(float) * roomSize);
	    int size3 = (int)(sizeof(reverbBuf3) / sizeof(float) * roomSize);
	    int size4 = (int)(sizeof(reverbBuf4) / sizeof(float) * roomSize);
	    int sizeAP1 = sizeof(allPassBuf1) / sizeof(float);
	    int sizeAP2 = sizeof(allPassBuf2) / sizeof(float);
	    
	    if (size1 < 10) size1 = 10;
	    if (size2 < 10) size2 = 10;
	    if (size3 < 10) size3 = 10;
	    if (size4 < 10) size4 = 10;
	    
	    for (int i = 0; i < samples; i++) {
	        float input = u16_to_float(sampleBuffer[i]);
	        
	        float comb1 = reverbBuf1[reverbPos1];
	        float filtered1 = comb1 * feedback + input;
	        reverbBuf1[reverbPos1] = filtered1;
	        
	        float comb2 = reverbBuf2[reverbPos2];
	        float filtered2 = comb2 * feedback + input;
	        reverbBuf2[reverbPos2] = filtered2;
	        
	        float comb3 = reverbBuf3[reverbPos3];
	        float filtered3 = comb3 * feedback + input;
	        reverbBuf3[reverbPos3] = filtered3;
	        
	        float comb4 = reverbBuf4[reverbPos4];
	        float filtered4 = comb4 * feedback + input;
	        reverbBuf4[reverbPos4] = filtered4;
	        
	        float reverbSum = (comb1 + comb2 + comb3 + comb4) * 0.25f;
	        
	        float ap1In = reverbSum;
	        float ap1Delay = allPassBuf1[allPassPos1];
	        float ap1Out = -ap1In + ap1Delay * (1.0f - decay * 0.5f);
	        float ap1DelayNew = ap1In + ap1Delay * decay * 0.5f;
	        allPassBuf1[allPassPos1] = ap1DelayNew;
	        
	        float ap2In = ap1Out;
	        float ap2Delay = allPassBuf2[allPassPos2];
	        float ap2Out = -ap2In + ap2Delay * (1.0f - decay * 0.5f);
	        float ap2DelayNew = ap2In + ap2Delay * decay * 0.5f;
	        allPassBuf2[allPassPos2] = ap2DelayNew;
	        
	        float output = input * (1.0f - wetDry) + ap2Out * wetDry;
	        
	        sampleBuffer[i] = float_to_u16(output);
	        
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
