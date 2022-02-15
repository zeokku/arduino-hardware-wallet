#include <EEPROM.h>
#include <AES.h>
#include <OFB.h>
#include <Ed25519.h>
#include <SHA512.h>

#define PASSWORD_MAX_LEN 32

#define TEST_DECRYPTION_IN_STORE 0

/* storage
{SALT}[16]{IV}[16]{ENCRYPTED_KEY}[32]
*/

#define SALT_LEN 16
#define AES_BLOCK_LEN 16
#define IV_LEN 16
/// for both encryption and signature
#define KEY_LEN 32

#define SALT_ADDRESS 0
#define IV_ADDRESS SALT_ADDRESS + SALT_LEN
#define ENCRYPTED_KEY_ADDRESS IV_ADDRESS + IV_LEN
#define STORAGE_BLOCK_LEN ENCRYPTED_KEY_ADDRESS + KEY_LEN

#pragma region utils
const int ledPin = 11;

void print_bytes(byte *buf, uint16_t len)
{
  byte *buf_end = buf + len;
  for (; buf < buf_end; buf += 1)
  {
    // bruh this shit doesn't print bytes with padded leading 0
    // Serial.print(buf[i], HEX);

    byte b0 = *buf >> 4;
    byte b1 = *buf & 0xF;

    Serial.print(b0, HEX);
    Serial.print(b1, HEX);

    Serial.print(" ");
  }
  Serial.println();
}

// generate random seed from analog input fluctuations
// is it a possible vulnerability though if an attacker connects to the ports
// and mess with signal's voltage
// seed should be ulong = uint32_t
uint32_t generate_seed()
{
  // Serial.print("analogs: ");
  // 12 inputs
  // max is 1023
  uint64_t long_seed = 1;
  for (byte a = 0; a < 12; a += 1)
  {
    int val = analogRead(a);
    // Serial.print(val);
    // Serial.print(' ');

    // prevent multiplication by 0
    if (val)
      long_seed *= val;
  }
  // Serial.println(seed);
  // delay(250);

  return (long_seed >> 32) ^ (long_seed & uint32_t(-1));
}

/// For a Teensy 2.0, Address can be 0 to 1023
void mem_read_bytes(byte *buf, uint16_t len, uint16_t addr)
{
  for (uint16_t i = 0; i < len; i += 1, addr += 1)
  {
    buf[i] = EEPROM.read(addr);
  }
}
void mem_write_bytes(const byte *buf, uint16_t len, uint16_t addr)
{
  for (uint16_t i = 0; i < len; i += 1, addr += 1)
  {
    EEPROM.write(addr, buf[i]);
  }
}

#pragma endregion

#pragma region func declarations

bool read_password(char *&password);
void store_key(byte *private_key, char *password);
void generate_key(char *password);

#pragma endregion

void setup()
{
  Serial.begin(9600);

  uint32_t seed = generate_seed();
  randomSeed(seed);

  Serial.print("👽");
  Serial.println("🍆");
}

#pragma region outdated read_password
// this allocated a password buffer which is then cleaned and freed in store_key()
// read until end of input or \0
// REPLACED BY Serial.readBytesUntil()
// char* read_password() {
//   char* password = new char[PASSWORD_MAX_LEN];
//   memset(password, 0, PASSWORD_MAX_LEN);

//   //almost used 'byte' type here which could to problems since (byte)len is always < than 256
//   //due to overflow which would allow 'infinite' input and buffer overwriting
//   uint16_t p = 0;

//   int16_t in = Serial.read();

//   // if there's no input the password would essentially be '' empty string
//   // but we limit max len to 8

//   // @todo can the input be split and buffered onto multiple chunks?
//   // should we use \0 as the only sign of end of the input?
//   // while in != 0;
//   // { if(n != -1) { write } read }
//   // receive buffer holds up to 64 bytes: https://www.arduino.cc/reference/en/language/functions/communication/serial/available/

//   while ( !(in == -1 || in == 0) && p < PASSWORD_MAX_LEN ) {
//     password[p++] = in;

//     in = Serial.read();
//   }

//   if (p == PASSWORD_MAX_LEN) {
//     //@todo failwith: PASSWORD_OVER_MAX_LENGTH
//   }
//   else if (p < 8) {
//     //@todo failwith: PASSWORD_IS_TOO_SHORT
//   }

