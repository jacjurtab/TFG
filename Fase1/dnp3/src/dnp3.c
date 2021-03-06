/*
 * dnp3.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Author: Jacinto Jurado Tabares
 */

#include "dnp3.h"


dnp3_flow_t *dnp3_flows;

/*
 * Static variables are only visible in this file
 */
static uint64_t numDNP3Pkts;
FILE *fich;
bool cantidad = false;
int num_pqts = 0;

/*
 * Function prototypes
 */

char* getFCString (uint8_t fc);
char* getObjectString (uint8_t grupo, uint8_t variacion);
int getClasValue (uint8_t clasificacion);
int getRangeValue (uint8_t clasificacion);
int getObjectSize (uint8_t grupo, uint8_t variacion);
void printStaticObject (uint8_t grupo, uint8_t variacion, unsigned char valor[], FILE * fich);
static inline void dnp3_pluginReport(FILE *stream);



// Tranalyzer functions

/*
 * This describes the plugin name, version, major and minor version of
 * Tranalyzer required and dependencies
 */
T2_PLUGIN_INIT("dnp3", "0.8.9", 0, 8);
//T2_PLUGIN_INIT_WITH_DEPS("dnp3", "0.8.9", 0, 8, "tcpFlags,tcpStates");


/*
 * This function is called before processing any packet.
 */
void initialize() {
    T2_PLUGIN_STRUCT_NEW(dnp3_flows);

    if (sPktFile) {
      //fputs("inicio\tlongitud\tcontrol\tdir_destino\tdir_origen\tcrc\ttransporte\tcodigoControl\tFunctionCode\tchunk\t", sPktFile);
      fputs("inicio\tlongitud\tcontrol\tdir_destino\tdir_origen\tcrc\ttransporte\tcodigoControl\tFunctionCode\tCadenaTexto\t", sPktFile);
      fich = fopen("extracciones_prueba.csv", "a");
      fprintf(fich, "NumeroPaquete,Origen,Destino,Longitud,CodigoFuncion,CampoVector,NumeroObjetos\n");
      fclose(fich);
    }
}


/*
 * This function is used to describe the columns output by the plugin
 */
binary_value_t* printHeader() {
    binary_value_t *bv = NULL;

        bv = bv_append_bv(bv, bv_new_bv("DNP3 number of packets", "dnp3NPkts", 0, 1, bt_uint_32));

    return bv;
}


/*
 * This function is called every time a new flow is created.
 */
void onFlowGenerated(packet_t *packet UNUSED, unsigned long flowIndex) {
    // Reset the structure for this flow
    dnp3_flow_t * const dnp3_flow = &dnp3_flows[flowIndex];
    memset(dnp3_flow, '\0', sizeof(*dnp3_flow));

   
    const flow_t * const flowP = &flows[flowIndex];
    if (flowP->status & L2_FLOW) return;

    if (flowP->layer4Protocol == L3_TCP && (flowP->srcPort == DNP3_PORT || flowP->dstPort == DNP3_PORT))
        dnp3_flow->stat |= DNP3_STAT_DNP3;
}


#if ETH_ACTIVATE > 0
/*
 * This function is called for every packet with a layer 2.
 * If flowIndex is HASHTABLE_ENTRY_NOT_FOUND, this means the packet also
 * has a layer 4 and thus a call to claimLayer4Information() will follow.
 */
void claimLayer2Information(packet_t *packet, unsigned long flowIndex) {

    if (flowIndex == HASHTABLE_ENTRY_NOT_FOUND) return;

}
#endif // ETH_ACTIVATE > 0


/*
 * This function is called for every packet with a layer 4.
 */
