```flencoder``` losslessly compresses an 8bit [WAV](https://en.wikipedia.org/wiki/WAV) file so that it takes up less disk space while retaining all the original information. The output is a TNDO file that can be used in your production. 

The tool reads a WAV file from ```stdin``` and writes the compressed data to ```stdout```.

```
mmendez$ flencoder < brut_final_28150.wav > brut_final_28150.tndo
TEXT: 'RIFF'
Full data size 12611236
TEXT: 'WAVE'
TEXT: 'fmt '
fmt  chunk size 16
audio_format 1
num_channels 2
sample_rate 28150
byte_rate 56300
block_align 2
bits_per_sample 8
TEXT: 'data'
data chunk size 12611200
num_samples 6305600, rounded to 6305664
Compressing signal for 8 bits
Bit encoding: average 4.5 bits
Bit encoding: average 4.6 bits

Lossless test passed, checksum 0xe27fd00

mmendez$ ls -la brut_final_28150.*
-rw-r--r--  1 mmendez  staff   7190564 17 Jun 14:30 brut_final_28150.tndo
-rw-r--r--  1 mmendez  staff  12611244 19 Apr 18:23 brut_final_28150.wav
```

You can read more about how to use these files in the ```Music``` section of the user manual.