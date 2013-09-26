/**************************************************************************************
  Copyright (c) 2009- Peking Supertv-DTV technology Ltd.
  All rights reserved

filename: CRC16.h
Description: CRC encode and decode
Other: None

Revision History
Data                 Author               Creation/Modification
2009-08-10          xuwenshan             CRC encode and decode
 **************************************************************************************/

#ifndef SUPERTV_INSTITUTE_REAL_TIME_ENCODING_CRC16_20090824
#define SUPERTV_INSTITUTE_REAL_TIME_ENCODING_CRC16_20090824

//global function
unsigned short pes_crc16(const unsigned char *p_info, unsigned int len);

//unsigned short CRC16Encode( const unsigned char* p_info, unsigned int len );
//unsigned short CRC16Decode( const unsigned char* p_info, unsigned int len );
//int CalculateCRC16TableFor8Bit( unsigned short* p_crc16_table_8bit );
//unsigned short CalculateCRC16( unsigned int index );

#endif
