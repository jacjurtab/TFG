/*
 * dnp3.h
 *
 */

#ifndef T2_DNP3_H_INCLUDED
#define T2_DNP3_H_INCLUDED

// Global includes
//#include <stdio.h>
//#include <string.h>

// Local includes
#include "t2Plugin.h"


/* ========================================================================== */
/* ------------------------ USER CONFIGURATION FLAGS ------------------------ */
/* ========================================================================== */

#define DNP3_DEBUG      0 // Print debug messages
#define DNP3_NUM_FUNC   0 // Number of function codes to store (0 to hide dnp3FC)
#define DNP3_FE_FRMT    0 // Function/Exception codes representation: 0: hex, 1: int





#define DNP3_LOAD  0 // Load DNP3_FNAME
#define DNP3_VAR1  0 // Output dnp3Var1 (var1)
#define DNP3_IP    0 // Output dnp3IP (var2)
#define DNP3_VEC   0 // Output dnp3Var5_Var6 and dnp3Vector
#define CARGA_UTIL 247

/* ========================================================================== */
/* ------------------------- DO NOT EDIT BELOW HERE ------------------------- */
/* ========================================================================== */


// plugin defines

#define FC 0x00

#if DNP3_FE_FRMT == 1
#define DNP3_FE_TYP bt_uint_8
#define DNP3_PRI_FE "%" PRIu8
#else // DNP3_FE_FRMT == 0
#define DNP3_FE_TYP bt_hex_8
#define DNP3_PRI_FE "0x%02" B2T_PRIX8
#endif // DNP3_FE_FRMT == 0

/* --NO CREADOS POR MI --*/
#define DNP3_NUM     5
#define DNP3_WURST  10
#define DNP3_TXTLEN 16
/* --NO CREADOS POR MI --*/

// Status
#define DNP3_STAT_DNP3    0x0001 // flow is Dnp3
#define DNP3_STAT_PROTO     0x0002 // non-dnp3 protocol identifier
#define DNP3_STAT_FUNC      0x0004 // unknown function code
// #define MB_STAT_EX        0x0008 // unknown exception code
// #define MB_STAT_UID       0x0010 // multiple unit identifiers
#define DNP3_STAT_NFUNC     0x0100 // list of function codes truncated... increase DNP3_NUM_FUNC
// #define MB_STAT_NFEX      0x0200 // list of function codes which caused exceptions truncated... increase MB_NUM_FEX
// #define MB_STAT_NEXCP     0x0400 // list of exception codes truncated... increase MB_NUM_EX
#define DNP3_STAT_SNAP      0x4000 // snapped packet
#define DNP3_STAT_MALFORMED 0x8000 // malformed packet

#define DNP3_PROTO 0x0000
#define DNP3_PORT 20000


// dnp3Stat status variable
#define DNP3_STAT_DNP3 0x0001 // use this in onFlowGenerated() to flag flows of interest

#if DNP3_DEBUG == 1
#define DNP3_DBG(format, args...) T2_PINF(plugin_name, format, ##args)
#else // DNP3_DEBUG == 0
#define DNP3_DBG(format, args...) /* do nothing */
#endif // DNP3_DEBUG == 0

uint8_t fc;
// Plugin structure
typedef struct {
  char inicio[2]; // bytes de inicio
  char len[1]; // length
  uint8_t   ctrl; // control
  uint16_t destino;
  uint16_t fuente;
  uint16_t crc; //crc de la cabecera de enlace
  uint8_t transporte;
  uint8_t ctrl_code; //codigo control cabecera aplicacion
  uint8_t function_code;
  char carga_util[CARGA_UTIL];
  	
} dnp3_hdr_t;

typedef struct { // always large variables first to limit memory fragmentation
  uint32_t nmp;
  uint16_t  stat;             // status
} dnp3_flow_t;


// plugin struct pointer for potential dependencies
extern dnp3_flow_t *dnp3_flows;

#endif // T2_DNP3_H_INCLUDED
