#include <boost/ut.hpp>
#include <vector>

#include "crypto/sha3.hpp"

int main() {
  // https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/SHAKE128_Msg0.pdf
  // https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/SHAKE128_Msg1600.pdf
  auto msg0 = std::vector<uint8_t>{};
  auto msg1600 = std::vector<uint8_t>{
      0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3,
      0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3,
      0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3,
      0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3,
      0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3,
      0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3,
      0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3,
      0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3,
      0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3,
      0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3,
      0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3,
      0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3,
      0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3,
      0xA3, 0xA3, 0xA3, 0xA3, 0xA3};

  auto exp0 = std::vector<uint8_t>{
      0x7F, 0x9C, 0x2B, 0xA4, 0xE8, 0x8F, 0x82, 0x7D, 0x61, 0x60, 0x45, 0x50, 0x76, 0x05, 0x85,
      0x3E, 0xD7, 0x3B, 0x80, 0x93, 0xF6, 0xEF, 0xBC, 0x88, 0xEB, 0x1A, 0x6E, 0xAC, 0xFA, 0x66,
      0xEF, 0x26, 0x3C, 0xB1, 0xEE, 0xA9, 0x88, 0x00, 0x4B, 0x93, 0x10, 0x3C, 0xFB, 0x0A, 0xEE,
      0xFD, 0x2A, 0x68, 0x6E, 0x01, 0xFA, 0x4A, 0x58, 0xE8, 0xA3, 0x63, 0x9C, 0xA8, 0xA1, 0xE3,
      0xF9, 0xAE, 0x57, 0xE2, 0x35, 0xB8, 0xCC, 0x87, 0x3C, 0x23, 0xDC, 0x62, 0xB8, 0xD2, 0x60,
      0x16, 0x9A, 0xFA, 0x2F, 0x75, 0xAB, 0x91, 0x6A, 0x58, 0xD9, 0x74, 0x91, 0x88, 0x35, 0xD2,
      0x5E, 0x6A, 0x43, 0x50, 0x85, 0xB2, 0xBA, 0xDF, 0xD6, 0xDF, 0xAA, 0xC3, 0x59, 0xA5, 0xEF,
      0xBB, 0x7B, 0xCC, 0x4B, 0x59, 0xD5, 0x38, 0xDF, 0x9A, 0x04, 0x30, 0x2E, 0x10, 0xC8, 0xBC,
      0x1C, 0xBF, 0x1A, 0x0B, 0x3A, 0x51, 0x20, 0xEA, 0x17, 0xCD, 0xA7, 0xCF, 0xAD, 0x76, 0x5F,
      0x56, 0x23, 0x47, 0x4D, 0x36, 0x8C, 0xCC, 0xA8, 0xAF, 0x00, 0x07, 0xCD, 0x9F, 0x5E, 0x4C,
      0x84, 0x9F, 0x16, 0x7A, 0x58, 0x0B, 0x14, 0xAA, 0xBD, 0xEF, 0xAE, 0xE7, 0xEE, 0xF4, 0x7C,
      0xB0, 0xFC, 0xA9, 0x76, 0x7B, 0xE1, 0xFD, 0xA6, 0x94, 0x19, 0xDF, 0xB9, 0x27, 0xE9, 0xDF,
      0x07, 0x34, 0x8B, 0x19, 0x66, 0x91, 0xAB, 0xAE, 0xB5, 0x80, 0xB3, 0x2D, 0xEF, 0x58, 0x53,
      0x8B, 0x8D, 0x23, 0xF8, 0x77, 0x32, 0xEA, 0x63, 0xB0, 0x2B, 0x4F, 0xA0, 0xF4, 0x87, 0x33,
      0x60, 0xE2, 0x84, 0x19, 0x28, 0xCD, 0x60, 0xDD, 0x4C, 0xEE, 0x8C, 0xC0, 0xD4, 0xC9, 0x22,
      0xA9, 0x61, 0x88, 0xD0, 0x32, 0x67, 0x5C, 0x8A, 0xC8, 0x50, 0x93, 0x3C, 0x7A, 0xFF, 0x15,
      0x33, 0xB9, 0x4C, 0x83, 0x4A, 0xDB, 0xB6, 0x9C, 0x61, 0x15, 0xBA, 0xD4, 0x69, 0x2D, 0x86,
      0x19, 0xF9, 0x0B, 0x0C, 0xDF, 0x8A, 0x7B, 0x9C, 0x26, 0x40, 0x29, 0xAC, 0x18, 0x5B, 0x70,
      0xB8, 0x3F, 0x28, 0x01, 0xF2, 0xF4, 0xB3, 0xF7, 0x0C, 0x59, 0x3E, 0xA3, 0xAE, 0xEB, 0x61,
      0x3A, 0x7F, 0x1B, 0x1D, 0xE3, 0x3F, 0xD7, 0x50, 0x81, 0xF5, 0x92, 0x30, 0x5F, 0x2E, 0x45,
      0x26, 0xED, 0xC0, 0x96, 0x31, 0xB1, 0x09, 0x58, 0xF4, 0x64, 0xD8, 0x89, 0xF3, 0x1B, 0xA0,
      0x10, 0x25, 0x0F, 0xDA, 0x7F, 0x13, 0x68, 0xEC, 0x29, 0x67, 0xFC, 0x84, 0xEF, 0x2A, 0xE9,
      0xAF, 0xF2, 0x68, 0xE0, 0xB1, 0x70, 0x0A, 0xFF, 0xC6, 0x82, 0x0B, 0x52, 0x3A, 0x3D, 0x91,
      0x71, 0x35, 0xF2, 0xDF, 0xF2, 0xEE, 0x06, 0xBF, 0xE7, 0x2B, 0x31, 0x24, 0x72, 0x1D, 0x4A,
      0x26, 0xC0, 0x4E, 0x53, 0xA7, 0x5E, 0x30, 0xE7, 0x3A, 0x7A, 0x9C, 0x4A, 0x95, 0xD9, 0x1C,
      0x55, 0xD4, 0x95, 0xE9, 0xF5, 0x1D, 0xD0, 0xB5, 0xE9, 0xD8, 0x3C, 0x6D, 0x5E, 0x8C, 0xE8,
      0x03, 0xAA, 0x62, 0xB8, 0xD6, 0x54, 0xDB, 0x53, 0xD0, 0x9B, 0x8D, 0xCF, 0xF2, 0x73, 0xCD,
      0xFE, 0xB5, 0x73, 0xFA, 0xD8, 0xBC, 0xD4, 0x55, 0x78, 0xBE, 0xC2, 0xE7, 0x70, 0xD0, 0x1E,
      0xFD, 0xE8, 0x6E, 0x72, 0x1A, 0x3F, 0x7C, 0x6C, 0xCE, 0x27, 0x5D, 0xAB, 0xE6, 0xE2, 0x14,
      0x3F, 0x1A, 0xF1, 0x8D, 0xA7, 0xEF, 0xDD, 0xC4, 0xC7, 0xB7, 0x0B, 0x5E, 0x34, 0x5D, 0xB9,
      0x3C, 0xC9, 0x36, 0xBE, 0xA3, 0x23, 0x49, 0x1C, 0xCB, 0x38, 0xA3, 0x88, 0xF5, 0x46, 0xA9,
      0xFF, 0x00, 0xDD, 0x4E, 0x13, 0x00, 0xB9, 0xB2, 0x15, 0x3D, 0x20, 0x41, 0xD2, 0x05, 0xB4,
      0x43, 0xE4, 0x1B, 0x45, 0xA6, 0x53, 0xF2, 0xA5, 0xC4, 0x49, 0x2C, 0x1A, 0xDD, 0x54, 0x45,
      0x12, 0xDD, 0xA2, 0x52, 0x98, 0x33, 0x46, 0x2B, 0x71, 0xA4, 0x1A, 0x45, 0xBE, 0x97, 0x29,
      0x0B, 0x6F};
  auto exp1600 = std::vector<uint8_t>{
      0x13, 0x1A, 0xB8, 0xD2, 0xB5, 0x94, 0x94, 0x6B, 0x9C, 0x81, 0x33, 0x3F, 0x9B, 0xB6, 0xE0,
      0xCE, 0x75, 0xC3, 0xB9, 0x31, 0x04, 0xFA, 0x34, 0x69, 0xD3, 0x91, 0x74, 0x57, 0x38, 0x5D,
      0xA0, 0x37, 0xCF, 0x23, 0x2E, 0xF7, 0x16, 0x4A, 0x6D, 0x1E, 0xB4, 0x48, 0xC8, 0x90, 0x81,
      0x86, 0xAD, 0x85, 0x2D, 0x3F, 0x85, 0xA5, 0xCF, 0x28, 0xDA, 0x1A, 0xB6, 0xFE, 0x34, 0x38,
      0x17, 0x19, 0x78, 0x46, 0x7F, 0x1C, 0x05, 0xD5, 0x8C, 0x7E, 0xF3, 0x8C, 0x28, 0x4C, 0x41,
      0xF6, 0xC2, 0x22, 0x1A, 0x76, 0xF1, 0x2A, 0xB1, 0xC0, 0x40, 0x82, 0x66, 0x02, 0x50, 0x80,
      0x22, 0x94, 0xFB, 0x87, 0x18, 0x02, 0x13, 0xFD, 0xEF, 0x5B, 0x0E, 0xCB, 0x7D, 0xF5, 0x0C,
      0xA1, 0xF8, 0x55, 0x5B, 0xE1, 0x4D, 0x32, 0xE1, 0x0F, 0x6E, 0xDC, 0xDE, 0x89, 0x2C, 0x09,
      0x42, 0x4B, 0x29, 0xF5, 0x97, 0xAF, 0xC2, 0x70, 0xC9, 0x04, 0x55, 0x6B, 0xFC, 0xB4, 0x7A,
      0x7D, 0x40, 0x77, 0x8D, 0x39, 0x09, 0x23, 0x64, 0x2B, 0x3C, 0xBD, 0x05, 0x79, 0xE6, 0x09,
      0x08, 0xD5, 0xA0, 0x00, 0xC1, 0xD0, 0x8B, 0x98, 0xEF, 0x93, 0x3F, 0x80, 0x64, 0x45, 0xBF,
      0x87, 0xF8, 0xB0, 0x09, 0xBA, 0x9E, 0x94, 0xF7, 0x26, 0x61, 0x22, 0xED, 0x7A, 0xC2, 0x4E,
      0x5E, 0x26, 0x6C, 0x42, 0xA8, 0x2F, 0xA1, 0xBB, 0xEF, 0xB7, 0xB8, 0xDB, 0x00, 0x66, 0xE1,
      0x6A, 0x85, 0xE0, 0x49, 0x3F, 0x07, 0xDF, 0x48, 0x09, 0xAE, 0xC0, 0x84, 0xA5, 0x93, 0x74,
      0x8A, 0xC3, 0xDD, 0xE5, 0xA6, 0xD7, 0xAA, 0xE1, 0xE8, 0xB6, 0xE5, 0x35, 0x2B, 0x2D, 0x71,
      0xEF, 0xBB, 0x47, 0xD4, 0xCA, 0xEE, 0xD5, 0xE6, 0xD6, 0x33, 0x80, 0x5D, 0x2D, 0x32, 0x3E,
      0x6F, 0xD8, 0x1B, 0x46, 0x84, 0xB9, 0x3A, 0x26, 0x77, 0xD4, 0x5E, 0x74, 0x21, 0xC2, 0xC6,
      0xAE, 0xA2, 0x59, 0xB8, 0x55, 0xA6, 0x98, 0xFD, 0x7D, 0x13, 0x47, 0x7A, 0x1F, 0xE5, 0x3E,
      0x5A, 0x4A, 0x61, 0x97, 0xDB, 0xEC, 0x5C, 0xE9, 0x5F, 0x50, 0x5B, 0x52, 0x0B, 0xCD, 0x95,
      0x70, 0xC4, 0xA8, 0x26, 0x5A, 0x7E, 0x01, 0xF8, 0x9C, 0x0C, 0x00, 0x2C, 0x59, 0xBF, 0xEC,
      0x6C, 0xD4, 0xA5, 0xC1, 0x09, 0x25, 0x89, 0x53, 0xEE, 0x5E, 0xE7, 0x0C, 0xD5, 0x77, 0xEE,
      0x21, 0x7A, 0xF2, 0x1F, 0xA7, 0x01, 0x78, 0xF0, 0x94, 0x6C, 0x9B, 0xF6, 0xCA, 0x87, 0x51,
      0x79, 0x34, 0x79, 0xF6, 0xB5, 0x37, 0x73, 0x7E, 0x40, 0xB6, 0xED, 0x28, 0x51, 0x1D, 0x8A,
      0x2D, 0x7E, 0x73, 0xEB, 0x75, 0xF8, 0xDA, 0xAC, 0x91, 0x2F, 0xF9, 0x06, 0xE0, 0xAB, 0x95,
      0x5B, 0x08, 0x3B, 0xAC, 0x45, 0xA8, 0xE5, 0xE9, 0xB7, 0x44, 0xC8, 0x50, 0x6F, 0x37, 0xE9,
      0xB4, 0xE7, 0x49, 0xA1, 0x84, 0xB3, 0x0F, 0x43, 0xEB, 0x18, 0x8D, 0x85, 0x5F, 0x1B, 0x70,
      0xD7, 0x1F, 0xF3, 0xE5, 0x0C, 0x53, 0x7A, 0xC1, 0xB0, 0xF8, 0x97, 0x4F, 0x0F, 0xE1, 0xA6,
      0xAD, 0x29, 0x5B, 0xA4, 0x2F, 0x6A, 0xEC, 0x74, 0xD1, 0x23, 0xA7, 0xAB, 0xED, 0xDE, 0x6E,
      0x2C, 0x07, 0x11, 0xCA, 0xB3, 0x6B, 0xE5, 0xAC, 0xB1, 0xA5, 0xA1, 0x1A, 0x4B, 0x1D, 0xB0,
      0x8B, 0xA6, 0x98, 0x2E, 0xFC, 0xCD, 0x71, 0x69, 0x29, 0xA7, 0x74, 0x1C, 0xFC, 0x63, 0xAA,
      0x44, 0x35, 0xE0, 0xB6, 0x9A, 0x90, 0x63, 0xE8, 0x80, 0x79, 0x5C, 0x3D, 0xC5, 0xEF, 0x32,
      0x72, 0xE1, 0x1C, 0x49, 0x7A, 0x91, 0xAC, 0xF6, 0x99, 0xFE, 0xFE, 0xE2, 0x06, 0x22, 0x7A,
      0x44, 0xC9, 0xFB, 0x35, 0x9F, 0xD5, 0x6A, 0xC0, 0xA9, 0xA7, 0x5A, 0x74, 0x3C, 0xFF, 0x68,
      0x62, 0xF1, 0x7D, 0x72, 0x59, 0xAB, 0x07, 0x52, 0x16, 0xC0, 0x69, 0x95, 0x11, 0x64, 0x3B,
      0x64, 0x39};

  std::vector<uint8_t> output(512);

  crypto::SHAKE128::hash(output, msg0);
  boost::ut::expect(output == exp0);

  crypto::SHAKE128::hash(output, msg1600);
  boost::ut::expect(output == exp1600);

  return 0;
}