#ifndef myTypes_h
#define myTypes_h

#include <WString.h>

// Types 'byte' und 'word' doesn't work!
typedef struct {
  int valid;                        // 0=no configuration, 1=valid configuration
  char wifi_name[31];               // SSID of WiFi
  char wifi_pw[31];                 // Password of WiFi
  int  influx_en;                   // 0=no influx, 1=use influx
  int  influx_port;                 // influx_portnumber
  char influx_ip[31];               // Ip address or hostname of InfluxDB
  char stn_id[25];                  // Name of station
  int freq;                         // Measuring frequency
  int ser_en;                       // Serial enabled
  int ser_f;                        // Serial format (JSON/CSV)
  
} configData_t;

#endif
