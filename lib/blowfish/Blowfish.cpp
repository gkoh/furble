/**
 * Blowfish Encryption Algorithm.
 *
 * Originally obtained on 11-Feb-2025 from:
 * https://www.schneier.com/wp-content/uploads/2015/12/bfsh-sch.zip
 *
 * Contents have been adapted to suit the requirements of the furble project.
 */
#include "Blowfish.h"

namespace Furble {

Blowfish::Blowfish(const std::vector<uint8_t> key) {
  uint32_t data;
  uint16_t j = 0;

  for (int i = 0; i < N + 2; ++i) {
    data = 0x00000000;
    for (int k = 0; k < 4; ++k) {
      data = (data << 8) | key[j];
      j = j + 1;
      if (j >= key.size()) {
        j = 0;
      }
    }
    m_P[i] = m_P[i] ^ data;
  }

  uint32_t datal = 0x00000000;
  uint32_t datar = 0x00000000;

  for (int i = 0; i < N + 2; i += 2) {
    encipher(&datal, &datar);

    m_P[i] = datal;
    m_P[i + 1] = datar;
  }

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 256; j += 2) {
      encipher(&datal, &datar);

      m_SBox[i][j] = datal;
      m_SBox[i][j + 1] = datar;
    }
  }
}

uint32_t Blowfish::f(uint32_t x) {
  uint8_t a;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint32_t y;

  d = x & 0xFF;
  x >>= 8;
  c = x & 0xFF;
  x >>= 8;
  b = x & 0xFF;
  x >>= 8;
  a = x & 0xFF;

  y = m_SBox[0][a] + m_SBox[1][b];
  y = y ^ m_SBox[2][c];
  y = y + m_SBox[3][d];

  return y;
}

void Blowfish::encipher(uint32_t *xl, uint32_t *xr) {
  uint32_t Xl;
  uint32_t Xr;
  uint32_t temp;

  Xl = *xl;
  Xr = *xr;

  for (int i = 0; i < N; ++i) {
    Xl = Xl ^ m_P[i];
    Xr = f(Xl) ^ Xr;

    temp = Xl;
    Xl = Xr;
    Xr = temp;
  }

  temp = Xl;
  Xl = Xr;
  Xr = temp;

  Xr = Xr ^ m_P[N];
  Xl = Xl ^ m_P[N + 1];

  *xl = Xl;
  *xr = Xr;
}
}  // namespace Furble