void claimLayer4Information(packet_t *packet, unsigned long flowIndex) {
    dnp3_flow_t * const dnp3FlowP = &dnp3_flows[flowIndex];

    
    if (!dnp3FlowP->stat) return; // not a dnp3 packet


    const dnp3_hdr_t *dnp = (dnp3_hdr_t*)packet->layer7Header;
    if (sPktFile) {

      if ((fich = fopen("extracciones_prueba.csv", "a")) == NULL){
	printf ( " Error en la apertura\n ");
      }
      else {
	uint8_t inicio1 = dnp->inicio[0];
	uint8_t inicio2 = dnp->inicio[1];
	uint16_t inicio = (inicio1 << 8) | inicio2;
      
	if (inicio == 0x0564){
	  num_pqts++;
	  
	  char* codigo_funcion = getFCString(dnp->function_code);

	  char carga_sin_crc[sizeof(dnp->carga_util)];

	  int f = 0;
	  int g = 0;
	  //Bucle que elimina el crc a??adido por dnp3 cada 16 bytes de carga util
	  while (f < dnp->len[0] - 8){  //tama??o de la carga util
	    char bloque_datos[17];
	    if (f == 0){
	      f = 13;
	      g = 13;
	      memcpy(carga_sin_crc, &(dnp->carga_util[0]), 13);
	    }
	    else{
	      memcpy(bloque_datos, &(dnp->carga_util[f+2]), 16);
	      memcpy(&(carga_sin_crc[g]), bloque_datos, 16);
	      f = f + 18;
	      g = g + 16;
	    }
	  }

	  //En este caso no hay objetos en el mensaje, solo cabecera, function code y alomejor internals indication
	  if (dnp->len[0] <= 10){ 

	    //Es un mensaje de respuesta asi que hay internal indications
	    if (dnp->function_code > 0x80){ 
	      fprintf(fich, "%d,", num_pqts);
	      fprintf(fich, "%d,", dnp->fuente);
	      fprintf(fich, "%d,", dnp->destino);
	      fprintf(fich, "%d,", dnp->len[0]);
	      fprintf(fich, "%s,", codigo_funcion);
	      fprintf(fich, "ND,");
	      fprintf(fich, "0");
	      fprintf(fich, "\n");
	    }

	    //Es un mensaje de solicitud, no hay internal indications
	    else{ 
	      fprintf(fich, "%d,", num_pqts);
	      fprintf(fich, "%d,", dnp->fuente);
	      fprintf(fich, "%d,", dnp->destino);
	      fprintf(fich, "%d,", dnp->len[0]);
	      fprintf(fich, "%s,", codigo_funcion);
	      fprintf(fich, "ND,");
	      fprintf(fich, "0");
	      fprintf(fich, "\n");
	    }
	  }
	  else{

	    //Mensaje de solicitud con objetos
	    if (dnp->function_code < 0x80){ 
	      fprintf(fich, "%d,", num_pqts);
	      fprintf(fich, "%d,", dnp->fuente);
	      fprintf(fich, "%d,", dnp->destino);
	      fprintf(fich, "%d,", dnp->len[0]);
	      fprintf(fich, "%s,", codigo_funcion);

	      int bytes_leidos = 0;
	      int num_objetos_leidos = 0;
	      while(bytes_leidos < dnp->len[0] - 8){
		char obj[2];
		memcpy(obj,&(carga_sin_crc[bytes_leidos]),2);
		char* object = getObjectString(obj[0],obj[1]);
		int tamanio_objeto = getObjectSize(obj[0],obj[1]);
		num_objetos_leidos++;

		fprintf(fich, "%s;", object);
	      
		char clasificacion[1];
		memcpy(clasificacion,&(carga_sin_crc[2+bytes_leidos]),1);
		int prefix = getClasValue(clasificacion[0]);
		int rango= getRangeValue(clasificacion[0]);

		 //Si existe el campo rango
		if (rango != 0){
		  char campo_rango[rango];
		  memcpy(campo_rango,&(carga_sin_crc[3+bytes_leidos]), rango);

		  //Si lo que se expresa en el campo rango corresponde a la cantidad de puntos del objeto
		  if (cantidad == true){ 
		    unsigned char valor[tamanio_objeto];
		    memcpy(valor,&(carga_sin_crc[3+rango+prefix+bytes_leidos]),tamanio_objeto);
		    printStaticObject(obj[0], obj[1], valor, fich);
		    bytes_leidos = bytes_leidos + 3 + rango + prefix + tamanio_objeto;
		  }
		  //Si el campo rango no indica que hay mas de un punto en ese objeto
		  else { 
		    unsigned char valor[tamanio_objeto];
		    memcpy(valor, &(carga_sin_crc[3+rango+prefix+bytes_leidos]), tamanio_objeto);
		    printStaticObject(obj[0], obj[1], valor, fich);
		    bytes_leidos = bytes_leidos + 3+ rango + prefix + tamanio_objeto;
		  }
		
		}
		//Si no hay campo rango
		else { 
		  unsigned char valor[tamanio_objeto];
		  memcpy(valor, &(carga_sin_crc[2+prefix+bytes_leidos]), tamanio_objeto);
		  printStaticObject(obj[0], obj[1], valor, fich);
		  bytes_leidos = bytes_leidos + 3 + prefix + tamanio_objeto;
		}
	      }
	      fprintf(fich, ",%d", num_objetos_leidos);
	      fprintf(fich, "\n");
	      }

	    //Mensaje de respuesta con objetos
	    else { 
	      fprintf(fich, "%d,", num_pqts);
	      fprintf(fich, "%d,", dnp->fuente);
	      fprintf(fich, "%d,", dnp->destino);
	      fprintf(fich, "%d,", dnp->len[0]);
	      fprintf(fich, "%s,", codigo_funcion);

	      int bytes_leidos = 2;
	      int num_objetos_leidos = 0;
	      while (bytes_leidos < dnp->len[0] - 8){
		char obj[2];
		memcpy(obj,&(carga_sin_crc[bytes_leidos]),2);
		char* object = getObjectString(obj[0],obj[1]);
		int tamanio_objeto = getObjectSize(obj[0],obj[1]);
		num_objetos_leidos++;

		fprintf(fich, "%s;", object);

		// Con el objeto Single-bit binary input hay un problema de tratamiento de los bytes
		if (((obj[0] << 8) | obj[1]) == 0x0101){
		  char clasificacion[1];
		  memcpy(clasificacion,&(carga_sin_crc[2+bytes_leidos]),1);
		  int prefix = getClasValue(clasificacion[0]);
		  int rango= getRangeValue(clasificacion[0]);
		  fprintf(fich, "No implementado;");
		  bytes_leidos = bytes_leidos + 1 + prefix + rango;
		}
		else {
		  char clasificacion[1];
		  memcpy(clasificacion,&(carga_sin_crc[2+bytes_leidos]),1);
		  int prefix = getClasValue(clasificacion[0]);
		  int rango= getRangeValue(clasificacion[0]);
	      
		  if (rango != 0){
		    char campo_rango[rango];
		    memcpy(campo_rango,&(carga_sin_crc[3+bytes_leidos]), rango);
		    if (cantidad == true){
		      int num_objetos = campo_rango[0];
		      int i = 1;
		      int puntos_leidos = 0;
		      fprintf(fich, "numeroPuntos=%d;", num_objetos);
		      while (i <= num_objetos){
			unsigned char valor[tamanio_objeto];
			memcpy(valor,&(carga_sin_crc[3+rango+prefix*i+bytes_leidos+puntos_leidos*tamanio_objeto]),tamanio_objeto);
			printStaticObject(obj[0], obj[1], valor, fich);
			i++;
			puntos_leidos++;
		      }
		      bytes_leidos = bytes_leidos +  (3 + rango + prefix*num_objetos + tamanio_objeto*num_objetos);
		    }
		    else {
		      unsigned char valor[tamanio_objeto];
		      memcpy(valor, &(carga_sin_crc[3+rango+prefix+bytes_leidos]), tamanio_objeto);
		      printStaticObject(obj[0], obj[1], valor, fich);
		      bytes_leidos = bytes_leidos + 3 + rango + prefix + tamanio_objeto;
		    }
		
		  }
		  else {
		    unsigned char valor[tamanio_objeto];
		    memcpy(valor, &(carga_sin_crc[2+prefix+bytes_leidos]), tamanio_objeto);
		    printStaticObject(obj[0], obj[1], valor, fich);
		    bytes_leidos = bytes_leidos + 3 + prefix + tamanio_objeto;
		  }
		}
	      }
	      fprintf(fich, ",%d", num_objetos_leidos);
	      fprintf(fich, "\n");
	    }
	  }
	}
	fclose(fich);
      }
    }
    

    // only 1. frag packet will be processed
    if (!t2_is_first_fragment(packet)) return;

    numDNP3Pkts++;
	dnp3FlowP->nmp++;

}


