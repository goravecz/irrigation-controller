#include "HunterRoam.h"

HunterRoam::HunterRoam(int pin) {
  _pin = pin;
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
}

String HunterRoam::errorHint(byte error) {
  switch (error) {
    case 0: return "OK";
    case 1: return "Invalid zone number";
    case 2: return "Invalid watering time";
    case 3: return "Invalid program number";
    default: return "Unknown error";
  }
}

void HunterRoam::sendLow() {
  digitalWrite(_pin, HIGH);
  delayMicroseconds(SHORT_INTERVAL);
  digitalWrite(_pin, LOW);
  delayMicroseconds(LONG_INTERVAL);
}

void HunterRoam::sendHigh() {
  digitalWrite(_pin, HIGH);
  delayMicroseconds(LONG_INTERVAL);
  digitalWrite(_pin, LOW);
  delayMicroseconds(SHORT_INTERVAL);
}

void HunterRoam::hunterBitfield(std::vector<byte>& bits, byte pos, byte val, byte len) {
  for (byte i = 0; i < len; i++) {
    bits[pos + i] = (val >> i) & 1;
  }
}

void HunterRoam::writeBus(std::vector<byte> buffer, bool extrabit) {
  // Reset impulse
  digitalWrite(_pin, HIGH);
  delay(325);
  digitalWrite(_pin, LOW);
  delay(65);

  // Start impulse
  digitalWrite(_pin, HIGH);
  delayMicroseconds(START_INTERVAL);
  digitalWrite(_pin, LOW);
  delayMicroseconds(SHORT_INTERVAL);

  // Transmit bits
  for (byte b : buffer) {
    if (b) sendHigh();
    else   sendLow();
  }

  if (extrabit) sendHigh();

  // Stop
  digitalWrite(_pin, LOW);
}

byte HunterRoam::stopZone(byte zone) {
  return startZone(zone, 0);
}

byte HunterRoam::startZone(byte zone, byte time) {
  if (zone < 1 || zone > 48) return 1;
  if (time > 240) return 2;

  std::vector<byte> bits(120, 0);

  // Zone (0-indexed) repeated 6 times
  hunterBitfield(bits, 23, zone - 1, 6);
  hunterBitfield(bits, 36, zone - 1, 6);
  hunterBitfield(bits, 49, zone - 1, 6);
  hunterBitfield(bits, 62, zone - 1, 6);
  hunterBitfield(bits, 75, zone - 1, 6);
  hunterBitfield(bits, 88, zone - 1, 6);

  // Duration split into nibbles, 3 repetitions of (high nibble, low nibble)
  hunterBitfield(bits, 31, (time >> 4) & 0xF, 4);
  hunterBitfield(bits, 44,  time & 0xF,        4);
  hunterBitfield(bits, 57, (time >> 4) & 0xF, 4);
  hunterBitfield(bits, 70,  time & 0xF,        4);
  hunterBitfield(bits, 83, (time >> 4) & 0xF, 4);
  hunterBitfield(bits, 96,  time & 0xF,        4);

  writeBus(bits, true);
  return 0;
}

byte HunterRoam::startProgram(byte num) {
  if (num < 1 || num > 4) return 3;

  std::vector<byte> bits(56, 0);
  hunterBitfield(bits, 31, num - 1, 2);
  writeBus(bits, false);
  return 0;
}
