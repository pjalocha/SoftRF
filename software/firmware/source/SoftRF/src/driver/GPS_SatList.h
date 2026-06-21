#ifndef GPS_SATLIST_H
#define GPS_SATLIST_H

#include <algorithm>
#include <stdint.h>
#include <stdio.h>

#include <format.h>
#include <nmea.h>

class GPS_Sat
{
public:
  static const uint8_t Sys_GQ = 0; // QZSS
  static const uint8_t Sys_GP = 1; // GPS
  static const uint8_t Sys_GL = 2; // GLONASS
  static const uint8_t Sys_GA = 3; // Galileo
  static const uint8_t Sys_GB = 4; // BeiDou

  union
  {
    uint32_t Word;
    struct
    {
      uint16_t Azim : 6; // [6 deg]
      uint8_t  Elev : 4; // [6 deg]
      uint8_t  SNR  : 6; // [dB/Hz]
      uint8_t  Time : 4; // [sec], 15 means invalid
      bool     Fix  : 1;
      uint16_t PRN  : 8;
      uint16_t Sys  : 3;
    } __attribute__((packed));
  };

  void Clear(void) { Word = 0; }

  static const char *SysName(uint8_t Sys)
  {
    const char *SysTable[8] = { "QZ", "GP", "GL", "GA", "BD", "--", "--", "--" };
    return SysTable[Sys & 7];
  }
};

class GPS_SatList
{
public:
  static const uint8_t MaxSize = 60;
  GPS_Sat Sat[MaxSize];
  uint8_t Size;
  uint8_t qSec;
  uint8_t BurstGSV;
  uint8_t BurstGSA;

  uint8_t FixMode;
  uint8_t PDOP;
  uint8_t HDOP;
  uint8_t VDOP;

  uint8_t VisSNR [8]; // [0.25 dB]
  uint8_t VisSats[8];
  uint8_t FixSNR [8]; // [0.25 dB]
  uint8_t FixSats[8];

  GPS_SatList() { Clear(); }

  void Clear(void)
  {
    Size = 0;
    qSec = 15;
    BurstGSV = 0;
    BurstGSA = 0;
    FixMode = 0;
    PDOP = 0;
    HDOP = 0;
    VDOP = 0;
    ClearStats();
  }

  void ClearStats(void)
  {
    for (uint8_t Sys = 0; Sys < 8; Sys++) {
      VisSNR[Sys] = 0;
      VisSats[Sys] = 0;
      FixSNR[Sys] = 0;
      FixSats[Sys] = 0;
    }
  }

  uint8_t CalcStats(void)
  {
    uint8_t AverSNR;
    return CalcStats(AverSNR);
  }

  uint8_t CalcStats(uint8_t &AverSNR)
  {
    uint32_t VisSum[8];
    uint32_t FixSum[8];

    for (uint8_t Sys = 0; Sys < 8; Sys++) {
      VisSum[Sys] = 0;
      VisSats[Sys] = 0;
      FixSum[Sys] = 0;
      FixSats[Sys] = 0;
    }

    uint8_t TotSat = 0;
    uint32_t TotSum = 0;
    for (uint8_t Idx = 0; Idx < Size; Idx++) {
      if (Sat[Idx].SNR == 0)
        continue;
      uint8_t Sys = Sat[Idx].Sys;
      TotSum += Sat[Idx].SNR;
      VisSum[Sys] += Sat[Idx].SNR;
      VisSats[Sys]++;
      if (Sat[Idx].Fix) {
        FixSum[Sys] += Sat[Idx].SNR;
        FixSats[Sys]++;
      }
      TotSat++;
    }

    for (uint8_t Sys = 0; Sys < 8; Sys++) {
      VisSNR[Sys] = VisSats[Sys] ? (VisSum[Sys] * 4 / VisSats[Sys]) : 0;
      FixSNR[Sys] = FixSats[Sys] ? (FixSum[Sys] * 4 / FixSats[Sys]) : 0;
    }

    AverSNR = TotSat ? (TotSum * 4 / TotSat) : 0;
    return TotSat;
  }

