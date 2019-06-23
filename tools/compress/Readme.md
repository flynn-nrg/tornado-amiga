```tndo_compress``` is the Tornado data compression tool. It implements a variant of the [LZW](https://en.wikipedia.org/wiki/LZW) and [LZSS](https://en.wikipedia.org/wiki/LZSS) compression algorithms with a focus on fast decompression.

The ```LZW``` implementation embeds the dictionary with the compressed data, thus transforming the decompression code into a trivial lookup and a memory copy.

Similarly, the ```LZSS``` decompressor is essentially a memory copy loop.

Compression ratios are typically between 10% and 40% worse than [ZLIB](https://zlib.net/), but decompression speeds are significantly faster and limited only by your Amiga's memory speed.

When tested with the [Canterbury Corpus](https://en.wikipedia.org/wiki/Canterbury_corpus) the results were satisfactory, achieving compression ratios comparable to other [Lempel-Ziv](https://en.wikipedia.org/wiki/Lempel-Ziv) implementations.

Let's pack the ```anim_delta``` encoded file:

```
mmendez$ tndo_compress -b -v -i capsule_anim_plte.raw -o capsule_anim_plte.tndo
--------------------------
Step 1 - Build dictionary.
--------------------------
Number of codes used: 16384
Maximum symbol length: 25
--------------------------------------------
Step 2 - Compress with pre-built dictionary.
--------------------------------------------
Uncompressed: 2445584, compressed: 1321228 (54.0%)
Maximum symbol length: 25
[...]
mmendez$ ls -la capsule_anim_plte.*
-rw-r--r--  1 mmendez  wheel  2445584 18 Jun 14:23 capsule_anim_plte.raw
-rw-r--r--  1 mmendez  wheel  1535216 18 Jun 15:26 capsule_anim_plte.tndo
```

* ```-b``` causes ```tndo_compress``` to try all compression settings and use the best one. It's the recommended way of using this tool. 
* ```-v``` enables verbose mode and will output progress data as the compressor works on the file.
* ```-i``` and ```-o``` are the input and output files, respectively.
