```bmp2bpl``` converts [BMP](https://en.wikipedia.org/wiki/BMP_file_format) images to planar data, either standard bitplanes or sprites.

While most of the effects you are going to write will make use of chunky buffers, sometimes you need data in planar format, e.g. when using [Sprites](Sprites.md).

The tool takes two arguments: output mode and input file, and writes the result to ```stdout```.

```
mmendez$ bmp2bpl --help
Usage: bmp2bpl [bpl/spr] pic.bmp > bpls.raw
```

Let's convert the data used in the ```sprites``` example:

```
mmendez$ bmp2bpl spr sprite1.bmp > sprite1.raw
Number of colors: 256
WARNING: you requested sprites, please note this bitmap has palette larger than 16
Writting 2 sprites of 4 bitplanes, using pairs
```