/*
 * This function is called once a flow is terminated.
 * Output all the statistics for the flow here.
 */
void onFlowTerminate(unsigned long flowIndex) {

    const dnp3_flow_t * const dnp3FlowP = &dnp3_flows[flowIndex];
    
    outputBuffer_append(main_output_buffer, (char*) &dnp3FlowP->nmp, sizeof(uint32_t));


}


/*
 * This function is used to report information regarding the plugin.
 * This will appear in the final report.
 */
void pluginReport(FILE *stream) {
    T2_FPLOG_NUMP(stream, plugin_name, "Number of Modbus packets", numDNP3Pkts, numPackets);
}


/*
 * This function is called once all the packets have been processed.
 * Cleanup all used memory here
 */
void onApplicationTerminate() {
    free(dnp3_flows);
}

char* getFCString (uint8_t fc){
  char* cadena  = "";
  switch (fc) {
  case 0x00:
    cadena = "Confirm";
    break;
  case 0x01:
    cadena = "Read";
    break;
  case 0x02:
    cadena = "Write";
    break;
  case 0x03:
    cadena = "Select";
    break;
  case 0x04:
    cadena = "Operate";
    break;
  case 0x05:
    cadena = "Direct Operate";
    break;
  case 0x06:
    cadena = "Direct Operate no ACK";
    break;
  case 0x07:
    cadena = "Immediate Freeze";
    break;
  case 0x08:
    cadena = "Immediate Freeze no ACK";
    break;
  case 0x09:
    cadena = "Freeze and Clear";
    break;
  case 0x0a:
    cadena = "Freeze and Clear no ACK";
    break;
  case 0x0b:
    cadena = "Freeze With Time";
    break;
  case 0x0c:
    cadena = "Freeze With Time no ACK";
    break;
  case 0x0d:
    cadena = "Cold Restart";
    break;
  case 0x0e:
    cadena = "Warm Restart";
    break;
  case 0x0f:
    cadena = "Initialize Data";
    break;
  case 0x10:
    cadena = "Initialize Application";
    break;
  case 0x11:
    cadena = "Start Application";
    break;
  case 0x12:
    cadena = "Stop Application";
    break;
  case 0x13:
    cadena = "Save configuration";
    break;
  case 0x14:
    cadena = "Enable Spontaneus Messages";
    break;
  case 0x15:
    cadena = "Disable Spontaneus Messages";
    break;
  case 0x16:
    cadena = "Assign Classes";
    break;
  case 0x17:
    cadena = "Delay Measurements";
    break;
  case 0x18:
    cadena = "Record Current Time";
    break;
  case 0x19:
    cadena = "Open File";
    break;
  case 0x1a:
    cadena = "Close File";
    break;
  case 0x1b:
    cadena = "Delete File";
    break;
  case 0x1c:
    cadena = "Get File Info";
    break;
  case 0x1d:
    cadena = "Authenticate File";
    break;
  case 0x1e:
    cadena = "Abort File";
    break;
  case 0x1f:
    cadena = "Activate Config";
    break;
  case 0x20:
    cadena = "Authentication Request";
    break;
  case 0x21:
    cadena = "Authentication Error";
    break;
  case 0x81:
    cadena = "Response";
    break;
  case 0x82:
    cadena = "Unsolicited Response";
    break;
  case 0x83:
    cadena = "Authentication Response";
    break;
  }
  
  return cadena;
}


