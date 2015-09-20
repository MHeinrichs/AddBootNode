/* This program prints drive information from the IDE-drive */
/* Last revised on 3-November-2005*/

#include <stdio.h>
#include <stdlib.h>
#include "ide-drive.h"

BYTE_WIDE lastHead=0xff;


BYTE_WIDE delayCIA(){ //one CIA acces: 1.4 micro sec
   return(CIA_TST);
}

WORD_WIDE delayShort(){
    return (CHP_TST);
}


BYTE_WIDE setHead(BYTE_WIDE head){
    BYTE_WIDE result = 0xFF;
    if(lastHead!=head){
        lastHead=head;
        TF_DRIVE_HEAD=head;
        //TF_DRIVE_HEAD=head;
        for (int time=0;time<5;++time){ //after head change: wait 400ns or better read five times TF_STAUS!
            result=TF_STATUS;
        }
    }
    return result;
}

void IDESoftReset(){
    int time;
    TF_DEVICE_CONTROL=8+nIEN+SRST;   /* Soft Reset Both drives + No Interrupt Request*/
    delayShort();
    for (time=0;time<16;++time){
        delayCIA();
    } 
    TF_DEVICE_CONTROL=8+nIEN;        /* end Reset, recover 200ms */
    delayShort();
    for (time=0;time<175;++time){
        delayCIA();

    }
}

void readDataSwap(WORD_WIDE* buf){
    for (int i=0;i<256;++i){
        buf[i]=TF_DATA; /* read data */
        buf[i]=((buf[i]>>8)&0xff)|((buf[i]&0xff)<<8);
    }
}

void readData(WORD_WIDE* buf){
    for (int i=0;i<256;++i){
        buf[i]=TF_DATA; /* read data */
    }
}

int DriveStatus(BYTE_WIDE drive, WORD_WIDE* buf){

    int i;
    unsigned erroria=0;
    IDESoftReset();


    erroria=setHead(drive); //LBA access!
    delayShort();
    if (erroria&ERR){
        return READ_ERROR; //error reading
    }

    //delayCIA();
    i=20000;
    TF_COMMAND=ATA_IDENTIFY_DRIVE;
    delayShort();

    erroria=TF_ALTERNATE_STATUS;
    delayShort();
    while (erroria&BSY!=0&&i>0 )
    {
        erroria = TF_ALTERNATE_STATUS;
        delayShort();
        i--;
    };

    erroria = TF_STATUS; //an atapi drive reports 0 here!

    if (erroria&ERR){
        return READ_ERROR; //error reading
    }

    if (erroria&DWF){
        return WRITE_ERROR; //error writing
    }

    if (erroria==0 ){ //atapi drive
        TF_COMMAND=ATA_IDENTIFY_PACKET_DEVICE;
        i=20000;
        erroria=TF_ALTERNATE_STATUS;
	    delayShort();
        while (erroria&BSY!=0&&i>0 )
        {
            erroria = TF_ALTERNATE_STATUS;
            delayShort();
            i--;
        };

        if(i>0){
            return DriveType(buf);
        }
        else{
            return TIME_OUT;
        }
    }

    if (i<=0){ //time out
        return TIME_OUT;
    }
    else{
        return DriveType(buf);

    }

}

int DriveType(WORD_WIDE* buf){
    unsigned erroria=0;
    int i;
    if(BYTESWAP){
        //swap bytes
        readDataSwap(buf);
    } else{
        readData(buf);
    }
    //erroria=TF_ALTERNATE_STATUS;
    //if (erroria&DRQ){
    //    return DATA_PENDING; //still data request afer read!
    //}
    erroria=buf[0]&(WORD_WIDE)0x8000;
    if(erroria==0){ //ATA_DRV
        IDESoftReset();
        return SUCCESS;

    }
    else if(erroria==0x8000){ // ATAPI_DRV
        TF_COMMAND=ATA_IDENTIFY_PACKET_DEVICE;
        i=20000;
        erroria=TF_ALTERNATE_STATUS;
 	    delayShort();
    	while (erroria&BSY!=0&&i>0 )
        {
            erroria = TF_ALTERNATE_STATUS;
            delayShort();
            i--;
        };

        if(i>0){
            if(BYTESWAP){
                //swap bytes
                readDataSwap(buf);
            } else{
                readData(buf);
            }

            erroria=buf[0]&(WORD_WIDE)0xC000;
            if(erroria==0x8000){
                IDESoftReset();
                return ATAPI_DRIVE;
            }
            else{
                IDESoftReset();
                return READ_ERROR;
            }
        }
        else{
            IDESoftReset();
            return READ_ERROR;
        }

    }
    IDESoftReset();
    return READ_ERROR;
}


void PrintDriveInformation(WORD_WIDE* buf){
    int i;
    unsigned long int temp32;
    //print some information
    if((buf[0]&0x8000)==0){ //ATA_DRV
        printf("ATA Drive\n");

    }
    else if((buf[0]&0x8000)==0x8000){ // ATAPI_DRV
        printf("ATAPI Drive\n");
    }
    else
        printf("Unknown drivetype\n");


    printf ("In translate mode a drive may expect different ");
    printf ("numbers for \ncyls/heads/sectors than the following.\n");
    printf ("Number of cylinders: %u\n",buf[1]);
    printf ("Number of heads: %u\n",buf[3]);
    printf ("Number of sectors per track: %u\n",buf[6]);
    printf ("Buffer size in 512 byte increments: %u\n",buf[21]);
    printf ("Serial number: ");
    for (i=0;i<20;i=i+2)
    {
        putchar (*((char *)buf+10*2+i));
        putchar (*((char *)buf+10*2+i+1));
    }
    printf ("\n");
    printf ("Firmware revision: "); /*8 ascii chars from wrd23*/
    for (i=0;i<8;i=i+2)
    {
        putchar (*((char *)buf+23*2+i));
        putchar (*((char *)buf+23*2+i+1));
    }
    printf ("\n");
    printf ("Model number: "); /* 40 ascii chars from 27*/
    for (i=0;i<40;i=i+2)
    {
        putchar (*((char *)buf+27*2+i));
        putchar (*((char *)buf+27*2+i+1));
    }
    printf ("\n");
    printf ("LBA28 Logical Block Addressing supported: ");
    if (buf[49]&(1<<9)){
        printf ("yes\n");
        printf ("Total number of user addressable LBA28 blocks: ");
        temp32=buf[61];
        temp32=(temp32 << 16) |buf[60];
        printf("\nLBA: %u\n",temp32);
    }
    else{
        printf ("no\n");
    }
    printf ("LBA48 Logical Block Addressing supported: ");
    if (buf[83]&(1<<10)){
        printf ("yes\n");
        printf ("Total number of user addressable LBA48 blocks: ");
        temp32=buf[103];
        temp32=(temp32 << 16)|buf[102];
        printf("\nLBA48: %u * ",temp32);
        temp32=buf[101];
        temp32=temp32 << 16 |buf[100];
        printf("%u\n",temp32);
    }
    else{
        printf ("no\n");
    }


    if ((buf[53]&1)==0){
        printf ("The following data reported for current\n");
        printf ("Cyls/Heads/Secs/Capacity may or may not be valid.\n");
    }
    printf ("Number of current cylinders: %u\n",buf[54]);
    printf ("Number of current heads: %u\n",buf[55]);
    printf ("Number of current sectors: %u\n",buf[56]);
    printf ("Current capacity in sectors: %u\n",buf[57]     +buf[58]<<16);
}

