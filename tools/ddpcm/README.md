Introduction
-------

```tndo_ddpcm``` is a lossy audio compressor based on [DPCM](https://en.wikipedia.org/wiki/Differential_pulse-code_modulation) with a focus on quality and low decompression cost.

The input is always a 16bit 2-channel [WAV](https://en.wikipedia.org/wiki/WAV) file and the ouput is a compressed TNDO audio file. Optionally, a preview file can be generated which is a 16bit 2-channel WAV file that contains the result of the compression and decompression of the source material.

Unlike the commonly used 4bit [IMA ADPCM](https://en.wikipedia.org/wiki/Adaptive_differential_pulse-code_modulation) format, used in many Amiga demos, ```Tornado DDPCM``` uses 6bit deltas which reduces the quantisation error. This means that the compression ratio is 2.4:1 instead of 4:1 for ADPCM, but the resulting audio quality is much higher.

Compression algorithm
---------------------

First the left and right audio channels are deinterleaved and stored in separate buffers. Each of them is then compressed independently.

The encoder uses a simple linear predictor expressed as:

```
predicted = s[n - 2] + (s[n - 1] - s[n - 2]) * 2
```

To compute sample n, we use the predictor and a delta value, like this:

```
s[n] = predict(s[n - 2], s[n - 1]) + delta
```

Unlike ADPCM, the quantisation tables are calculated for a specific file. 1024 64-entry ```q_tables``` are used, each of which covering 1/1024th of the samples.

Each ```q_table``` is computed by generating a histogram of all the deltas required to calculate the samples in that region and then picking the top 64 ones.

To compress a frame we first store the first 2 samples verbatim. Then, for the remaining 48 samples:

 * Set the delta scaling factor to 1.
 * Compress a frame and calculate the cumulative noise.
 * Iterate through all the scaling factors (1 to 64) and choose the one that yielded the lowest cumulative error.
 * Store the compressed frame and return the scaling factor.

The scaling factor is kept in the ```s_table``` and used during decompression.

Decompression
-------------

In order to decompress a frame we need:

 * The compressed frame data (40 bytes).
 * The scaling factor for that frame.
 * The corresponding ```q_table``` that holds the quantised deltas.

With that information we can generate 50 16bit samples:

```c
// Decodes a single frame from src to dst using the provided q_table.
void decodeFrame(uint8_t *src, int16_t *dst, int16_t *q_table, uint8_t scale) {
        uint8_t unpacked[DDPCM_FRAME_NUMSAMPLES - 2];
        int16_t y1, y2, p;
        
        uint16_t *first = (uint16_t *) src;
        dst[0] = ntohs((int16_t) first[0]);
        dst[1] = ntohs((int16_t) first[1]);
        
        unpack6to8(&src[4], unpacked);
        unpack6to8(&src[10], &unpacked[8]);
        unpack6to8(&src[16], &unpacked[16]);
        unpack6to8(&src[22], &unpacked[24]);
        unpack6to8(&src[28], &unpacked[32]);
        unpack6to8(&src[34], &unpacked[40]);
        
        for(uint32_t i = 2; i < DDPCM_FRAME_NUMSAMPLES; i++) {
          y1 = dst[i - 2];
          y2 = dst[i - 1];
          p = predict(y1, y2);
          dst[i] = p + (q_table[unpacked[i - 2]] / scale);
        }
}
```

Compressing a file
------------------
We need to provide an input file, an output file and, optinally, a preview file name:

```
thunderball:ddpcm mmendez$ ./tndo_ddcpm -i brutalism_16_22050.wav -o brutalism_16_22050.tndo -p brutalism_16_22050_ddpcm.wav 

Tornado DDPCM encoder. (C) 2019 Miguel Mendez.

Audio data information:
-----------------------
Bits per sample: 16
Sample rate: 22050Hz
Size: 19756800

Per channel data:
-----------------
Number of samples: 4966400
Sample size: 2
Frame size: 50 samples
Number of frames: 99328
Frames per quantisation table: 97

Compressing left channel: 99327/99328
Compressing right channel: 99327/99328

Uncompressed size: 19756800 bytes
Compressed size: 8407040 bytes (262144 bytes in qtables, 198656 bytes in scale tables, 7946240 bytes in compressed frames)
Decompressing left channel: 99327/99328
Decompressing right channel: 99327/99328
thunderball:ddpcm mmendez$ ls -la brutalism_16_22050*
-rw-r--r--  1 mmendez  staff   8407128 24 Jul 17:55 brutalism_16_22050.tndo
-rw-r--r--  1 mmendez  staff  19798444 24 Jul 17:52 brutalism_16_22050.wav
-rw-r--r--  1 mmendez  staff  19865644 24 Jul 17:55 brutalism_16_22050_ddpcm.wav
```