char* getObjectString (uint8_t grupo, uint8_t variacion) {
  char* cadena = "";
  uint16_t object = (grupo << 8) | variacion;
  switch (object) {
    //Internal Indications
  case 0x0000:
    cadena = "Internal Indications of Response";
    break;
  case 0x1000:
    cadena = "Internal Indications of Response";
    break;
    // Device Attributes
  case 0x00f2:
    cadena = "Device Attributes - Device Manufacturers SW Version (Obj:00  Var:242)";
    break;
  case 0x00f3:
    cadena = "Device Attributes - Device Manufacturers HW Version (Obj:00  Var:243)";
    break;
  case 0x00f6:
    cadena = "Device Attributes - User-Assigned ID code/number (Obj:00  Var:246)";
    break;
  case 0x00f8:
    cadena = "Device Attributes - Device Serial Number (Obj:00  Var:248)";
    break;
  case 0x00fa:
    cadena = "Device Attributes - Device Product Name and Model (Obj:00  Var:250)";
    break;
  case 0x00fc:
    cadena = "Device Attributes - Device Manufacturers Name (Obj:00  Var:252)";
    break;
  case 0x00fe:
    cadena = "Device Attributes - Non-specific All-attributes Request (Obj:00  Var:254)";
    break;
  case 0x00ff:
    cadena = "Device Attributes - List of Attribute Variations (Obj:00  Var:255)";
    break;
  //Binary Input Objects
  case 0x0100:
    cadena = "Binary Input Default Variation (Obj:01  Var:Default)";
    break;
  case 0x0101:
    cadena = "Single-Bit Binary Input (Obj:01  Var:01)";
    break;
  case 0x0102:
    cadena = "Binary Input With Status (Obj:01  Var:02)";
    break;
  case 0x0200:
    cadena = "Binary Input Change Default Variation (Obj:02  Var:Default)";
    break;
  case 0x0201:
    cadena = "Binary Input Change Without Time (Obj:02  Var:01)";
    break;
  case 0x0202:
    cadena = "Binary Input Change With Time (Obj:02  Var:02)";
    break;
  case 0x0203:
    cadena = "Binary Input Change With Relative Time (Obj:02  Var:03)";
    break;
  //Double bit-input objects
  case 0x0300:
    cadena = "Double-bit Input Default Variation (Obj:03  Var:Default)";
    break;
  case 0x0301:
    cadena = "Double-bit Input No Flags (Obj:03  Var:01)";
    break;
  case 0x0302:
    cadena = "Double-bit Input With Status (Obj:03  Var:02)";
    break;
  case 0x0400:
    cadena = "Double-bit Input Change Default Variation (Obj:04  Var:Default)";
    break;
  case 0x0401:
    cadena = "Double-bit Input Change Without Time (Obj:04  Var:01)";
    break;
  case 0x0402:
    cadena = "Double-bit Input Change With Time (Obj:04  Var:02)";
    break;
  case 0x0403:
    cadena = "Double-bit Input Change With Relative Time (Obj:04  Var:03)";
    break;
  //Binary Output Objects
  case 0x0a00:
    cadena = "Binary Output Default Variation (Obj:10  Var:Default)";
    break;
  case 0x0a01:
    cadena = "Binary Output (Obj:10  Var:01)";
    break;
  case 0x0a02:
    cadena = "Binary Output Status (Obj:10  Var:02)";
    break;
  case 0x0b00:
    cadena = "Binary Output Change Default Variation (Obj:11  Var:Default)";
    break;
  case 0x0b01:
    cadena = "Binary Output Change Without Time (Obj:11  Var:01)";
    break;
  case 0x0b02:
    cadena = "Binary Output Change With Time (Obj:11  Var:02)";
    break;
  case 0x0c01:
    cadena = "Control Relay Output Block (Obj:12  Var:01)";
    break;
  //Counter Objects
  case 0x1400:
    cadena = "Binary Counter Default Variation (Obj:20  Var:Default)";
    break;
  case 0x1401:
    cadena = "32-Bit Binary Counter (Obj:20  Var:01)";
    break;
  case 0x1402:
    cadena = "16-Bit Binary Counter (Obj:20  Var:02)";
    break;
  case 0x1403:
    cadena = "32-Bit Binary Delta Counter (Obj:20  Var:03)";
    break;
  case 0x1404:
    cadena = "16-Bit Binary Delta Counter (Obj:20  Var:04)";
    break;
  case 0x1405:
    cadena = "32-Bit Binary Counter Without Flag (Obj:20  Var:05)";
    break;
  case 0x1406:
    cadena = "16-Bit Binary Counter Without Flag (Obj:20  Var:06)";
    break;
  case 0x1407:
    cadena = "32-Bit Binary Delta Counter Without Flag (Obj:20  Var:07)";
    break;
  case 0x1408:
    cadena = "16-Bit Binary Delta Counter Without Flag (Obj:20  Var:08)";
    break;
  case 0x1500:
    cadena = "Frozen Binary Counter Default Variation (Obj:21  Var:Default)";
    break;
  case 0x1501:
    cadena = "32-Bit Frozen Binary Counter (Obj:21  Var:01)";
    break;
  case 0x1502:
    cadena = "16-Bit Frozen Binary Counter (Obj:21  Var:02)";
    break;
  case 0x1503:
    cadena = "32-Bit Frozen Binary Delta Counter (Obj:21  Var:03)";
    break;
  case 0x1504:
    cadena = "16-Bit Frozen Binary Delta Counter (Obj:21  Var:04)";
    break;
  case 0x1505:
    cadena = "32-Bit Frozen Binary Counter (Obj:21  Var:01)";
    break;
  case 0x1506:
    cadena = "16-Bit Frozen Binary Counter (Obj:21  Var:02)";
    break;
  case 0x1507:
    cadena = "32-Bit Frozen Binary Delta Counter (Obj:21  Var:03)";
    break;
  case 0x1508:
    cadena = "16-Bit Frozen Binary Delta Counter (Obj:21  Var:04)";
    break;
  case 0x1509:
    cadena = "32-Bit Frozen Binary Counter Without Flag (Obj:21  Var:05)";
    break;
  case 0x150a:
    cadena = "16-Bit Frozen Binary Counter Without Flag (Obj:21  Var:06)";
    break;
  case 0x150b:
    cadena = "32-Bit Frozen Binary Delta Counter Without Flag (Obj:21  Var:07)";
    break;
  case 0x150c:
    cadena = "16-Bit Frozen Binary Delta Counter Without Flag (Obj:21  Var:08)";
    break;
  case 0x1600:
    cadena = "Binary Counter Change Default Variation (Obj:22  Var:Default)";
    break;
  case 0x1601:
    cadena = "32-Bit Counter Change Event w/o Time (Obj:22  Var:01)";
    break;
  case 0x1602:
    cadena = "16-Bit Counter Change Event w/o Time (Obj:22  Var:02)";
    break;
  case 0x1603:
    cadena = "32-Bit Delta Counter Change Event w/o Time (Obj:22  Var:03)";
    break;
  case 0x1604:
    cadena = "16-Bit Delta Counter Change Event w/o Time (Obj:22  Var:04)";
    break;
  case 0x1605:
    cadena = "32-Bit Counter Change Event with Time (Obj:22  Var:05)";
    break;
  case 0x1606:
    cadena = "16-Bit Counter Change Event with Time (Obj:22  Var:06)";
    break;
  case 0x1607:
    cadena = "32-Bit Delta Counter Change Event with Time (Obj:22  Var:07)";
    break;
  case 0x1608:
    cadena = "16-Bit Delta Counter Change Event with Time (Obj:22  Var:08)";
    break;
  case 0x1700:
    cadena = "Frozen Binary Counter Change Default Variation (Obj:23  Var:Default)";
    break;
  case 0x1701:
    cadena = "32-Bit Frozen Counter Change Event w/o Time (Obj:23  Var:01)";
    break;
  case 0x1702:
    cadena = "16-Bit Frozen Counter Change Event w/o Time (Obj:23  Var:02)";
    break;
  case 0x1703:
    cadena = "32-Bit Frozen Delta Counter Change Event w/o Time (Obj:23  Var:03)";
    break;
  case 0x1704:
    cadena = "16-Bit Frozen Delta Counter Change Event w/o Time (Obj:23  Var:04)";
    break;
  case 0x1705:
    cadena = "32-Bit Frozen Counter Change Event with Time (Obj:23  Var:05)";
    break;
  case 0x1706:
    cadena = "16-Bit Frozen Counter Change Event with Time (Obj:23  Var:06)";
    break;
  case 0x1707:
    cadena = "32-Bit Frozen Delta Counter Change Event with Time (Obj:23  Var:07)";
    break;
  case 0x1708:
    cadena = "16-Bit Frozen Delta Counter Change Event with Time (Obj:23  Var:08)";
    break;
  //Analog Input Objects
  case 0x1e00:
    cadena = "Analog Input Default Variation (Obj:30  Var:Default)";
    break;
  case 0x1e01:
    cadena = "32-Bit Analog Input (Obj:30  Var:01)";
    break;
  case 0x1e02:
    cadena = "16-Bit Analog Input (Obj:30  Var:02)";
    break;
  case 0x1e03:
    cadena = "32-Bit Analog Input Without Flag (Obj:30  Var:03)";
    break;
  case 0x1e04:
    cadena = "16-Bit Analog Input Without Flag (Obj:30  Var:04)";
    break;
  case 0x1e05:
    cadena = "32-Bit Floating Point Input (Obj:30  Var:05)";
    break;
  case 0x1e06:
    cadena = "64-Bit Floating Point Input (Obj:30  Var:06)";
    break;
  case 0x2000:
    cadena = "Analog Input Change Default Variation (Obj:32  Var:Default)";
    break;
  case 0x2001:
    cadena = "32-Bit Analog Change Event w/o Time (Obj:32  Var:01)";
    break;
  case 0x2002:
    cadena = "16-Bit Analog Change Event w/o Time (Obj:32  Var:02)";
    break;
  case 0x2003:
    cadena = "32-Bit Analog Change Event with Time (Obj:32  Var:03)";
    break;
  case 0x2004:
    cadena = "16-Bit Analog Change Event with Time (Obj:32  Var:04)";
    break;
  case 0x2005:
    cadena = "32-Bit Floating Point Change Event w/o Time (Obj:32  Var:05)";
    break;
  case 0x2006:
    cadena = "64-Bit Floating Point Change Event w/o Time (Obj:32  Var:06)";
    break;
  case 0x2007:
    cadena = "32-Bit Floating Point Change Event w/ Time (Obj:32  Var:07)";
    break;
  case 0x2008:
    cadena = "64-Bit Floating Point Change Event w/ Time (Obj:32  Var:08)";
    break;
  case 0x2105:
    cadena = "32-Bit Floating Point Frozen Change Event w/o Time (Obj:33  Var:05)";
    break;
  case 0x2106:
    cadena = "64-Bit Floating Point Frozen Change Event w/o Time (Obj:33  Var:06)";
    break;
  case 0x2107:
    cadena = "32-Bit Floating Point Frozen Change Event w/ Time (Obj:33  Var:07)";
    break;
  case 0x2108:
    cadena = "64-Bit Floating Point Frozen Change Event w/ Time (Obj:33  Var:08)";
    break;
  case 0x2200:
    cadena = "Analog Input Deadband Default Variation (Obj:34  Var:Default)";
    break;
  case 0x2201:
    cadena = "16-Bit Analog Input Deadband (Obj:34  Var:01)";
    break;
  case 0x2202:
    cadena = "32-Bit Analog Input Deadband (Obj:34  Var:02)";
    break;
  case 0x2203:
    cadena = "32-Bit Floating Point Analog Input Deadband (Obj:34  Var:03)";
    break;
  //Analog Output Objects
  case 0x2800:
    cadena = "Analog Output Default Variation (Obj:40  Var:Default)";
    break;
  case 0x2801:
    cadena = "32-Bit Analog Output Status (Obj:40  Var:01)";
    break;
  case 0x2802:
    cadena = "16-Bit Analog Output Status (Obj:40  Var:02)";
    break;
  case 0x2803:
    cadena = "32-Bit Floating Point Output Status (Obj:40  Var:03)";
    break;
  case 0x2804:
    cadena = "64-Bit Floating Point Output Status (Obj:40  Var:04)";
    break;
  case 0x2901:
    cadena = "32-Bit Analog Output Block (Obj:41  Var:01)";
    break;
  case 0x2902:
    cadena = "16-Bit Analog Output Block (Obj:41  Var:02)";
    break;
  case 0x2903:
    cadena = "32-Bit Floating Point Output Block (Obj:41  Var:03)";
    break;
  case 0x2904:
    cadena = "64-Bit Floating Point Output Block (Obj:41  Var:04)";
    break;
  case 0x2a00:
    cadena = "Analog Output Event Default Variation (Obj:42  Var:Default)";
    break;
  case 0x2a01:
    cadena = "32-Bit Analog Output Event w/o Time (Obj:42  Var:01)";
    break;
  case 0x2a02:
    cadena = "16-Bit Analog Output Event w/o Time (Obj:42  Var:02)";
    break;
  case 0x2a03:
    cadena = "32-Bit Analog Output Event with Time (Obj:42  Var:03)";
    break;
  case 0x2a04:
    cadena = "16-Bit Analog Output Event with Time (Obj:42  Var:04)";
    break;
  case 0x2a05:
    cadena = "32-Bit Floating Point Output Event w/o Time (Obj:42  Var:05)";
    break;
  case 0x2a06:
    cadena = "64-Bit Floating Point Output Event w/o Time (Obj:42  Var:06)";
    break;
  case 0x2a07:
    cadena = "32-Bit Floating Point Output Event w/ Time (Obj:42  Var:07)";
    break;
  case 0x2a08:
    cadena = "64-Bit Floating Point Output Event w/ Time (Obj:42  Var:08)";
    break;
  // Time Objects
  case 0x3200:
    cadena = "Time and Date Default Variations (Obj:50  Var:Default)";
    break;
  case 0x3201:
    cadena = "Time and Date (Obj:50  Var:01)";
    break;
  case 0x3202:
    cadena = "Time and Date w/Interval (Obj:50  Var:02)";
    break;
  case 0x3203:
    cadena = "Last Recorded Time and Date (Obj:50  Var:03)";
    break;
  case 0x3301:
    cadena = "Time and Date CTO (Obj:51  Var:01)";
    break;
  case 0x3302:
    cadena = "Unsynchronized Time and Date CTO (Obj:51  Var:02)";
    break;
  case 0x3401:
    cadena = "Time Delay - Coarse (Obj:52  Var:01)";
    break;
  case 0x3402:
    cadena = "Time Delay - Fine (Obj:52  Var:02)";
    break;
  //Class Data Objects
  case 0x3c01:
    cadena = "Class 0 Data (Obj:60  Var:01)";
    break;
  case 0x3c02:
    cadena = "Class 1 Data (Obj:60  Var:02)";
    break;
  case 0x3c03:
    cadena = "Class 2 Data (Obj:60  Var:03)";
    break;
  case 0x3c04:
    cadena = "Class 3 Data (Obj:60  Var:04)";
    break;
  //File Objects
  case 0x4603:
    cadena = "File Control - File Command (Obj:70  Var:03)";
    break;
  case 0x4604:
    cadena = "File Control - File Status (Obj:70  Var:04)";
    break;
  case 0x4605:
    cadena = "File Control - File Transport (Obj:70  Var:05)";
    break;
  case 0x4606:
    cadena = "File Control - File Transport Status (Obj:70  Var:06)";
    break;
  //Devices Objects
  case 0x5001:
    cadena = "Internal Indications (Obj:80  Var:01)";
    break;
  //Data Sets
  case 0x5501:
    cadena = "Data-Set Prototype  with UUID (Obj:85  Var:01)";
    break;
  case 0x5601:
    cadena = "Data-Set Descriptor  Data-Set Contents (Obj:86  Var:01)";
    break;
  case 0x5602:
    cadena = "Data-Set Descriptor  Characteristics (Obj:86  Var:02)";
    break;
  case 0x5603:
    cadena = "Data-Set Descriptor  Point Index Attributes (Obj:86  Var:03)";
    break;
  case 0x5701:
    cadena = "Data-Set  Present Value (Obj:87  Var:01)";
    break;
  case 0x5801:
    cadena = "Data-Set  Snapshot (Obj:88  Var:01)";
    break;
  //Octect Strings Objects
  case 0x6e00:
    cadena = "Octet String (Obj:110)";
    break;
  case 0x6f00:
    cadena = "Octet String Event (Obj:111)";
    break;
  //Virtual Terminal Objects
  case 0x7000:
    cadena = "Virtual Terminal Output Block (Obj:112)";
    break;
  case 0x7100:
    cadena = "Virtual Terminal Event Data (Obj:113)";
    break;
  }
  return cadena;
}