  int PrintStats(char *Line) const
  {
    int Len = sprintf(Line, "SatSNR:");
    for (uint8_t Sys = 0; Sys < 8; Sys++) {
      if (VisSats[Sys] == 0)
        continue;
      Len += sprintf(Line + Len, " %s:%d/%4.1f:%d/%4.1fdB",
        GPS_Sat::SysName(Sys), FixSats[Sys], 0.25 * FixSNR[Sys],
        VisSats[Sys], 0.25 * VisSNR[Sys]);
    }
    Len += sprintf(Line + Len, " %d:%3.1f/%3.1f/%3.1f",
      FixMode, 0.1 * PDOP, 0.1 * HDOP, 0.1 * VDOP);
    return Len;
  }

  int Process(NMEA_RxMsg &NMEA)
  {
    if (isGSV(NMEA)) return ProcessGSV(NMEA);
    if (isGxGSA(NMEA)) return ProcessGSA(NMEA);
    if (isGxGGA(NMEA)) return ProcessGGA(NMEA);
    if (isGxRMC(NMEA)) return ProcessRMC(NMEA);
    return 0;
  }

private:
  static bool isGxType(const NMEA_RxMsg &NMEA, char A, char B, char C)
  {
    return NMEA.Data[1] == 'G' && NMEA.Data[3] == A &&
           NMEA.Data[4] == B && NMEA.Data[5] == C;
  }

  static bool isGxRMC(const NMEA_RxMsg &NMEA) { return isGxType(NMEA, 'R', 'M', 'C'); }
  static bool isGxGGA(const NMEA_RxMsg &NMEA) { return isGxType(NMEA, 'G', 'G', 'A'); }
  static bool isGxGSA(const NMEA_RxMsg &NMEA) { return isGxType(NMEA, 'G', 'S', 'A'); }
  static bool isGSV(const NMEA_RxMsg &NMEA)
  {
    return isGxType(NMEA, 'G', 'S', 'V') ||
           (NMEA.Data[1] == 'B' && NMEA.Data[2] == 'D' &&
            NMEA.Data[3] == 'G' && NMEA.Data[4] == 'S' && NMEA.Data[5] == 'V');
  }

  int ProcessRMC(NMEA_RxMsg &RMC)
  {
    BurstGSV = 0;
    BurstGSA = 0;
    return ProcessTime(RMC);
  }

  int ProcessGGA(NMEA_RxMsg &GGA)
  {
    BurstGSV = 0;
    BurstGSA = 0;
    return ProcessTime(GGA);
  }

  int ProcessTime(NMEA_RxMsg &NMEA)
  {
    const char *Time = (const char *) NMEA.ParmPtr(0);
    bool NewTime = false;

    if (Time && Time[6] == '.') {
      int8_t Sec = Read_Dec2(Time + 4);
      if (Sec >= 0) {
        Sec %= 15;
        NewTime = Sec != qSec;
        qSec = Sec;
      }
    }
    if (NewTime)
      Clean(qSec);
    return 0;
  }

  int ProcessGSA(NMEA_RxMsg &GSA)
  {
    BurstGSV = 0;
    if (BurstGSA == 0)
      CleanFix();
    BurstGSA++;

    int8_t Sys = -1;
    if (GSA.Parms < 17)
      return 0;
    if (GSA.Parms >= 18)
      Sys = Read_Dec1(GSA.ParmPtr(17)[0]);

    for (int Par = 2; Par < 14; Par++) {
      int8_t PRN = Read_Dec2((const char *) GSA.ParmPtr(Par));
      if (PRN < 0)
        break;
      uint8_t Idx = Sys >= 0 ? Find(Sys, PRN) : Find(PRN);
      if (Idx > Size)
        continue;
      if (Sys >= 0 && Idx == Size && Size < MaxSize) {
        Size++;
        Sat[Idx].Word = 0;
        Sat[Idx].Sys = Sys;
        Sat[Idx].PRN = PRN;
        Sat[Idx].Azim = 63;
      }
      if (Idx < Size) {
        Sat[Idx].Fix = 1;
        Sat[Idx].Time = qSec;
      }
    }

    PDOP = ReadDOP((const char *) GSA.ParmPtr(14));
    HDOP = ReadDOP((const char *) GSA.ParmPtr(15));
    VDOP = ReadDOP((const char *) GSA.ParmPtr(16));
    int8_t Mode = Read_Dec1(GSA.ParmPtr(1)[0]);
    FixMode = Mode > 0 ? Mode : 0;
    return 0;
  }

