```wav2samp``` converts 8bit mono [WAV](https://en.wikipedia.org/wiki/WAV) files to raw samples that can be used for e.g. splash screens. The tool automatically converts the samples to signed which the Amiga audio hardware requires.

We give it an input and out file, like this;

```
mmendez$ wav2samp -i herbert.wav -o herbert.raw
Sample information:
Bits per sample: 8
Sample rate: 11025
Size: 13824
```

The resulting file starts with a very minimalistic header with the necessary information to correctly replay the sample:

* Big endian ```uint32_t``` containing the sample rate.
* Big endiang ```uint32_t``` containing the size of the sample data.
* Sample data.
