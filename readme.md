### Fun project to turn Teensy 2.0 microcontroller into a key storage and signature generation device

It should work on any Arduino IDE compatible microcontroller though

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

## Operations

You can either generate a private key directly on MC so it never leaves its internal memory or you can write existing key

_Generating a new key:_
**input:** `g[Password : string]`
**output**: `g[Status : 1 byte bool]( [PublicKey : 32 bytes] - true | [FailReason : string]\0 - false )`

_Storing existing key:_
**input**: `k[Password : string]\0[Key : 32 bytes]`
**output**: `k[Status : 1 byte bool]( *empty* - true | [FailReason : string]\0 - false)`

Currently there's no way to check if password was correct. So it's advised to test the signature with your saved public key

_Signing payload:_
**input**: `s[Password : string]\0[PayloadLength : 2 bytes int le][Payload : PayloadLength bytes]`
**output**: `s[Status : 1 byte bool]( [Signature : 64 bytes] - true | [FailReason : \0 terminated] - false )`

You can check `client/src/index.ts` for operation examples
