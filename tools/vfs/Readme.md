```assemble_assets``` combines a series of files into a single container file. Its main use is to create a single data file with all the demo assets nicely packed into one and is usually done once development has ended and we want to build the release version of our demo. You will find it in the ```vfs``` directory of the tools section.

***Please note that this tool does not compress the data. You should compress your individual assets using ```tndo_compress``` prior to putting them together.***

This is how that assets for the demo [Brutalism](http://www.pouet.net/prod.php?which=81062) were put together:

```
mmendez$ sh build_container.sh
Tornado Demo System Asset Manager
(C) 2017-2019 Miguel Mendez

Assembling assets to brutalism.tndo.
Adding files: data/capsulflix.tndo data/capsulflix.p61 data/loading_640_360.tndo data/loading_music.tndo data/brut_28150_8.tndo data/fountain-palrgb.raw data/protomin_back_1.tndo data/protomin_back_2.tndo data/protomin_back_3.tndo data/protomin_back_4.tndo data/fountain-palrgb.raw data/vinyetas-pal-tex.tndo data/vinyetas_sprite2.tndo data/vinyetas_sprite1.tndo data/vinyetas_sprite4.tndo data/vinyetas_sprite3.tndo data/scrolls_v.tndo data/scrolls2_v.tndo data/scrolls_h.tndo data/scrolls_spr.tndo data/scrolls2_h.tndo data/scrolls_spr2.tndo data/breuer-palrgb.tndo data/breuer-shd.tndo data/breuer-tex-reordered.tndo data/scrolls_spr.tndo data/fractal_fondo1.tndo data/fractal_fondo2.tndo data/fractal_fondo3.tndo data/fractal_fondo4.tndo data/fractal_bola1.tndo data/fractal_bola2.tndo data/fractal_bola3.tndo data/fractal_bola4.tndo data/voxels_fondo1.tndo data/voxels_fondo2.tndo data/voxels_fondo3.tndo data/voxels_fondo4.tndo data/fractal_sprite1.tndo data/fractal_sprite2.tndo data/fractal_sprite3.tndo data/fractal_sprite4.tndo data/tab_swirl.raw data/voxels_rock.tndo data/voxels_david.tndo data/voxels_david_cubed.tndo data/fractal.tndo data/blur-gr-1.rle data/blur-gr-2.rle data/blur-gr-3.rle data/blur-gr-4.rle data/blur-gr-5.rle data/blur-gr-6.rle data/blur-gr-7.rle data/blur-gr-8.rle data/blur-gr-9.rle data/blur-gr-10.rle data/blur-gr-11.rle data/blur-gr-12.rle data/blur-gr-13.rle data/blur-gr-14.rle data/blur-gr-15.rle data/blur-gr-16.rle data/blur-gr-17.rle data/blur-gr-18.rle data/blur-gr-19.rle data/blur-gr-20.rle data/blur-gr-21.rle data/blur-gr-22.rle data/blur-gr-23.rle data/blur-gr-24.rle data/blur-gr-25.rle data/blur-gr-26.rle data/blur-gr-27.rle data/blur-gr-28.rle data/blur-gr-29.rle data/blur-gr-30.rle data/back_blur.tndo data/freeway-palrgb.tndo data/freeway-shd.tndo data/freeway-tex-reordered.tndo data/scrolls_spr.tndo data/zoom.tndo data/zoom2.tndo data/zoom_frame0.tndo data/zoom.pal data/layers_back_1.tndo data/layers_back_2.tndo data/layers_back_3.tndo data/layers_back_4.tndo data/layers_spr1_a.tndo data/layers_spr1_b.tndo data/layers_spr1_c.tndo data/layers_spr2_a.tndo data/layers_spr2_b.tndo data/layers_spr2_c.tndo data/layers_spr3_a.tndo data/layers_spr3_b.tndo data/layers_spr3_c.tndo data/layers_spr4_a.tndo data/layers_spr4_b.tndo data/layers_spr4_c.tndo data/endscroll.p61 data/endscroll_back.tndo data/endscroll_fntbmp1.tndo data/endscroll_intro.tndo 
Done.
mmendez$ ls -la brutalism.tndo 
-rw-r--r--  1 mmendez  staff  16251608 18 Jun 15:41 brutalism.tndo
```

To learn more about how to prepare your demo for release, please refer to the ```Getting ready for release``` user manual section.