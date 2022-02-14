#include <EEPROM.h>
#include <AES.h>
#include <OFB.h>
#include <BLAKE2s.h>

const int ledPin = 11;


void print_bytes(const byte* buf, byte len) {
  for (byte i = 0; i < len; i += 1) {
    // bruh this shit doesn't print bytes with padded leading 0
    //Serial.print(buf[i], HEX);

    byte b0 = buf[i] >> 4;
    byte b1 = buf[i] & 0xF;

    Serial.print(b0, HEX);
    Serial.print(b1, HEX);

    Serial.print(" ");
  }
  Serial.println();
}

/// For a Teensy 2.0, Address can be 0 to 1023
void read_bytes_mem(byte* buf, byte len, uint16_t addr) {
  for (byte i = 0; i < len; i += 1, addr += 1) {
    buf[i] = EEPROM.read(addr);
  }
}
void write_bytes_mem(const byte* buf, byte len, uint16_t addr) {
  for (byte i = 0; i < len; i += 1, addr += 1) {
    EEPROM.write(addr, buf[i]);
  }
}

// pepper to be used as salt. why pepper? because it's constantantly defined
// generated from random.org
// 16 ints from 0 to 255 incl.
const byte pepper[] = {
  170,  50,  253, 211,
  120, 175, 57,  181,
  49,  112, 223, 201,
  25,  11,  189, 16,
};

uint32_t generate_seed();
void hash();
void store_key();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  Serial.printlnuint32_t(-1));

  uint32_t seed = generate_seed();
  randomSeed(seed);

  //Serial.print("seed: ");
  //Serial.println(seed);

  

  //229
  //187 160 94 171 133 68 8 20 175 243 44 17 223 252 83 43
  //Serial.println(random(256));

  //byte* iv = new byte[16];
  //random_bytes(iv, 16);

  //no idea why but stupid ass serial js lib couldn't properly read shit from here bruh

  //store_key();
}

void hash() {

  char password[] = "test string";
  int password_len = sizeof(password) - 1; // sub 1 for \0

  Serial.println(password);
  Serial.println(password_len);

  byte payload_len = password_len + sizeof(pepper);
  byte* payload = new byte[payload_len];
  memcpy(payload, password, password_len);
  memcpy(payload + password_len, pepper, sizeof(pepper));

  //print_bytes((byte*)password, password_len);
  //print_bytes(pepper, sizeof(pepper));
  //print_bytes(payload, password_len + sizeof(pepper));

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

void random_bytes(const byte* buf, byte len) {
  for (byte b = 0; b < len; b += 1) {
    byte r = random();
    
    buf[b] = r;
    
    //Serial.print(r);
    //Serial.print(' ');
  }
}


void store_key() {
  byte* input_key = new byte[32];
  memset(input_key, 0x00, 32);
  byte* password = new byte[32];
  memset(password, 0x00, 32);
  password[0] = 0x80;

  byte* encrypted = new byte[32];

  byte* iv = new byte[16];
  random_bytes(iv, 16);

  // BLOCK SIZE IS 128 BIT
  OFB<AES256>* aes = new OFB<AES256>();
  //AES256* aes = new AES256();
  aes->setKey(password, 32);
  aes->setIV(iv, 16);

  aes->encrypt(encrypted, input_key, 32);

  //    aes->encryptBlock(encrypted, input_key);
  //    aes->encryptBlock(encrypted + 16, input_key + 16);

  Serial.write(0x10);

  Serial.write(encrypted, 32);
  Serial.write(input_key, 32);
  Serial.write(password, 32);

  Serial.println();

  print_bytes(iv, 16);
  print_bytes(encrypted, 32);
  print_bytes(input_key, 32);
  print_bytes(password, 32);

  delete aes;

  //clear key buffer with overwriting data
  //crypto lib automatically overwrites own internal buffers
  memset(input_key, 0, 32);
  memset(password, 0, 32);


  delete[] input_key;
  delete[] password;
  delete[] encrypted;
  delete[] iv;
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
        hash();
        break;
      case 0x10:
      case 'e':
        store_key();
        break;
      default:
        Serial.write(operation);
    }

    //Discard any received data that has not been read
    //Serial1.clear()

  }




}
