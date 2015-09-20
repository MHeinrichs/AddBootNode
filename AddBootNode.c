#include "AddBootNode.h"
#include "mydev.h"

int writeMountlist(APTR *blocklist, int isKick13, char* device, int devicenum, char* fileName){
    long i,j;
    long rc=0;
    struct PartitionBlock *pb;
    char name[5]={0};//empty string
    FILE *outfile = fopen(fileName,"a");

    if(!outfile){
        return -1;
    }

    for(i=0;blocklist[i] ;++i){
        //check for partitionblock
        if(((struct RigidDiskBlock *)(blocklist[i]))->rdb_ID == IDNAME_PARTITION){
            pb = (struct *PartitionBlock)blocklist[i];

            if(isKick13 && pb->pb_Environment[DE_DOSTYPE]>0x444F5301){ //more than FFS: skip in kick 1.3!
                continue;
            }
            for (j=0;j<sizeof(long);++j){
                name[j]=pb->pb_DriveName[j+1];
            }

            fprintf(outfile,"%s:\n", name); //name:
            fprintf(outfile,"\tDevice = %s\n",device);

            if(isKick13 && pb->pb_Environment[DE_DOSTYPE]==0x444F5301){ //FFS
                fprintf(outfile,"\tFileSystem = L:FastFileSystem\n");
            }
            fprintf(outfile,"\tUnit = %lu\n",devicenum);
            fprintf(outfile,"\tFlags = %lu\n", pb->pb_DevFlags);
            fprintf(outfile,"\tSurfaces = %lu\n", pb->pb_Environment[DE_NUMHEADS]);
            fprintf(outfile,"\tBlocksPerTrack = %lu\n", pb->pb_Environment[DE_BLKSPERTRACK]);
            fprintf(outfile,"\tReserved = %lu\n", pb->pb_Environment[DE_RESERVEDBLKS]);
            fprintf(outfile,"\tInterleave = %lu\n", pb->pb_Environment[DE_INTERLEAVE]);
            fprintf(outfile,"\tLowCyl = %lu\n", pb->pb_Environment[DE_LOWCYL]);
            fprintf(outfile,"\tHighCyl = %lu\n", pb->pb_Environment[DE_UPPERCYL]);
            fprintf(outfile,"\tBuffers = %lu\n", pb->pb_Environment[DE_NUMBUFFERS]);
            fprintf(outfile,"\tGlobVec = -1\n");
            fprintf(outfile,"\tBufMemType = %lu\n", pb->pb_Environment[DE_BUFMEMTYPE]);
            fprintf(outfile,"\tMaxTransfer = 0x%08lx\n", pb->pb_Environment[DE_MAXTRANSFER]);
            fprintf(outfile,"\tMask = 0x%08lx\n", pb->pb_Environment[DE_MASK]);
            fprintf(outfile,"\tBootPri = %lu\n", pb->pb_Environment[DE_BOOTPRI]);
            fprintf(outfile,"\tDosType = 0x%08lx\n", pb->pb_Environment[DE_DOSTYPE]);
            fprintf(outfile,"\tMount = 1\n");
            fprintf(outfile,"#\n\n");
        }
    }
    fclose(outfile);
    return 0;
}