int getClasValue (uint8_t clasificacion){
  int tam_prefix = 0;
  if ((clasificacion & 0x70) == 0x00){
    tam_prefix = 0;
  }
  else if ((clasificacion & 0x70) == 0x10){
    tam_prefix = 1;
  }
  else if ((clasificacion & 0x70) == 0x20){
    tam_prefix = 2;
  }
  else if ((clasificacion & 0x70) == 0x30){
    tam_prefix = 4;
  }
  else if ((clasificacion & 0x70) == 0x40){
    tam_prefix = 1;
  }
  else if ((clasificacion & 0x70) == 0x50){
    tam_prefix = 2;
  }
  else if ((clasificacion & 0x70) == 0x60){
    tam_prefix = 4;
  }
  return tam_prefix;
}

int getRangeValue (uint8_t clasificacion){
  int rango = 0;
  if ((clasificacion & 0x0F) == 0x00){
    rango = 2;
    cantidad = false;
  }
  if ((clasificacion & 0x0F) == 0x01){
    rango = 4;
    cantidad = false;
  }
  if ((clasificacion & 0x0F) == 0x02){
    rango = 8;
    cantidad = false;
  }
  if ((clasificacion & 0x0F) == 0x03){
    rango = 2;
    cantidad = false;
  }
  if ((clasificacion & 0x0F) == 0x04){
    rango = 4;
    cantidad = false;
  }
  if ((clasificacion & 0x0F) == 0x05){
    rango = 8;
    cantidad = false;
  }
  if ((clasificacion & 0x0F) == 0x06){
    rango = 0;
    cantidad = false;
  }
  if ((clasificacion & 0x0F) == 0x07){
    rango = 1;
    cantidad = true;
  }
  if ((clasificacion & 0x0F) == 0x08){
    rango = 2;
    cantidad = true;
  }
  if ((clasificacion & 0x0F) == 0x09){
    rango = 4;
    cantidad = true;
  }
  if ((clasificacion & 0x0F) == 0x0B){
    rango = 1;
    cantidad = false;
  }
  return rango;
}


