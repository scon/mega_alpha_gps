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
  char apn[30];                     // APN for GSM
  char apn_usr[30];                 // APN Username
  char apn_pw[30];                  // APN Password
  int  sim_pin_en;                  // 0=no_pin, 1=use pin
  int  meas_timeout;                 // influx_portnumber
  int  no_send;                      // log only
//  int freq;                         // Measuring frequency
//  int ser_en;                       // Serial enabled
//  int ser_f;                        // Serial format (JSON/CSV)
  
} configData_t;

#endif