//   //Discard any received data that has not been read
//   //Serial1.clear()

//   return password;
// }
#pragma endregion

void loop()
{
  if (Serial.available() > 0)
  {
    byte operation = Serial.read();

    switch (operation)
    {
    case 'k': // store existing key // k[Password]\0[Key]
    {
      Serial.println("Store key");

      char *password = 0;

      if (!read_password(password))
      {
        Serial.println("Password is too short");
        return;
      }

      byte *private_key = new byte[32];
      memset(private_key, 0, 32);
      size_t key_len = Serial.readBytes(private_key, 32);

      if (key_len == 32)
      {
        store_key(private_key, password);
      }
      else
      {
        Serial.println("Invalid key");
      }
    }
    break;
    case 'g': // generate key // g[Password]
    {
      Serial.println("Generate key");

      char *password = 0;

      if (read_password(password))
      {
        generate_key(password);
      }
      else
      {
        Serial.println("Password is too short");
      }
    }
    break;
    case 's':
    { // sign payload // s[Password]\0[2bytes_PayloadLength][Payload]
      Serial.println("Sign payload");

      char *password = 0;

      if (!read_password(password))
      {
        Serial.println("Password is too short");
        return;
      }

      uint16_t payload_len = 0;

      if (Serial.readBytes((byte *)&payload_len, sizeof(uint16_t)) != 2)
      {
        Serial.println("Error reading payload length");
        return;
      }

      Serial.print("Payload size: ");
      Serial.println(payload_len);

      //@todo do we need to set some limit for payload size?
      byte *payload = new byte[payload_len];
      memset(payload, 0, payload_len);

      if (Serial.readBytes(payload, payload_len) != payload_len)
      {
        delete[] payload;

        Serial.println("Error reading payload");
        return;
      }

      sign_payload(password, payload, payload_len);
    }
    break;
    default:
      Serial.write(operation);
    }
  }
}

bool read_password(char *&password)
{
  password = new char[PASSWORD_MAX_LEN];
  memset(password, 0, PASSWORD_MAX_LEN);
  size_t password_len = Serial.readBytesUntil(0, password, PASSWORD_MAX_LEN);

  if (password_len < 8)
  {
    delete[] password;
    return false;
  }

  return true;
}

void zkdf(char *password, byte salt[SALT_LEN], byte result[KEY_LEN])
{
  uint16_t password_len = strlen(password);

  // Serial.print("Password len: ");
  // Serial.println(password_len);

  uint16_t payload_len = password_len + SALT_LEN;
  byte *payload = new byte[payload_len];

  memcpy(payload, password, password_len);
  memcpy(payload + password_len, salt, SALT_LEN);

  // rip sha3
  // SHA3_256* hash = new SHA3_256();

  // so this is essentially SHA512/256
  SHA512 *hash = new SHA512();

  hash->update(payload, payload_len);

  delete[] payload;

  // nice we can truncate right in finalize()
  hash->finalize(result, 32);

  delete hash;

  /*
    //pinMode(ledPin, OUTPUT);
    //digitalWrite(ledPin, HIGH); // LOW = turn off
    //delay(1000); // ms

    Serial.write(0x01);
  */
}

void random_bytes(byte *buf, byte len)
{
  for (byte b = 0; b < len; b += 1)
  {
    byte r = random();

    buf[b] = r;

    // Serial.print(r);
    // Serial.print(' ');
  }
}

