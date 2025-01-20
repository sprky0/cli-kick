#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TWO_PI 6.283185307f

typedef struct {
    float tune;   // 0..1
    float attack; // 0..1
    float decay;  // 0..1
    float level;  // 0..1
} KickParams;

/**
 * Generate a single 909-like kick into 'buffer'.
 * sampleRate: e.g. 48000.0f
 * numSamples: length of output buffer
 * params:     user-defined 0..1 for each "knob"
 */
void generateKick(float *buffer, size_t numSamples, float sampleRate, KickParams params)
{
    // -- Envelope times (roughly) --
    float attackTime = 0.001f + (0.005f * params.attack); // ~1-6ms
    float decayTime  = 0.15f  + (0.3f   * params.decay);  // ~150-450ms
    
    // -- Frequency range (roughly) --
    float baseFreq   = 40.0f  + (40.0f  * params.tune);   // ~40 to ~80 Hz
    float startFreq  = 300.0f + (200.0f * params.tune);   // ~300 to ~500 Hz initial “thwack”
    
    // -- Convert times to sample counts --
    size_t attackSamples = (size_t)(attackTime * sampleRate);
    size_t decaySamples  = (size_t)(decayTime  * sampleRate);
    
    // For amplitude envelope
    float amp = 0.0f;
    float ampIncrement = (attackSamples > 0) 
                        ? (1.0f / (float)attackSamples) 
                        : 1.0f;
    float ampDecayRate = (decaySamples > 0) 
                        ? (1.0f / (float)decaySamples) 
                        : 1.0f;
    
    // For pitch envelope
    float pitchEnv = 0.0f;
    float pitchIncrement = (decaySamples > 0) 
                           ? (1.0f / (float)decaySamples) 
                           : 1.0f;
    
    // Short click
    float clickDuration = 0.0005f; // 0.5 ms
    size_t clickSamples = (size_t)(clickDuration * sampleRate);
    
    float phase = 0.0f;
    
    for (size_t n = 0; n < numSamples; n++) {
        // ===== AMPLITUDE ENVELOPE =====
        if (n < attackSamples) {
            // Attack ramp up
            amp += ampIncrement; 
            if (amp > 1.0f) amp = 1.0f;
        } else {
            // Decay portion
            amp -= ampDecayRate;
            if (amp < 0.0f) amp = 0.0f;
        }
        
        // ===== PITCH ENVELOPE =====
        if (n < decaySamples) {
            pitchEnv += pitchIncrement; 
            if (pitchEnv > 1.0f) pitchEnv = 1.0f;
        }
        
        float currFreq = startFreq * (1.0f - pitchEnv) 
                       + baseFreq  * pitchEnv;
        
        // ===== OSCILLATOR =====
        float sampleValue = sinf(phase);
        phase += (TWO_PI * currFreq / sampleRate);
        if (phase > TWO_PI) {
            phase -= TWO_PI;
        }
        
        // ===== CLICK / TRANSIENT =====
        if (n < clickSamples) {
            // Add a short pulse of amplitude
            sampleValue += 0.5f; 
        }
        
        // ===== AMPLITUDE & LEVEL =====
        sampleValue *= amp * params.level;
        
        // Store in buffer
        buffer[n] = sampleValue;
    }
}

/**
 * Write a minimal 24-bit mono WAV to the given FILE *.
 * buffer: pointer to float samples in [-1..1]
 * numSamples: how many samples
 * sampleRate: e.g. 48000
 */
