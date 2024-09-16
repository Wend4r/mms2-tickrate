## Sample plugin

### Requirements (included)

* [HL2SDK](https://github.com/Wend4r/hl2sdk) of Source 2 game on writing plugin for.
* [Metamod:Source](https://github.com/alliedmodders/metamod-source)

### Setting up

* Windows: Open the [MSVC developer console](https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line) with the correct platform (x86_64) that you plan on targetting.
* Configure with ``cmake --preset {PRESET}`` where:
  * ``{PRESET}`` - specify a configure preset.
  * Are allowed:
    * Linux: `Debug` and `Release`.
    * Windows: `Windows/Debug` and `Windows/Release`.
* Build with ``cmake --preset {PRESET} --build --parallel``.
* Once the plugin is compiled the files would be packaged and placed in ``build/{PRESET}`` folder.
* Be aware that plugins get loaded either by corresponding ``.vdf`` files in the metamod folder, or by listing them in ``addons/metamod/metaplugins.ini`` file.
