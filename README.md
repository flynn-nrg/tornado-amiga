Tornado Amiga Demo System
=======================

Tornado is an [Amiga](https://en.wikipedia.org/wiki/Amiga) [demo](https://en.wikipedia.org/wiki/Demoscene) framework. It allows you to cross-develop and test code on both Amiga and Posix/SDL environments (currently MacOS X and GNU/Linux).

Introduction
----------------

Back in early 2017 several friends from the demo group [Capsule](http://www.pouet.net/groups.php?which=178) decided to get back together and start coding on the Amiga again after a 20 year hiatus. It quickly became apparent that developing natively on the Amiga would not be efficient. Luis 'Peskanov' Pons had a very basic cross-compiler setup with Amiga and SDL targets that he had been using to test ideas, so I took the concept further and created a full framework that would take care of things such as file and memory management, hardware abstraction and instrumentation and debugging.

When this project started no other public frameworks existed but we took ideas from Michal ‘kiero’ Wozniak's [Amiga demo making talk](https://www.youtube.com/watch?v=Pr75DoARDMQ) as well as noname's [Modern Amiga Demo Cross-Development](https://www.youtube.com/watch?v=s1lVS4tW33g) talk at Evoke 2018.

This framework was used to develop [Renouncetro](http://www.pouet.net/prod.php?which=75732) and [Brutalism](http://www.pouet.net/prod.php?which=81062), two productions released at Revision 2018 and 2019 respectively.

If this sounds interesting you can take a look at the [user manual](manual/Readme.md) and the [examples](examples).

Authors
----------

Tornado was mainly developed by Miguel 'Flynn' Mendez with contributions from Luis 'Peskanov' Pons and Antonio 'winden' Vargas.

The music used in the p61 and DDPCM examples was composed by Carlos 'Estrayk' del Alamo.

The music used in the Pretracker example was composed by  Jakub 'AceMan' Szelag and it's part of the Pretracker distribution. 

The graphics used in the examples were created by Manuel 'Leunam' Sagall and Jordi Carlos 'God'.


It also makes use of the following third party software:

* [Rocket](https://github.com/kusma/rocket): Used for synchronisation.
* [BASS](https://www.un4seen.com/): Used to play music on the SDL/Posix target.
* [ImGUI](https://github.com/ocornut/imgui) and [ImGuiSDL](https://github.com/Tyyppi77/imgui_sdl): Used for the SDL/Posix UI and rocket controls.
* [vbcc](http://www.compilers.de/vbcc.html): Used to cross-compile Amiga code.
* [Kalmalyzer](https://github.com/Kalmalyzer): Michael Kalms' collection of [c2p](https://www.lysator.liu.se/~mikaelk/doc/c2ptut/) and audio routines. 
* [The Player 6.1](http://www.pouet.net/prod.php?which=19922): Mod replay routines.
* [Pretracker](http://www.pouet.net/prod.php?which=80999) by Pink/Abyss.
* [STB single-file public domain libraries for C/C++](https://github.com/nothings/stb).

License
----------

This software is released under the [zlib license](https://en.wikipedia.org/wiki/Zlib_License).