int getObjectSize (uint8_t grupo, uint8_t variacion){
  int size = 0;
  uint16_t object = (grupo << 8) | variacion;
  switch (object) {
    //Internal Indications
  case 0x0000:
    size = 2;
    break;
  case 0x1000:
    size = 2;
    break;
    // Device Attributes
  case 0x00f2:
    size = 0;
    break;
  case 0x00f3:
    size = 0;
    break;
  case 0x00f6:
    size = 0;
    break;
  case 0x00f8:
    size = 0;
    break;
  case 0x00fa:
    size = 0;
    break;
  case 0x00fc:
    size = 0;
    break;
  case 0x00fe:
    size = 0;
    break;
  case 0x00ff:
    size = 0;
    break;
  //Binary Input Objects
  case 0x0100:
    size = 1;
    break;
  case 0x0101:
    size = 1;
    break;
  case 0x0102:
    size = 1;
    break;
  case 0x0200:
    size = 1;
    break;
  case 0x0201:
    size = 1;
    break;
  case 0x0202:
    size = 7;
    break;
  case 0x0203:
    size = 3;
    break;
  //Double bit-input objects
  case 0x0300:
    size = 0;
    break;
  case 0x0301:
    size = 1;
    break;
  case 0x0302:
    size = 1;
    break;
  case 0x0400:
    size = 0;
    break;
  case 0x0401:
    size = 1;
    break;
  case 0x0402:
    size = 7;
    break;
  case 0x0403:
    size = 3;
    break;
  //Binary Output Objects
  case 0x0a00:
    size = 1;
    break;
  case 0x0a01:
    size = 1;
    break;
  case 0x0a02:
    size = 1;
    break;
  case 0x0b00:
    size = 1;
    break;
  case 0x0b01:
    size = 1;
    break;
  case 0x0b02:
    size = 7;
    break;
  case 0x0c01:
    size = 4;
    break;
  //Counter Objects
  case 0x1400:
    size = 0;
    break;
  case 0x1401:
    size = 5;
    break;
  case 0x1402:
    size = 3;
    break;
  case 0x1403:
    size = 5;
    break;
  case 0x1404:
    size = 3;
    break;
  case 0x1405:
    size = 4;
    break;
  case 0x1406:
    size = 2;
    break;
  case 0x1407:
    size = 4;
    break;
  case 0x1408:
    size = 2;
    break;
  case 0x1500:
    size = 0;
    break;
  case 0x1501:
    size = 5;
    break;
  case 0x1502:
    size = 3;
    break;
  case 0x1503:
    size = 5;
    break;
  case 0x1504:
    size = 3;
    break;
  case 0x1505:
    size = 11;
    break;
  case 0x1506:
    size = 9;
    break;
  case 0x1507:
    size = 11;
    break;
  case 0x1508:
    size = 9;
    break;
  case 0x1509:
    size = 4;
    break;
  case 0x150a:
    size = 2;
    break;
  case 0x150b:
    size = 4;
    break;
  case 0x150c:
    size = 2;
    break;
  case 0x1600:
    size = 0;
    break;
  case 0x1601:
    size = 5;
    break;
  case 0x1602:
    size = 3;
    break;
  case 0x1603:
    size = 5;
    break;
  case 0x1604:
    size = 3;
    break;
  case 0x1605:
    size = 11;
    break;
  case 0x1606:
    size = 9;
    break;
  case 0x1607:
    size = 11;
    break;
  case 0x1608:
    size = 9;
    break;
  case 0x1700:
    size = 0;
    break;
  case 0x1701:
    size = 5;
    break;
  case 0x1702:
    size = 3;
    break;
  case 0x1703:
    size = 5;
    break;
  case 0x1704:
    size = 3;
    break;
  case 0x1705:
    size = 11;
    break;
  case 0x1706:
    size = 9;
    break;
  case 0x1707:
    size = 11;
    break;
  case 0x1708:
    size = 9;
    break;
  //Analog Input Objects
  case 0x1e00:
    size = 0;
    break;
  case 0x1e01:
    size = 5;
    break;
  case 0x1e02:
    size = 3;
    break;
  case 0x1e03:
    size = 4;
    break;
  case 0x1e04:
    size = 2;
    break;
  case 0x1e05:
    size = 4;//**********
    break;
  case 0x1e06:
    size = 8;//**********
    break;
  case 0x1f01:
    size = 5;
    break;
  case 0x1f02:
    size = 3;
    break;
  case 0x1f03:
    size = 11;
    break;
  case 0x1f04:
    size = 9;
    break;
  case 0x1f05:
    size = 4;
    break;
  case 0x1f06:
    size = 2;
    break;
  case 0x1f07:
    size = 4;
    break;
  case 0x1f08:
    size = 8;
    break;
  case 0x2000:
    size = 0;
    break;
  case 0x2001:
    size = 5;
    break;
  case 0x2002:
    size = 3;
    break;
  case 0x2003:
    size = 4;
    break;
  case 0x2004:
    size = 2;
    break;
  case 0x2005:
    size = 5;
    break;
  case 0x2006:
    size = 9;
    break;
  case 0x2007:
    size = 11;
    break;
  case 0x2008:
    size = 15;
    break;
  case 0x2101:
    size = 5;
    break;
  case 0x2102:
    size = 3;
    break;
  case 0x2103:
    size = 11;
    break;
  case 0x2104:
    size = 9;
    break;
  case 0x2105:
    size = 5;
    break;
  case 0x2106:
    size = 9;
    break;
  case 0x2107:
    size = 11;
    break;
  case 0x2108:
    size = 15;
    break;
  case 0x2200:
    size = 0;
    break;
  case 0x2201:
    size = 3;
    break;
  case 0x2202:
    size = 5;
    break;
  case 0x2203:
    size = 5;
    break;
  //Analog Output Objects
  case 0x2800:
    size = 0;
    break;
  case 0x2801:
    size = 5;
    break;
  case 0x2802:
    size = 3;
    break;
  case 0x2803:
    size = 5;
    break;
  case 0x2804:
    size = 9;
    break;
  case 0x2901:
    size = 4;
    break;
  case 0x2902:
    size = 2;
    break;
  case 0x2903:
    size = 4;
    break;
  case 0x2904:
    size = 8;
    break;
  case 0x2a00:
    size = 0;
    break;
  case 0x2a01:
    size = 5;
    break;
  case 0x2a02:
    size = 3;
    break;
  case 0x2a03:
    size = 11;
    break;
  case 0x2a04:
    size = 9;
    break;
  case 0x2a05:
    size = 5;
    break;
  case 0x2a06:
    size = 9;
    break;
  case 0x2a07:
    size = 11;
    break;
  case 0x2a08:
    size = 15;
    break;
  // Time Objects
  case 0x3200:
    size = 0;
    break;
  case 0x3201:
    size = 6;
    break;
  case 0x3202:
    size = 6;
    break;
  case 0x3203:
    size = 6;
    break;
  case 0x3301:
    size = 6;
    break;
  case 0x3302:
    size = 6;
    break;
  case 0x3401:
    size = 2;
    break;
  case 0x3402:
    size = 2;
    break;
  //Class Data Objects
  case 0x3c01:
    size = 0;
    break;
  case 0x3c02:
    size = 0;
    break;
  case 0x3c03:
    size = 0;
    break;
  case 0x3c04:
    size = 0;
    break;
  //File Objects
  case 0x4603:
    size = 0;
    break;
  case 0x4604:
    size = 0;
    break;
  case 0x4605:
    size = 0;
    break;
  case 0x4606:
    size = 0;
    break;
  //Devices Objects
  case 0x5001:
    size = 1;
    break;
  //Data Sets
  case 0x5501:
    size = 0;
    break;
  case 0x5601:
    size = 0;
    break;
  case 0x5602:
    size = 0;
    break;
  case 0x5603:
    size = 0;
    break;
  case 0x5701:
    size = 0;
    break;
  case 0x5801:
    size = 0;
    break;
  //Octect Strings Objects
  case 0x6e00:
    size = 0;
    break;
  case 0x6f00:
    size = 0;
    break;
  //Virtual Terminal Objects
  case 0x7000:
    size = 0;
    break;
  case 0x7100:
    size = 0;
    break;
  }
  return size;
}


