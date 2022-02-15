### Fun project to turn Teensy 2.0 microcontroller into a key storage and signature generation device

But basically it should work on any Arduino IDE compatible microcontroller

Fun fact: this sketch uses 30324 bytes (94%) of Arduino 2.0 program memory

## Building

You will need Arduino IDE for building the firmware

_Firmware:_
[1] Put `Crypto` and `CryptoLegacy` folders from https://github.com/rweather/arduinolibs/tree/master/libraries to your `Arduino/libraries` folder
[2] Open `Crypto-Wallet.ino` and upload it to your microcontroller

Client is not finished and currently only suitable for testing, you can test calling operations from it

_Client:_
[1] `cd client`
[2] `yarn`
[3] `npx tsc`
[4] `node .`
