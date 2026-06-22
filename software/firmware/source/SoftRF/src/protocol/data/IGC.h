#ifndef IGC_H
#define IGC_H

#if defined(IGCFILESYS)

void makeLogNameDate(char *buf);  // needs buf[4]
void FlightLog_setup();
void openFlightLog();
size_t PSRAMavailable();
void clearPSRAMlog();
void suspendFlightLog();
bool decompressfile(char *filename);
void resumeFlightLog();
void logFlightPosition();
void completeFlightLog();
void closeFlightLog();
const char *FlightLogStatus();
void FlightLogComment(const char *data, bool force=false);
void MD5_test();

#if defined(ARDUINO_ARCH_NRF52)
void FlightLog_decomp();
#endif

extern bool FlightLogOpen;
extern char FlightLogPath[];

extern char *PSRAMbuf;
extern size_t PSRAMbufUsed;

#else

static const bool FlightLogOpen = false;

static inline void makeLogNameDate(char *) {}
static inline void FlightLog_setup() {}
static inline void openFlightLog() {}
static inline size_t PSRAMavailable() { return 0; }
static inline void clearPSRAMlog() {}
static inline void suspendFlightLog() {}
static inline bool decompressfile(char *) { return false; }
static inline void resumeFlightLog() {}
static inline void logFlightPosition() {}
static inline void completeFlightLog() {}
static inline void closeFlightLog() {}
static inline const char *FlightLogStatus() { return "N/A"; }
static inline void FlightLogComment(const char *, bool = false) {}
static inline void MD5_test() {}

#endif

#endif