UWORD GetDriveInfo(char* device, int devicenum, int verbose){
    struct iohandle *harddisk;
    struct MyUnit *myUnit=NULL;
	UWORD retVal = ATA_DRV;
    char* ref="ide.device";
	ULONG capacity;
	//check for IDE.device all others are treated as ATA_DRV
	if(stricmp(device,ref)==0 && verbose){
		//OK, ide.device! Lets go checking!	    
	    //get the memory
	    harddisk = open_device(device, devicenum);
	
		//got the handle?
	    if(harddisk  ){
	    	//get a pointer to the unit-info struct
		    myUnit = (struct MyUnit *) ((struct IOStdReq *)harddisk->hand.req)->io_Unit;
		    //store return value
			retVal = myUnit->mdu_drv_type;
		    if(verbose!=0 ){ //check if verbose is given
		    	printf("Device %s\tUnit%d:\n",device,devicenum);
		    	switch(retVal){
		    		case ATA_DRV: printf("Type: ATA Harddisk\n");break;
		    		case ATAPI_DRV: printf("Type: ATAPI-CD/DVD-ROM\n");break;
		    		case UNKNOWN_DRV: printf("Type: Unknownn");break;
		    		case SATA_DRV: printf("Type: SATA Harddisk\n");break;
		    		case SATAPI_DRV: printf("Type: SATAPI-CD/DVD-ROM\n");break;
		    	}
		    	printf("Model number: %s.\n",myUnit->mdu_model_num);
		    	printf("Firmware revision: %s.\n",myUnit->mdu_firm_rev);
		    	printf("Serial: %s.\n",myUnit->mdu_ser_num);
				if(retVal==ATA_DRV || retVal==SATA_DRV ){
					switch(	myUnit->mdu_lba){
						case CHS_ACCESS: 
							printf("Supports only cylinder-head-sector (CHS) access.\n");
							break;	
						case LBA28_ACCESS: 							
							printf("Supports LBA28 access.\n");
							printf("Number of blocks %lu\n",myUnit->mdu_numlba);
							break;
						case LBA48_ACCESS: 							
							printf("Supports LBA48 access. (not supported jet!)\n");
							printf("Number of 48LBA-blocks %lu\n",myUnit->mdu_numlba48);
							printf("Number of 28LBA-blocks %lu\n",myUnit->mdu_numlba);
							break;
					}		
					printf(	"Cylinders: %lu Heads: %lu Sectors: %lu\n",
							myUnit->mdu_cylinders,
							myUnit->mdu_heads,
							myUnit->mdu_sectors_per_track);
					capacity = myUnit->mdu_cylinders*myUnit->mdu_heads*myUnit->mdu_sectors_per_track; //c*h*s = num of blocs, one block =512Byte 
					capacity /=2; //blocks (512byte) to kb									
					capacity /=1024; //to MB
					//if(capacity >1024){
					//	capacity /=1024; //to GB
					//	printf(	"Capacity(GB): %lu\n",capacity);
					//}
					//else{
						printf(	"Capacity(MB): %lu\n",capacity);
					//}							

				}	
				if(retVal==ATAPI_DRV || retVal==SATAPI_DRV ){
					printf("Disk present: %s\n",(myUnit->mdu_no_disk?"NO":"YES"));
					printf("Motor status: %d\n",myUnit->mdu_motor);
					printf("Number of LBA-blocks %lu\n",myUnit->mdu_numlba);					
				}
				printf("Max number of sectors per IO (NOT MAX TRAFSFER!): %d\n",myUnit->mdu_SectorBuffer);
			}
		    close_io(harddisk);
	    }
	    else if (verbose){
			printf("could not access unit %2d on %s!\n",devicenum,device);
			retVal = UNKNOWN_DRV;
		}	    
	}
	else if (verbose){
		printf("No info available for %s. Only ide.device supported!\n",device);
	}
	return retVal;
}

int AddBootNodes(char* device, int devicenum, char* outfile, long test){
    long rc =0, numPartAdded=0;
    long i,j;
    BOOL isKick13;
    BOOL isFFSLoaded = FALSE;
    struct DeviceNode* node;
    struct iohandle *harddisk;
    struct MyUnit *myUnit=NULL;
    unsigned char name[5]={0};
    APTR *blocklist;
    ULONG paramPkt[21]={0};
    struct PartitionBlock *pb;
    char* fileSystem = "L:FastFileSystem";
    //get the memory
    harddisk = open_device(device, devicenum);

    if(NULL == harddisk  ){
        return 0;
    }

    myUnit = (struct MyUnit *) ((struct IOStdReq *)harddisk->hand.req)->io_Unit;

    blocklist = AllocMem(MAXBLK * sizeof(APTR),MEMF_CLEAR);
    if(NULL == blocklist){
        close_io(harddisk);
        return -2;
    }

    //open rdb from harddisk
    if(!read_rdb(harddisk,blocklist)){
        //error: aboard
        close_io(harddisk);
        free_rdb(blocklist);
        FreeMem(blocklist,MAXBLK * sizeof(APTR));
        return 0; //this usually means we have an uninitialized harddisk without rdb
    }

    //io end
    close_io(harddisk);

    ExpansionBase=OpenLibrary("expansion.library",0L);
    if (ExpansionBase){
        //version check
        isKick13 = ExpansionBase->lib_Version <36;
        isFFSLoaded= !isKick13;
        rc=1;

        if(outfile){
            rc = writeMountlist(blocklist, isKick13, device, devicenum, outfile);
        }
        else{
         
            //iterate over blocks and add partitions
            for(i=0;blocklist[i] && rc!=0 ;++i){

                if(((struct RigidDiskBlock *)(blocklist[i]))->rdb_ID == IDNAME_PARTITION){
                    pb = (struct *PartitionBlock)blocklist[i];

                    for (j=0;j<sizeof(long) ;++j){
                        name[j]=pb->pb_DriveName[j+1];
                    }

                    paramPkt[0]=(ULONG) name;
                    paramPkt[1]=(ULONG) device;
                    paramPkt[2]=(ULONG) devicenum;
                    paramPkt[3]=(ULONG) pb->pb_DevFlags;
                    //pb_Environment holds the partition information
                    //copy 17 ULONGS from pb_Environment to paramPktwith offset of four

                    CopyMemQuick(pb->pb_Environment, &(paramPkt[4]),17*sizeof(ULONG));

                    if((node = (APTR)MakeDosNode( paramPkt ))){
			//only add OFS Partitions to Kick 1.3, if FFS is not loaded!
                        if(paramPkt[20]<0x444F5301 || (paramPkt[20]==0x444F5301 && isFFSLoaded) || !isKick13){
                            if(isKick13 && paramPkt[20]>=0x444F5301){
                                printf("NO support for FFS or other partitions in kick 1.3!\nPlease mount manualy: %b\n",node->dn_Name+1);
                                FreeMem(node, sizeof(struct DeviceNode));
                            }
                            else{
                                if(test){
                                    printf("Found partition %s with start cyl %ld and end cyl %ld\n",name,pb->pb_Environment[DE_LOWCYL],pb->pb_Environment[DE_UPPERCYL]);
                                } else{
                                    rc = AddDosNode( 0, ADNF_STARTPROC, node);
                                }
                            }
                        }
                        else{
                            FreeMem(node, sizeof(struct DeviceNode));
                        }
                    }

                    if(rc){ //success?
                        ++numPartAdded;
                    }
                }
            }
        }

        CloseLibrary(ExpansionBase);
    }

    //free mem
    free_rdb(blocklist);
    FreeMem(blocklist,MAXBLK * sizeof(APTR));
    if(numPartAdded){
        return 0;
    }
    else{ //Nothing added
        return 5;
    }
}

