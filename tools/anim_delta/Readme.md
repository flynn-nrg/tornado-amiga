```anim_delta``` transforms a color indexed PNG image into a series of delta-encoded frames to be used by the ```anim_replay``` code. 

You can see it in action in the ```splash``` example. All the images from the animation were put together sequentially using [ImageMagick](https://imagemagick.org/index.php)'s ```append``` command and the image was later converted to palette mode.

The data is encoded as follows:

* Palette in [LoadRGB32](http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node02FB.html) format.
* Big endian uint32_t containing the number of frames.
* Frame 0.
* Delta frames.

A delta frame consists of rows. Each row starts with a 32bit bitmap. A bit set to 1 means a 10 pixel segment was stored. Following the bitmap are all the stored segments.

To recreate frame ```n``` from frame ```n - 1``` all we have to do is copy the new segments at the corresponding offset.

Two things to note:
* Frame 0 is stored as is, without any bitmaps.
* The tool is designed to work images that are 320 pixels wide.

Let's convert a PNG file with our animation frames:

```
mmendez$ anim_delta -b -h 180 -i capsule_anim_plte.png -o capsule_anim_plte.raw
Image information: W: 320, H: 17460, Bit depth: 8, Colours: 256, Frames: 96
Original size: 5587200.
Encoded size: 2442500 (43.715992%).
Palette size: 3080.
```

* ```-b``` means that there's a black frame at the end that should be ignored. The reason for this is to force colour 0 to be black when converting the image to indexed colour and thus avoid having a non-black border when replaying the animation.
* ```-h``` defines the frame height. Because we work with a single image, the converter has no way of knowing how tall each frame is supposed to be.
* ```-i``` and ```-o``` set the input and output files, respectively.
