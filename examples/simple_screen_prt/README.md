This is a simple example that will teach you how to do the following using Tornado:

* Load assets. Note how Tornado transparently decompresses compressed TNDO files at load time and allocates memory for them.
* How to play [Pretracker](http://www.pouet.net/prod.php?which=80999) modules in your demo.
* Initialise the graphics subsystem. We're using a 320x256 8BPL screen with chunky buffers. Tornado creates the necessary copper lists and allocates the planar and chunky buffers for you, optimally aligned.
* A very simple demo loop with calls to the c2p routine and a wait for mouse press.

The compressed TNDO file was created using the *tndo_compress* tool from the file Capsule_logo.png after converting it to raw using *png2raw*.
