The tornadoOptions flags
===================

The ```tornadoOptions``` variable contains a series of flags that control certain aspects of the Tornado framework.

The examples use a sane default that should work for every use case. These are the settings for the ```blur``` example:

```c
  dp->minCPU = MIN_CPU_040;
  dp->tornadoOptions =
      KILL_OS | LOGGING | INSTALL_LEVEL3 | INSTALL_LEVEL2 | USE_AUDIO;
#ifdef __DEBUG_CODE
  dp->tornadoOptions |=
      VERBOSE_DEBUGGING | MEMORY_PROFILING | EMIT_CONTAINER_SCRIPT;
#endif
```

* We need at least an '040 CPU and we always shut down the OS, install the VBL and keyboard handlers and use audio.
* Only during development we have the debugging information, memory profiling and the container script generator.

***Setting the incorrect flags will cause unwanted side effects ranging from erroneous behaviour of the system to memory corruption and crashes.***

The flags are defined in the ```tornado_settings.h``` include file. Any flag not documented here is considered internal and subject to change at any time.

CPU settings
------------------

* ```MIN_CPU_020``` : The demo requires at least a 68020 processor. 
* ```MIN_CPU_030``` : The demo requires at least a 68030 processor.
* ```MIN_CPU_040``` : The demo requires at least a 68040 processor.
* ```MIN_CPU_060``` : The demo requires at least a 68060 processor.

Hardware control
-----------------------

* ```KILL_OS``` : Shut down AmigaOS and take full control of the hardware while the demo runs. Not setting this will break many things, so please don't do it.
* ```INSTALL_LEVEL3``` : This will install the VBL handler that is used to drive PCM audio replay and each effect's VBL callback. Again, do not disable this unless you have a very good reason to.
* ```INSTALL_LEVEL2``` : This setting allows you to read the keyboard while the demo is running.
* ```USE_AUDIO``` : This flag instructs Tornado to initialise the audio hardware for PCM replay. Set only if you use streaming music in your demo, which is most likely the case.

Development settings
-----------------------------

* ```VERBOSE_DEBUGGING```: This flag will make Tornado display additional debugging information related to memory allocation and resource usage. Should be enabled during development.
* ```MEMORY_PROFILING``` : Print a memory usage summary at the end. For more information on this, please consult the [Memory Manager](MemoryManager.md) section.

Asset Management
--------------------------

* ```NO_Z_DECOMPRESS``` : Do not automatically decompress compressed assets. It is then your responsibility to unpack them at a later point.
* ```ASSETS_IN_REUSABLE_MEM```: Load assets in the reusable memory pool. This can be useful if you don't need the original asset data after processing during initialisation.
* ```ENABLE_TNDO_VFS```: The Tornado [Asset Manager](AssetManager.md) will operate in container mode and load all the assets from a TNDO container instead of using regular files.
* ```EMIT_CONTAINER_SCRIPT```: A container build script will be written at the end of the demo execution to facilitate the creation of the TNDO file used for release.

Other
-----
* ```LOGGING```: Display the Tornado banner and the hardware information when the demo starts.
* ```SHOW_SPLASH```: This will call the ```demoSplash``` function used to display a splash screen during loading.
* ```DOUBLE_BUFFERED_SPLASH``` : This will instruct Tornado to allocate a double buffered splash, suited for animation.
