#include "atid500.h" /* contains macros for the interface*/
#include "ata.h"

#define CIA_TST     (*(volatile BYTE_WIDE *)(0XBFE301)) //cia call 1.4micro sec
#define CHP_TST     (*(volatile WORD_WIDE *)(0X000004)) //chip mem call 400ns
#define TIME_OUT 0
#define READ_ERROR -1
#define DATA_PENDING -2
#define WRITE_ERROR -3
#define SUCCESS 1
#define ATAPI_DRIVE 2
#ifndef _IDEDRIVE
#define _IDEDRIVE

/**
   Function resets both drives on the bus
*/
void IDESoftReset();

/**
   Function to wait for 1.4ms
*/
BYTE_WIDE delayCIA();

/**
   Function to set the head or master/slave-drive if neccesary.
*/

BYTE_WIDE setHead(BYTE_WIDE head);

/**
   Function to read one sector (256 words) of data and swap high/low byte
*/
void readDataSwap(WORD_WIDE* buf);

/**
   Function to read one sector (256 words) of data.
*/
void readData(WORD_WIDE* buf);

/**
   Function to get the Drive information.

   Return codes:
   DATA_PENDING: Still Data Request after read of 512 byte
   READ_ERROR: Error reading data
   TIME_OUT: Timeout: Usually this means no drive
   SUCCESS: Success
*/
int DriveStatus(BYTE_WIDE drive, WORD_WIDE* buf);

int DriveType(WORD_WIDE*buf);


/**
   Function to print the drive information on screen.
*/
void PrintDriveInformation(WORD_WIDE* buf);
#endif
