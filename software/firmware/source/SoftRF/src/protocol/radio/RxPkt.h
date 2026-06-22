#ifndef PROTOCOL_RXPKT_H
#define PROTOCOL_RXPKT_H

#include <stdint.h>
#include <string.h>

#include <ads-l.h>
#include <bitcount.h>
#include <ldpc.h>

enum {
  SOFTRF_RX_SYS_FLR      = 0,
  SOFTRF_RX_SYS_OGN      = 1,
  SOFTRF_RX_SYS_ADSL     = 2,
  SOFTRF_RX_SYS_FNT      = 4,
  SOFTRF_RX_SYS_LDR      = 5,
  SOFTRF_RX_SYS_HDR      = 6,
  SOFTRF_RX_SYS_NONE     = 7,
  SOFTRF_RX_SYS_FLR_ADSL = 8,
  SOFTRF_RX_SYS_OGN_ADSL = 9
};

class SoftRF_RxPacket {
 public:
  static const uint8_t MaxBytes = 64;

  uint8_t SysID = SOFTRF_RX_SYS_NONE;
  uint8_t Bytes = 0;
  uint8_t Channel = 0;
  uint8_t RSSI = 0;
  bool Manchester = false;
  bool Correct = false;
  uint8_t Data[MaxBytes + 1];
  uint8_t Err[MaxBytes + 1];

  void Clear()
  {
    SysID = SOFTRF_RX_SYS_NONE;
    Bytes = 0;
    Channel = 0;
    RSSI = 0;
    Manchester = false;
    Correct = false;
    memset(Data, 0, sizeof(Data));
    memset(Err, 0, sizeof(Err));
  }

  void Set(uint8_t sys_id, const uint8_t *data, const uint8_t *err,
           uint8_t bytes, bool manchester, uint8_t channel, int8_t rssi)
  {
    Clear();
    SysID = sys_id;
    Bytes = bytes > MaxBytes ? MaxBytes : bytes;
    Manchester = manchester;
    Channel = channel;
    RSSI = (uint8_t) (-2 * rssi);
    memcpy(Data, data, Bytes);
    if (err)
      memcpy(Err, err, Bytes);
  }

  static uint8_t DiffBits(const uint8_t *data, const uint8_t *ref,
                          const uint8_t *mask, uint8_t len)
  {
    uint8_t count = 0;
    for (uint8_t idx = 0; idx < len; ++idx)
      count += Count1s((uint8_t) ((data[idx] ^ ref[idx]) & mask[idx]));
    return count;
  }

  static uint8_t ByteShift(uint8_t *data, uint8_t bytes, uint8_t shift)
  {
    if (bytes <= shift)
      return 0;
    bytes -= shift;
    memmove(data, data + shift, bytes);
    return bytes;
  }

  static uint8_t BitShift(uint8_t *data, uint8_t bytes, uint8_t shift)
  {
    if (shift == 0)
      return bytes;

    uint8_t byte_offset = shift >> 3;
    shift &= 0x07;
    if (bytes <= byte_offset)
      return 0;

    if (shift == 0)
      return ByteShift(data, bytes, byte_offset);

    bytes -= byte_offset;
    if (bytes <= 1)
      return 0;

    uint8_t out_bytes = bytes - 1;
    uint8_t complement = 8 - shift;
    for (uint8_t idx = 0; idx < out_bytes; ++idx) {
      uint8_t byte = data[byte_offset + idx];
      uint8_t next = data[byte_offset + idx + 1];
      data[idx] = (byte << shift) | (next >> complement);
    }
    return out_bytes;
  }

  void BitShift(uint8_t bits)
  {
    BitShift(Data, Bytes, bits);
    Bytes = BitShift(Err, Bytes, bits);
  }

  uint8_t DecodeSysID()
  {
    if (SysID == SOFTRF_RX_SYS_OGN_ADSL) {
      static const uint8_t SignADSL[3] = {0x24, 0xB1, 0x80};
      static const uint8_t MaskADSL[3] = {0xFF, 0xFF, 0xF0};
      static const uint8_t SignOGN[3]  = {0x9B, 0x2B, 0x60};
      static const uint8_t MaskOGN[3]  = {0xFF, 0xFF, 0xF8};

      if (DiffBits(Data, SignADSL, MaskADSL, 3) <= 1) {
        BitShift(20);
        Bytes = 24;
        SysID = SOFTRF_RX_SYS_ADSL;
      } else if (DiffBits(Data, SignOGN, MaskOGN, 3) <= 1) {
        BitShift(21);
        Bytes = 26;
        SysID = SOFTRF_RX_SYS_OGN;
      }
      return SysID;
    }

    if (SysID == SOFTRF_RX_SYS_FLR_ADSL) {
      static const uint8_t SignADSL[3] = {0xE4, 0x96, 0x30};
      static const uint8_t MaskADSL[3] = {0xFF, 0xFF, 0xFE};
      static const uint8_t SignFLR[3]  = {0x63, 0xF5, 0x6C};
      static const uint8_t MaskFLR[3]  = {0xFF, 0xFF, 0xFE};

      if (DiffBits(Data, SignADSL, MaskADSL, 3) <= 1) {
        BitShift(23);
        Bytes = 24;
        SysID = SOFTRF_RX_SYS_ADSL;
      } else if (DiffBits(Data, SignFLR, MaskFLR, 3) <= 1) {
        BitShift(23);
        Bytes = 26;
        SysID = SOFTRF_RX_SYS_FLR;
      }
      return SysID;
    }

    return SysID;
  }

  uint8_t ErrCount() const
  {
    uint8_t count = 0;
    for (uint8_t idx = 0; idx < Bytes; ++idx)
      count += Count1s(Err[idx]);
    return count;
  }

  uint8_t ErrCount(const uint8_t *corrected) const
  {
    uint8_t count = 0;
    for (uint8_t idx = 0; idx < Bytes; ++idx)
      count += Count1s((uint8_t) ((Data[idx] ^ corrected[idx]) & ~Err[idx]));
    return count;
  }

  uint8_t CorrectOGN(LDPC_Decoder &decoder, uint8_t iterations = 32)
  {
    uint8_t check = 0;
    decoder.Input(Data, Err);
    for (; iterations; --iterations) {
      check = decoder.ProcessChecks();
      if (check == 0)
        break;
    }
    decoder.Output(Data);
    Correct = (check == 0);
    return check;
  }

  int CorrectADSL(uint8_t max_bad_bits = 6)
  {
    int corrected = ADSL_Packet::Correct(Data, Err, max_bad_bits);
    Correct = (corrected >= 0);
    return corrected;
  }
};

#endif /* PROTOCOL_RXPKT_H */
