#include <Arduino.h>
#include "devices.hpp"

void generatePacket(const AppleDevice& device, uint8_t* buffer, size_t& outLength) {
  memset(buffer, 0, 31); // Clear buffer

  if (device.type == APPLE_AUDIO) {
      outLength = 31;
      uint8_t header[] = {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07};
      uint8_t body[]   = {0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12};
      
      memcpy(buffer, header, 7);
      buffer[7] = device.modelId;
      memcpy(buffer + 8, body, 11);
  } 
  else if (device.type == APPLE_SETUP) {
      outLength = 23;
      // The common 23-byte setup prefix
      uint8_t prefix[] = {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1};
      // The common 23-byte setup suffix (starting after index 13)
      uint8_t suffix[] = {0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};
      
      memcpy(buffer, prefix, 13);
      buffer[13] = device.modelId; // In "Short" packets, the ID is at index 13
      memcpy(buffer + 14, suffix, 9);
  }
}