//TODO: Contemplar el formato de salida de todos los objetos
void printStaticObject (uint8_t grupo, uint8_t variacion, unsigned char valor[], FILE * fich) {
  //En funci??n del tama??o del objeto y de su formato hacer su propio formato de impresi??n
  uint16_t object = (grupo << 8) | variacion;
  switch (object) {
  case 0x3201:
    fprintf(fich, "0x%x%x%x%x%x;", valor[4], valor[3], valor[2], valor[1], valor[0]);
    break;
  case 0x3301:
    fprintf(fich, "0x%x%x%x%x%x;", valor[4], valor[3], valor[2], valor[1], valor[0]);
    break;
  case 0x3402:
    fprintf(fich, "0x%x%x", valor[1], valor[0]);
    break;
  case 0x3c01:
    fprintf(fich, "Null;");
    break;
  case 0x3c02:
    fprintf(fich, "Null;");
    break;
  case 0x3c03:
    fprintf(fich, "Null;");
    break;
  case 0x3c04:
    fprintf(fich, "Null;");
    break;
  case 0x0203:
    if (valor[0] & 0x80){
      fprintf(fich, "1;");
    }
    else {
      fprintf(fich, "0;");
    }
    break;
  case 0x2001:
    fprintf(fich, "%d%d%d%d;", valor[4], valor[3], valor[2], valor[1]);
    break;
  default:
    fprintf(fich, "No implementado;");
    break;
  }
}
