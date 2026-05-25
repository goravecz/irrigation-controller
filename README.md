# irrigation-controller

A simple UI to control the irrigation in my yard via an ESP32 connected to a Hunter irrigation controller.

## Credits & License

The `firmware/HunterRoam.h` and `firmware/HunterRoam.cpp` files are a port of the [HunterRoam library](https://github.com/ecodina/hunter-wifi/tree/master/esp8266-hunter-sprinkler/lib/HunterRoam) by [Eloi Codina](https://github.com/ecodina), originally written for ESP8266.

Copyright (C) 2020 Eloi Codina. Licensed under [GPL v3](https://www.gnu.org/licenses/gpl-3.0.html).

Modifications: ported from ESP8266 to ESP32 (updated board includes, pin configuration).

The firmware in this repository is also released under GPL v3 in compliance with the original license. The UI (`index.html`) and Cloudflare Worker (`worker.js`) are separate works and are not subject to the GPL.