/// clears private_key, password buffers after finished
/// password should be \0 terminated string
void store_key(byte private_key[KEY_LEN], char password[PASSWORD_MAX_LEN])
{

  // Serial.print("Password bytes: ");
  // print_bytes((byte *)password, PASSWORD_MAX_LEN);
  // Serial.print("Password: ");
  // Serial.println(password);

  byte *encryption_key = new byte[KEY_LEN];

  byte *salt = new byte[SALT_LEN]; // double min pwd len
  random_bytes(salt, SALT_LEN);

  // Serial.print("Salt: ");
  // print_bytes(salt, SALT_LEN);

  zkdf(password, salt, encryption_key);

  mem_write_bytes(salt, SALT_LEN, SALT_ADDRESS);
  delete[] salt;

  memset(password, 0, PASSWORD_MAX_LEN);
  delete[] password;

  // block size
  byte *iv = new byte[IV_LEN];
  random_bytes(iv, IV_LEN);

  byte *encrypted = new byte[KEY_LEN];

  // BLOCK SIZE IS 128 BIT
  OFB<AES256> *aes = new OFB<AES256>();
  // AES256* aes = new AES256();
  aes->setKey(encryption_key, KEY_LEN);
  aes->setIV(iv, IV_LEN);

  aes->encrypt(encrypted, private_key, KEY_LEN);

  delete aes;

  // Serial.print("IV: ");
  // print_bytes(iv, 16);
  //
  // Serial.print("Private key: ");
  // print_bytes(private_key, 32);
  //
  // Serial.print("Encrypted:   ");
  // print_bytes(encrypted, 32);

#if TEST_DECRYPTION_IN_STORE

  aes = new OFB<AES256>();
  aes->setKey(encryption_key, 32);
  aes->setIV(iv, 16);

  byte *decrypted = new byte[32];
  aes->decrypt(decrypted, encrypted, 32);

  delete aes;

  Serial.print("Decrypted:   ");
  print_bytes(decrypted, 32);
  delete[] decrypted;
#endif

  // clear key buffer with overwriting data
  // crypto lib automatically overwrites own internal buffers
  memset(private_key, 0, KEY_LEN);
  memset(encryption_key, 0, KEY_LEN);

  mem_write_bytes(iv, IV_LEN, IV_ADDRESS);
  delete[] iv;

  mem_write_bytes(encrypted, KEY_LEN, ENCRYPTED_KEY_ADDRESS);
  delete[] encrypted;
}

void retrieve_key(byte private_key[KEY_LEN], char password[PASSWORD_MAX_LEN])
{
  byte *encryption_key = new byte[KEY_LEN];

  byte *salt = new byte[SALT_LEN];
  mem_read_bytes(salt, SALT_LEN, SALT_ADDRESS);

  zkdf(password, salt, encryption_key);

  delete[] salt;

  memset(password, 0, PASSWORD_MAX_LEN);
  delete[] password;

  byte *iv = new byte[IV_LEN];
  mem_read_bytes(iv, IV_LEN, IV_ADDRESS);

  byte *encrypted = new byte[KEY_LEN];
  mem_read_bytes(encrypted, KEY_LEN, ENCRYPTED_KEY_ADDRESS);

  OFB<AES256> *aes = new OFB<AES256>();

  aes->setKey(encryption_key, KEY_LEN);
  aes->setIV(iv, IV_LEN);
  aes->decrypt(private_key, encrypted, KEY_LEN);

  delete aes;

  memset(encryption_key, 0, KEY_LEN);
  delete[] encryption_key;
}

void sign_payload(char password[PASSWORD_MAX_LEN], byte *payload, uint16_t payload_len)
{
  byte *private_key = new byte[KEY_LEN];
  retrieve_key(private_key, password);

  Serial.print('Private key retrieved: ');
  print_bytes(private_key, KEY_LEN);

  //

  byte *public_key = new byte[KEY_LEN];
  Ed25519::derivePublicKey(public_key, private_key);

  Serial.print("Public key: ");
  print_bytes(public_key, KEY_LEN);

  byte *signature = new byte[64];
  Ed25519::sign(signature, private_key, public_key, payload, payload_len);

  memset(private_key, 0, KEY_LEN);
  delete[] private_key;

  delete[] public_key;

  Serial.print("Signature: ");
  print_bytes(signature, 64);

  delete[] signature;
}

void generate_key(char password[PASSWORD_MAX_LEN])
{
  byte *private_key = new byte[KEY_LEN];

  Ed25519::generatePrivateKey(private_key);

  Serial.print('Private key: ');
  print_bytes(private_key, KEY_LEN);

  store_key(private_key, password);

  //

  byte *public_key = new byte[KEY_LEN];
  Ed25519::derivePublicKey(public_key, private_key);

  Serial.write(public_key, KEY_LEN);

  Serial.print("Public key: ");
  print_bytes(public_key, KEY_LEN);

  delete[] public_key;
}
