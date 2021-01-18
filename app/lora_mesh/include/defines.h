#ifndef DEFINES_H_
#define DEFINES_H_

#define MINUTE_IN_US 60000000u

#define OBS true
#define SOLAR false

/**
 * CONFIGURATION
 */


// Station ID
#define HYDRO_STATION_ID "h_j3"
//12 h_j1
//19 h_j3
//17 h_j5
#define HYDRO_DATA_SERVER "http://150.162.62.3/data/hydro/put.php"


// Set Turbidity Sensor
#define TURB_SENSOR_TYPE OBS  //true if OBS false if Solar


// Connectivity Settings

// Claro
//#define GPRS_OPERATOR_APN "claro.com.br"
//#define GPRS_OPERATOR_APN "g.claro.com.br"
#define GPRS_OPERATOR_APN "generica.claro.com.br"
//#define GPRS_OPERATOR_APN "genericaclaro.com.br"
//#define GPRS_OPERATOR_APN "ft.claro.com.br"
//#define GPRS_OPERATOR_APN "streaming.claro.com.br"
//#define GPRS_OPERATOR_APN "wap.claro.com.br"


#define GPRS_USE_AUTH 1
//#define GPRS_AUTH_USER ""
//#define GPRS_AUTH_PASSWD ""
#define GPRS_AUTH_USER "claro"
#define GPRS_AUTH_PASSWD "claro"


// Oi
// #define GPRS_OPERATOR_APN "gprs.oi.com.br"
// #define GPRS_USE_AUTH 0
// #define GPRS_AUTH_USER "oi"
// #define GPRS_AUTH_PASSWD "oi"


// TIM
// #define GPRS_OPERATOR_APN "tim.br"
// #define GPRS_USE_AUTH 1
// #define GPRS_AUTH_USER "tim"
// #define GPRS_AUTH_PASSWD "tim"

// Sending Settings
//#define SENDING_BATCH_SIZE 144
//#define SENDING_BATCH_SIZE_MAX 150
#define SENDING_BATCH_SIZE 1
#define SENDING_BATCH_SIZE_MAX 40

#define MINUTES 5u
#define SENDER_PERIOD  (MINUTE_IN_US * MINUTES)
#define SENSORS_PERIOD (SENDER_PERIOD - 1000)

#endif
