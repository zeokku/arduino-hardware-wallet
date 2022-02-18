### Fun project to turn Teensy 2.0 microcontroller into a key storage and signature generation device

It should work on any Arduino IDE compatible microcontroller though

Fun fact: this sketch uses 30308 bytes (93%) of Teensy 2.0 program memory

## Building

You will need Arduino IDE for building the firmware

### _Firmware:_

1. Put `Crypto` and `CryptoLegacy` folders from https://github.com/rweather/arduinolibs/tree/master/libraries to your `Arduino/libraries` folder

2. Open `Crypto-Wallet.ino` and upload it to your microcontroller

Client is not finished yet and currently only suitable for testing, but you can check existing test calls for all operations there, so you could easily implement own client

### _Client:_

1. `cd client`
2. `yarn`
3. `npx tsc`
4. `node .`

## Operations

All operations and their results are prefixed with `operation code` which corresponds to specific ASCII character.

In result 2nd byte will be `operation result` and its code corresponds to the following enum (found in `src/error_codes.ts`):

```
  "Success", // 0
  "Password is too short",
  "Invalid password",
  "Invalid key",
  "Invalid payload length",
  "Invalid payload",
```

You may check `client/src/index.ts` for operation examples and result handling

You have two options. You can either generate a private key directly on MC so it never leaves its internal memory, unless you export it - check `BUILD_ALLOW_PRIVATE_EXPORT` macro to enable/disable this. Or you also can store already existing key.

### g | _Generating a new key:_

**input:** `g[Password : string]`

**output**: `g[OperationResult : byte](PublicKey : 32 bytes)`

### k | _Storing existing key:_

**input**: `k[Password : string]\0[PublicKey : 32 bytes]`

**output**: `k[OperationResult : byte]`

### s | _Signing payload:_

<u>Currently there's no way to check if password was correct. So it's advised to test the signature with your saved public key</u>

**input**: `s[Password : string]\0[PayloadLength : uint16 le][Payload : PayloadLength bytes]`

**output**: `s[OperationResult : byte](Signature : 64 bytes)`

### p | _Exporting public key:_

**input**: `p[Password : string]`

**output**: `p[OperationResult : byte](PublicKey : 32 bytes)`

### x | _Exporting private key:_

This option is disabled by default, to enable private key export entrypoint set `BUILD_ALLOW_PRIVATE_EXPORT` to 1

**input**: `x[Password : string]`

**output**: `x[OperationResult : byte](PrivateKey : 32 bytes)`

## Cryptography

The wallet uses Ed25519 signature scheme. The key is stored in encrypted form in EEPROM memory along with password seed and IV. The encryption method used is AES-256 with OFB mode. OFB mode is chosen due to the fact it cannot be parallelized for both decryption and encryption meaning to uncover full private key the attacker will have to act sequentially in any case. And because OFB uses exact same scheme for encryption and decryption we have some memory savings.

Though in the future if there's enough of program memory it's better to use GCM mode so we could store the tag in EEPROM and check for password correctness.

Encryption key is derived from password using custom KDF. Current KDF works such way that password is hashed with SHA2-512 and then truncated to 256 bits. Initially it was SHA3-256 but due to limitations for program memory it was decided to use SHA2-512 which is used in Ed25519. Though, since we truncate our digest we get length-extension resistance just like SHA3.

The future plan is to use PBKDF2-like scheme, since the target length is equal or less than digest size we can omit preparations and directly use [`F(P, S, c, 1)`](https://datatracker.ietf.org/doc/html/rfc2898#page-11).

Random number generation is done with built-in `random()` function with the seed defined by a product of multiplying all 12 (in Teensy 2.0) unconnected analog input values (analog noise) into one 64-bit number and then xoring both halves into one 32-bit random seed. This is done in `setup()`

All internal buffers storing sensitive information (raw password, hashed password a.k.a. encryption key and raw private key) are overwritten with 0s before being freed to prevent memory scan and essentially leaking the info.