void writeWav24Bit(FILE *fp, const float *buffer, size_t numSamples, int sampleRate)
{
    // --- WAV Header ---
    // RIFF chunk descriptor
    unsigned int dataChunkSize = (unsigned int)(numSamples * 3); // 24-bit = 3 bytes per sample (mono)
    unsigned int subchunk2Size = dataChunkSize;
    unsigned int chunkSize     = 36 + subchunk2Size; // (36 + SubChunk2Size) for PCM
    
    // Write "RIFF"
    fputs("RIFF", fp);
    // Write overall chunk size
    // (little-endian 32-bit)
    fwrite(&chunkSize, 4, 1, fp);
    // Write "WAVE"
    fputs("WAVE", fp);
    
    // fmt subchunk
    fputs("fmt ", fp);
    unsigned int subchunk1Size = 16;     // PCM
    fwrite(&subchunk1Size, 4, 1, fp);
    
    unsigned short audioFormat = 1;      // PCM
    unsigned short numChannels = 1;      // mono
    unsigned int   sRate       = (unsigned int)sampleRate;
    unsigned short bitsPerSample = 24;
    unsigned short blockAlign  = (unsigned short)(numChannels * (bitsPerSample / 8));
    unsigned int   byteRate    = sRate * blockAlign;
    
    fwrite(&audioFormat,   2, 1, fp);
    fwrite(&numChannels,   2, 1, fp);
    fwrite(&sRate,         4, 1, fp);
    fwrite(&byteRate,      4, 1, fp);
    fwrite(&blockAlign,    2, 1, fp);
    fwrite(&bitsPerSample, 2, 1, fp);
    
    // data subchunk
    fputs("data", fp);
    fwrite(&subchunk2Size, 4, 1, fp);
    
    // --- WAV Sample Data (24-bit) ---
    // Float in [-1..1] -> signed 24-bit in [-8388608..8388607]
    for (size_t i = 0; i < numSamples; i++) {
        float val = buffer[i];
        // Clip to [-1..1]
        if (val > 1.0f) val = 1.0f;
        if (val < -1.0f) val = -1.0f;
        
        // Scale up to 24-bit range
        // max positive = +8388607
        // max negative = -8388608
        float scaled = val * 8388607.0f;
        
        // Round to nearest integer
        int sample = (int)lrintf(scaled);
        
        // Clip again just in case
        if (sample >  8388607)  sample =  8388607;
        if (sample < -8388608)  sample = -8388608;
        
        // Write in little-endian 24-bit
        unsigned char b1 = (unsigned char)(sample & 0xFF);
        unsigned char b2 = (unsigned char)((sample >> 8) & 0xFF);
        unsigned char b3 = (unsigned char)((sample >> 16) & 0xFF);
        
        fwrite(&b1, 1, 1, fp);
        fwrite(&b2, 1, 1, fp);
        fwrite(&b3, 1, 1, fp);
    }
}

/**
 * Example main:
 *   ./kick909 <tune> <attack> <decay> <level> [-o <out.wav>]
 * If -o is omitted, writes to stdout.
 */
int main(int argc, char *argv[])
{
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <tune> <attack> <decay> <level> [-o <output.wav>]\n", argv[0]);
        return 1;
    }
    
    KickParams params;
    params.tune   = atof(argv[1]);  // 0..1
    params.attack = atof(argv[2]);  // 0..1
    params.decay  = atof(argv[3]);  // 0..1
    params.level  = atof(argv[4]);  // 0..1
    
    // Defaults
    float sampleRate = 48000.0f;
    float duration   = 1.0f;       // 1 second
    size_t numSamples = (size_t)(sampleRate * duration);
    
    // Parse optional -o
    FILE *outFile = stdout;  // default to stdout
    for (int i = 5; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if ((i + 1) < argc) {
                outFile = fopen(argv[i + 1], "wb");
                if (!outFile) {
                    fprintf(stderr, "Failed to open file '%s' for writing.\n", argv[i + 1]);
                    return 1;
                }
                i++;
            } else {
                fprintf(stderr, "Error: missing output file after -o.\n");
                return 1;
            }
        }
    }
    
    // Allocate buffer for float samples
    float *buffer = (float*)calloc(numSamples, sizeof(float));
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }
    
    // Generate the kick
    generateKick(buffer, numSamples, sampleRate, params);
    
    // Write to WAV (24-bit)
    writeWav24Bit(outFile, buffer, numSamples, (int)sampleRate);
    
    // Clean up
    free(buffer);
    if (outFile != stdout) {
        fclose(outFile);
    }
    
    return 0;
}
