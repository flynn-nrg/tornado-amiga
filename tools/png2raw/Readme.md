```png2raw``` converts an 8 or 4 bit per pixel [PNG](https://en.wikipedia.org/wiki/Portable_Network_Graphics) image into a file containing:

* The palette in [LoadRGB32](http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node02FB.html) format.
* The RAW pixel data.

You can optionally not save the palette or just save palette.

```
mmendez$ png2raw 
Usage: png2raw [-n] [-p] -i <input_file> -o <output_file>
       -n : Do not save palette.
       -p : Only save the palette.
```

Let's convert the background image used in the ```simple_screen``` example:

```
mmendez$ png2raw -i Capsule_logo.png -o Capsule_logo.raw
Image information: W: 320, H: 256, Bit depth: 8, Colours: 256
Palette size: 3080.
```