  int ProcessGSV(NMEA_RxMsg &GSV)
  {
    BurstGSA = 0;
    BurstGSV++;

    uint8_t SatSys = 0;
    if      (GSV.Data[1] == 'G' && GSV.Data[2] == 'P') SatSys = GPS_Sat::Sys_GP;
    else if (GSV.Data[1] == 'G' && GSV.Data[2] == 'L') SatSys = GPS_Sat::Sys_GL;
    else if (GSV.Data[1] == 'G' && GSV.Data[2] == 'A') SatSys = GPS_Sat::Sys_GA;
    else if ((GSV.Data[1] == 'B' && GSV.Data[2] == 'D') ||
             (GSV.Data[1] == 'G' && GSV.Data[2] == 'B')) SatSys = GPS_Sat::Sys_GB;
    else if (GSV.Data[1] == 'G' && GSV.Data[2] == 'Q') SatSys = GPS_Sat::Sys_GQ;
    else return -1;

    if (GSV.Parms < 3)
      return -1;
    int8_t Pkts = Read_Dec1((const char *) GSV.ParmPtr(0));
    int8_t Pkt  = Read_Dec1((const char *) GSV.ParmPtr(1));
    int8_t Sats = Read_Dec2((const char *) GSV.ParmPtr(2));
    if (Sats < 0)
      Sats = Read_Dec1((const char *) GSV.ParmPtr(2));
    if (Pkts < 0 || Pkt < 0 || Sats < 0)
      return -1;

    uint8_t Count = 0;
    for (int Parm = 3; Parm <= GSV.Parms - 4; ) {
      int8_t  PRN  = Read_Dec2((const char *) GSV.ParmPtr(Parm++));
      int8_t  Elev = Read_Dec2((const char *) GSV.ParmPtr(Parm++));
      int16_t Azim = Read_Dec3((const char *) GSV.ParmPtr(Parm++));
      int8_t  SNR  = Read_Dec2((const char *) GSV.ParmPtr(Parm++));
      if (PRN < 0)
        break;
      if (Elev < 0 || Azim < 0) {
        Elev = 0;
        Azim = 378;
      }
      if (SNR < 0) SNR = 0;
      else if (SNR > 63) SNR = 63;
      Add(SatSys, PRN, Elev, Azim, SNR, qSec);
      Count++;
    }
    return Count;
  }

  static uint8_t ReadDOP(const char *Inp)
  {
    int16_t DOP = 0;
    if (Read_Float1(DOP, Inp) <= 0)
      return 0;
    if (DOP < 0)
      return 0;
    if (DOP > 255)
      return 255;
    return DOP;
  }

  uint8_t Find(uint8_t Sys, uint8_t PRN) const
  {
    uint8_t Idx = 0;
    for (; Idx < Size; Idx++) {
      if (Sat[Idx].PRN == PRN && Sat[Idx].Sys == Sys)
        break;
    }
    return Idx;
  }

  uint8_t Find(uint8_t PRN) const
  {
    uint8_t Idx = 0;
    for (; Idx < Size; Idx++) {
      if (Sat[Idx].PRN == PRN)
        break;
    }
    return Idx;
  }

  void Delete(uint8_t Idx)
  {
    if (Idx >= Size)
      return;
    if (Idx == Size - 1) {
      Size--;
      return;
    }
    Size--;
    Sat[Idx] = Sat[Size];
  }

  void CleanFix(uint8_t Time = 15)
  {
    for (uint8_t Idx = 0; Idx < Size; Idx++) {
      if (Sat[Idx].Time != Time)
        Sat[Idx].Fix = 0;
    }
  }

  void Clean(uint8_t Time)
  {
    for (uint8_t Idx = 0; Idx < Size; ) {
      if (Sat[Idx].Time == Time)
        Delete(Idx);
      else
        Idx++;
    }
  }

  uint8_t Add(uint8_t Sys, uint8_t PRN, uint16_t Elev, uint16_t Azim,
              uint8_t SNR, uint8_t Time)
  {
    uint8_t Idx = Find(Sys, PRN);
    if (Idx >= MaxSize)
      return Idx;
    GPS_Sat &New = Sat[Idx];
    if (Idx == Size)
      New.Clear();
    New.Sys = Sys;
    New.PRN = PRN;
    New.Elev = (Elev + 3) / 6;
    New.Azim = (Azim + 3) / 6;
    if (New.Time == Time) {
      if (SNR > 0)
        New.SNR = SNR;
    } else {
      New.SNR = SNR;
    }
    New.Time = Time;
    if (Idx == Size)
      Size++;
    return Idx;
  }
};

#endif
