This example builds on the simple_screen code and adds hardware sprites on top. Read the ```Sprites``` section of the user manual for more details.


The sprite was converted from a BMP file (```srcdata/sprite1.bmp```) using the ```bmp2bpl``` tool, like this:

```
bmp2bpl spr sprite1.bmp > sprite1.raw
```

And then compressed with tndo_compress:

```
tndo_compress -v -b -i sprite1.raw -o sprite1.tndo
```


**Please note that sprites are not emulated on the posix/SDL target!**
