This is a simple example that will teach you how to do the following using Tornado:

* Load assets. Note how Tornado transparently decompresses compressed TNDO files at load time and allocates memory for them.
* Playing music with the AHI subsystem.
* Initialise the graphics subsystem. We're using a system friendly 320x256 8BPL screen with chunky buffers. 
* A very simple demo loop with calls to the c2p routine and a wait for mouse press.

Note that we did not use the ```KILL_OS``` flag, so Tornado will be running in OS friendly mode. You can drag the screen or switch between tasks while the effect is running!

The compressed TNDO file was created using the *tndo_compress* tool from the file Capsule_logo.png after converting it to raw using *png2raw*.
