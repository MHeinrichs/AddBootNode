/*  */

#include <exec/ports.h>
#include <exec/devices.h>
#include <exec/tasks.h>

#define CMD_MYCMD    29
#define MYPROCSTACKSIZE   0x1000
struct MyOldUnit {
   struct   Unit mdu_Unit;
   ULONG    mdu_UnitNum;
   UBYTE    mdu_SigBit;
   UBYTE    mdu_pad;
   APTR     mdu_Device;
   ULONG    mdu_drv_type;        /*see bellow for possible values*/
   ULONG    mdu_firstcall;       /*was drive called yet?*/
   ULONG    mdu_auto;            /*get drive parameters automatic? = TRUE*/
   ULONG    mdu_lba;             /*use LBA? For ATAPI always TRUE*/
   ULONG    mdu_sectors_per_track;  /*only for ATA*/
   ULONG    mdu_heads;           /*only for ATA*/
   ULONG    mdu_cylinders;       /*only for ATA*/
   ULONG    mdu_numlba;          /*only for ATA with LBA=TRUE*/
   char     mdu_ser_num[22];     /*serial number*/
   char     mdu_firm_rev[48];    /*firware revision*/
   char     mdu_model_num[56];   /*model number*/
   ULONG    mdu_motor;           /*motor status*/
   ULONG    mdu_change_cnt;      /*count of disk changes - only for ATAPI*/
   ULONG    mdu_no_disk;         /*isn't disk inserted? - only for ATAPI*/
};

struct MyUnit {
   struct   Unit mdu_Unit;
   APTR     mdu_Device;
   ULONG    mdu_UnitNum;
   ULONG    mdu_change_cnt;      /*count of disk changes - only for ATAPI*/
   ULONG    mdu_no_disk;         /*isn't disk inserted? - only for ATAPI*/
   ULONG    mdu_numlba48;        /*only for ATA with LBA=LBA48_ACCESS*/
   ULONG    mdu_sectors_per_track;  /*only for ATA*/
   ULONG    mdu_heads;           /*only for ATA*/
   ULONG    mdu_cylinders;       /*only for ATA*/
   ULONG    mdu_numlba;          /*only for ATA with LBA=LBA28_ACCESS or LBA48_ACCESS*/
   char     mdu_ser_num[24];     /*serial number*/
   char     mdu_firm_rev[12];    /*firware revision*/
   char     mdu_model_num[44];   /*model number*/
   UWORD    mdu_drv_type;        /*see bellow for possible values*/
   UWORD    mdu_firstcall;       /*was drive called yet?*/
   UWORD    mdu_auto;            /*get drive parameters automatic? = TRUE*/
   UWORD    mdu_lba;             /*use LBA? For ATAPI always TRUE*/
   UWORD    mdu_motor;           /*motor status*/
   UBYTE    mdu_SigBit;
   UBYTE    mdu_SectorBuffer;	 //max number of sectors per transfer block
   UBYTE    mdu_actSectorCount;	 //actual number of sectors per transfer block
   UBYTE    mdu_pad;
};

struct MyUnit225 {

   ;struct   Unit mdu_Unit;
   char     mdu_PortSpacer[48];   //spacer for messageportstructure (is only 32byte, but who knows!)
   APTR     mdu_Device;
   ULONG    mdu_UnitNum;
   ULONG    mdu_change_cnt;      /*count of disk changes - only for ATAPI*/
   ULONG    mdu_no_disk;         /*isn't disk inserted? - only for ATAPI*/
   ULONG    mdu_numlba48;        /*only for ATA with LBA=LBA48_ACCESS*/
   ULONG    mdu_sectors_per_track;  /*only for ATA*/
   ULONG    mdu_heads;           /*only for ATA*/
   ULONG    mdu_cylinders;       /*only for ATA*/
   ULONG    mdu_numlba;          /*only for ATA with LBA=LBA28_ACCESS or LBA48_ACCESS*/
   ULONG    mdu_act_Actual;			 /*SCSI-Packet-Stuff*/
   char     mdu_sense_data[20];  /*data for sense scsi-packet*/
   char     mdu_ser_num[24];     /*serial number*/
   char     mdu_firm_rev[12];    /*firware revision*/
   char     mdu_model_num[44];   /*model number*/
   char     mdu_act_cmd[16];      /*actual SCSI-Command (8 words = 16 bytes)*/
   char     mdu_EmulInquiry[36];
   char     mdu_EmulMSPage3[28];
   char     mdu_EmulMSPage4[28];
   char     mdu_rs_cmd[12];
   UWORD    mdu_drv_type;        /*see bellow for possible values*/
   UWORD    mdu_firstcall;       /*was drive called yet?*/
   UWORD    mdu_auto;            /*get drive parameters automatic? = TRUE*/
   UWORD    mdu_lba;             /*use LBA? For ATAPI always TRUE*/
   UWORD    mdu_motor;           /*motor status*/
   UBYTE    mdu_SigBit;
   UBYTE    mdu_SectorBuffer;	 //max number of sectors per transfer block
   UBYTE    mdu_actSectorCount;	 //actual number of sectors per transfer block
   UBYTE    mdu_act_Flags;        //actual SCSI-Packet Flags
   UBYTE    mdu_act_Status;			 //actual SCSI-Status
   UBYTE    mdu_pad1;
};

struct MyUnit230 {

   struct   Unit mdu_Unit;
	 UBYTE    mdu_SigBit;
	 UBYTE    mdu_TimerOpen;	 
   char     mdu_EmulInquiry[36];
   char     mdu_EmulMSPage3[28];
   char     mdu_EmulMSPage4[28];
   char     mdu_rs_cmd[12];
   char     mdu_sense_data[20];  /*data for sense scsi-packet*/
   char     mdu_ser_num[24];     /*serial number*/
   char     mdu_firm_rev[12];    /*firware revision*/
   char     mdu_model_num[44];   /*model number*/
   char     mdu_act_cmd[16];      /*actual SCSI-Command (8 words = 16 bytes)*/
	 APTR     mdu_Device;
	 APTR     mdu_ATARdWt;
	 ULONG    mdu_change_cnt;
	 ULONG    mdu_no_disk;
	 ULONG    mdu_numlba48;
	 ULONG    mdu_sectors_per_track;
	 ULONG    mdu_heads;
	 ULONG    mdu_cylinders;
	ULONG    mdu_numlba;
	ULONG    mdu_act_Actual;
	UWORD    mdu_drv_type;
	UWORD    mdu_lba;
	UWORD    mdu_motor;
	UBYTE    mdu_actSectorCount;
	UBYTE    mdu_act_Flags;
	UBYTE    mdu_act_Status;
	UBYTE    mdu_UnitNum;
	UBYTE    mdu_firstcall;
	UBYTE    mdu_auto;
	ULONG    mdu_timeout;
	APTR     mdu_msgport;
	APTR     mdu_timeinterrupt;
	APTR     mdu_timerequest;
  UBYTE    mdu_stack[MYPROCSTACKSIZE];
	struct   Task mdu_tcb; 					//Task Control Block (TCB) for disk task   
};
/*drive types*/
#define ATA_DRV      0
#define ATAPI_DRV    1
#define UNKNOWN_DRV  2
#define SATA_DRV     3
#define SATAPI_DRV   4
#define CHS_ACCESS   0
#define LBA28_ACCESS 1
#define LBA48_ACCESS 2

#define MAXDEVICE 1