long isKick13(){
    long isKick13=0;
    ExpansionBase=OpenLibrary("expansion.library",0L);
    if (ExpansionBase){
        //version check
        isKick13 = ExpansionBase->lib_Version <36;
        CloseLibrary(ExpansionBase);
    }
    return isKick13;
}

main(int argc, char *argv[])
{
    struct RDArgs *rdargs=NULL;
    struct {
        char *device;
        long *minUnit;
        long *maxUnit;
        char *file;
        long *stepUnit;
        long info;
        long test;
    } args = {
        0
    };
    int returnCode = 6;
    int i, unit,driveStatus;
    int minU=0, maxU=10, step=10;
    int defaultDevice = 0;
    int numOfDevices=0;
    UWORD driveType = UNKNOWN_DRV;
    //fill default values
    args.device="ide.device";
    args.minUnit=&minU;
    args.maxUnit=&maxU;
    args.stepUnit=&step;

    if(!isKick13()){
        rdargs = ReadArgs ("DEVICE/K,MINUNIT/K/N,MAXUNIT/K/N,FILE/K,STEP/K/N,DRIVEINFO/S,TEST/S",(void *)&args,NULL);
    }
    else{
        printf("Kick 1.3 mode! No command line parameters are evaluated except the first.\n First parameter is a file to save the mountlist.\n");
        if(argc>1){
            args.file=argv[1];
        }
    }
    if((*args.stepUnit)<=0) //sanity check
    	(*args.stepUnit)=1;
    //printf("Dev: %s MinU:%d MaxU:%d File:%s step:%d info:%d Test:%d\n",args.device, (*args.minUnit), (*args.maxUnit),args.file,(*args.stepUnit),args.info,args.test);
    
    numOfDevices = ((*args.maxUnit)/(*args.stepUnit)) + 1;

    for(i=(*args.minUnit)/(*args.stepUnit); i<numOfDevices; ++i){
    	unit = i*(*args.stepUnit);
	    //printf("Dev: %s unit:%d info:%d test:%d\n",args.device, unit, args.info,args.test);
    	driveType = GetDriveInfo(args.device,unit,args.info); //this returns the drivetype for ide.device or ATA_DRV for any other device
     	if(driveType==ATA_DRV || driveType==SATA_DRV ){ //add only harddisks. No CDROMS and Unknown   	
	        driveStatus=AddBootNodes(args.device,unit,args.file,args.test);
	        if(driveStatus<returnCode){
	            returnCode=driveStatus;
	        }
	    }
    }

    if(rdargs)
        FreeArgs (rdargs);
    exit( returnCode  );
}


