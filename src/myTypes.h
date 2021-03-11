#ifndef myTypes_h
#define myTypes_h

#include <WString.h>

// Types 'byte' und 'word' doesn't work!
typedef struct {
  char server_ip[20];               // IP of receiving server
  char server_port[10];             // PORT of receiving server
  char sim_pin[8];                  // PIN of SIM-card
  char stn_id[20];                  // ID of the STATION
  char sns_id[20];                  // ID of the Sensormodule
//  int  influx_en;                   // 0=no influx, 1=use influx
//  int  influx_port;                 // influx_portnumber
//  int freq;                         // Measuring frequency
//  int ser_en;                       // Serial enabled
//  int ser_f;                        // Serial format (JSON/CSV)
  
} configData_t;

#endif
