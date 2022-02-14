#include <EEPROM.h>
#include <AES.h>
#include <OFB.h>
#include <Ed25519.h>
#include <SHA512.h>

#pragma region utils
const int ledPin = 11;

void print_bytes(byte* buf, uint16_t len) {
  byte* buf_end = buf + len;
  for (; buf < buf_end; buf += 1) {
    // bruh this shit doesn't print bytes with padded leading 0
    //Serial.print(buf[i], HEX);

    byte b0 = *buf >> 4;
    byte b1 = *buf & 0xF;

    Serial.print(b0, HEX);
    Serial.print(b1, HEX);

    Serial.print(" ");
  }
  Serial.println();
}

/// For a Teensy 2.0, Address can be 0 to 1023
void read_bytes_mem(byte* buf, uint16_t len, uint16_t addr) {
  for (byte i = 0; i < len; i += 1, addr += 1) {
    buf[i] = EEPROM.read(addr);
  }
}
void write_bytes_mem(const byte* buf, uint16_t len, uint16_t addr) {
  for (byte i = 0; i < len; i += 1, addr += 1) {
    EEPROM.write(addr, buf[i]);
  }
}

#pragma endregion

// pepper to be used as salt. why pepper? because it's constantantly defined
// generated from random.org
// 16 ints from 0 to 255 incl.
const byte pepper[] = {
  170,  50,  253, 211,
  120, 175, 57,  181,
  49,  112, 223, 201,
  25,  11,  189, 16,
};


#pragma region func declarations

uint32_t generate_seed();

void store_key(byte*, char*);
void generate_key(char*);

#pragma endregion

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  uint32_t seed = generate_seed();
  randomSeed(seed);

  ///////////

  byte* input_key = new byte[32];
  memset(input_key, 0x10, 32);

  char* password = new char[256];
  memset(password, 0, 256);
  memcpy(password, "👽", 4);

  //store_key(input_key, password);
  generate_key(password);

  //sign_payload("🍆")
}

void loop() {
  // read is ended with 0x0a (10) = new line
  if (Serial.available() > 0) {
    //Serial.println(Serial.readString());
    byte operation = Serial.read();
    //Serial.print("USB received: ");
    //Serial.println(incomingByte, DEC);

    switch (operation) {
      case 0x01:
        //service op
        break;
      case 0x10:
      case 'e':
        {
          //read key and pass here

          //store_key(input_key, password);
        }
        break;
      case 'g'://generate key
        {
          char* password = new char[256];
          memset(password, 0, 256);
          memcpy(password, "test", 4);

          generate_key(password);
        }
        break;
      default:
        Serial.write(operation);
    }

    //Discard any received data that has not been read
    //Serial1.clear()

  }
}

void zkdf(char* password, byte* result) {
  uint16_t password_len = strlen(password);

  Serial.println(password);
  Serial.println(password_len);

  uint16_t payload_len = password_len + sizeof(pepper);
  byte* payload = new byte[payload_len];

  memcpy(payload, password, password_len);
  memcpy(payload + password_len, pepper, sizeof(pepper));

  //rip sha3
  //SHA3_256* hash = new SHA3_256();

  //so this is essentially SHA512/256
  SHA512* hash = new SHA512();

  hash->update(payload, payload_len);
  //nice we can truncate right in finalize()
  hash->finalize(result, 32);

  delete hash;


  /*
    BLAKE2s* blake = new BLAKE2s();

    byte digest_len = blake->hashSize();
    byte* digest = new byte[digest_len];

    blake->update(payload, payload_len);
    blake->finalize(digest, digest_len);

    delete blake;

    Serial.println(digest_len);
    print_bytes(digest, digest_len);

    //pinMode(ledPin, OUTPUT);
    //digitalWrite(ledPin, HIGH); // LOW = turn off
    //delay(1000); // ms

    Serial.write(0x01);
  */

}

// generate random seed from analog input fluctuations
// is it a possible vulnerability though if an attacker connects to the ports
// and mess with signal's voltage
// seed should be ulong = uint32_t
uint32_t generate_seed() {
  //Serial.print("analogs: ");
  //12 inputs
  //max is 1023
  uint64_t long_seed = 1;
  for (byte a = 0; a < 12; a += 1) {
    int val = analogRead(a);
    //Serial.print(val);
    //Serial.print(' ');

    //prevent multiplication by 0
    if (val)
      long_seed *= val;
  }
  //Serial.println(seed);
  //delay(250);

  return (long_seed >> 32) ^ (long_seed & uint32_t(-1));
}

void random_bytes(byte* buf, byte len) {
  for (byte b = 0; b < len; b += 1) {
    byte r = random();

    buf[b] = r;

    //Serial.print(r);
    //Serial.print(' ');
  }
}

/// clears input_key, password buffers after finished
/// password is \0 terminated string
void store_key(byte private_key[32], char* password) {
  
  Serial.print("Password bytes: ");
  print_bytes((byte*)password, 32);//it's actually 256 buf
  Serial.print("Password: ");
  Serial.println(password);
  
  byte* encryption_key = new byte[32];

  zkdf(password, encryption_key);
  memset(password, 0, 256);

  byte* iv = new byte[16];
  random_bytes(iv, 16);

  byte* encrypted = new byte[32];

  // BLOCK SIZE IS 128 BIT
  OFB<AES256>* aes = new OFB<AES256>();
  //AES256* aes = new AES256();
  aes->setKey(encryption_key, 32);
  aes->setIV(iv, 16);

  aes->encrypt(encrypted, private_key, 32);

  delete aes;

  //Serial.write(0x10);
  //Serial.println();

  Serial.print("Private key: ");
  print_bytes(private_key, 32);

  Serial.print("IV: ");
  print_bytes(iv, 16);

  Serial.print("Encrypted: ");
  print_bytes(encrypted, 32);

#pragma region decryption test

  aes = new OFB<AES256>();
  aes->setKey(encryption_key, 32);
  aes->setIV(iv, 16);

  byte* decrypted = new byte[32];
  aes->decrypt(decrypted, encrypted, 32);

  delete aes;

  Serial.print("Decrypted: ");
  print_bytes(decrypted, 32);
  delete[] decrypted;

#pragma endregion

  //clear key buffer with overwriting data
  //crypto lib automatically overwrites own internal buffers
  memset(private_key, 0, 32);
  memset(encryption_key, 0, 32);

  //@todo
  //write_bytes_mem(encrypted, 32, 0);

  delete[] password;
  delete[] encrypted;
  delete[] iv;
}

void sign_payload(byte private_key[32], byte* payload, uint16_t len){
  byte signature[64];
  Ed25519::sign(signature, private_key, private_key, payload, len);
}

void generate_key(char* password) {
  byte* private_key = new byte[32];
  byte* public_key = new byte[32];

  Ed25519::generatePrivateKey(private_key);
  Ed25519::derivePublicKey(public_key, public_key);

  Serial.print("Public key: ");
  print_bytes(public_key, 32);

  store_key(private_key, password);
}
