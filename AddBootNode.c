#include "AddBootNode.h"
#include "mydev.h"
#include "ide-drive.h"
#include "ata.h"

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

    if(myUnit->mdu_drv_type!=ATA_DRV && myUnit->mdu_drv_type!=SATA_DRV ){ //add only harddisks. No CDROMS and Unknown
        close_io(harddisk);
        return 0;
    }

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

int isIDEDevice(char* in){
    char* ref="ide.device";
    int i, size;
    //find sizes
    size=0;
    while(in[size]){
        ++size;
    }
    i=0;
    while(ref[i]){
        ++i;
    }
    //same size?
    if(i!=size) return 0;

    for(i=0; i<size;++i){
        if(ref[i]!=in[i]) break;
    }
    return i==size;
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
        long force;
        long test;
    } args = {
        0
    };
    int returnCode = 6;
    int driveStatus;
    int i;
    int minU=0, maxU=10, step=10;
    int defaultDevice = 0;
    int numOfDevices=0;
    unsigned short int buf[256];
    int *flagUseDrive;
    //fill default values
    args.device="ide.device";
    args.minUnit=&minU;
    args.maxUnit=&maxU;
    args.stepUnit=&step;

    //printf("Dev: %s MinU:%d MaxU:%d File:%s step:%d info:%d force:%d\n",args.device, (*args.minUnit), (*args.maxUnit),args.file,(*args.stepUnit),args.info,args.force);
    if(!isKick13()){
        rdargs = ReadArgs ("DEVICE/K,MINUNIT/K/N,MAXUNIT/K/N,FILE/K,STEP/K/N,DRIVEINFO/S,FORCE/S,TEST/S",(void *)&args,NULL);
    }
    else{
        printf("Kick 1.3 mode! No command line parameters are evaluated except the first.\n First parameter is a file to save the mountlist.\n");
        if(argc>1){
            args.file=argv[1];
        }
    }
    if((*args.stepUnit)<=0) //sanity check
    	(*args.stepUnit)=1;
    //printf("Dev: %s MinU:%d MaxU:%d File:%s step:%d info:%d force:%d\n",args.device, (*args.minUnit), (*args.maxUnit),args.file,(*args.stepUnit),args.info,args.force);
    
    numOfDevices = ((*args.maxUnit)/(*args.stepUnit)) + 1;
    flagUseDrive = AllocMem(numOfDevices*sizeof(int),MEMF_CLEAR);
    
    if (flagUseDrive){
        //are we using the default device?
        defaultDevice= isIDEDevice(args.device);
        for(i=(*args.minUnit)/(*args.stepUnit); i<numOfDevices; ++i){
            //do presence check only on the default device
            if(defaultDevice && args.info){
                switch(i){
                    case 0:
                        driveStatus = DriveStatus(DRV0, buf);
                        break;
                    case 1:
                        driveStatus = DriveStatus(DRV1, buf);
                        break;
                }
                flagUseDrive[i]=driveStatus==SUCCESS||args.force;
                //print info

                if(args.info){
                    args.test=args.info;
                    switch(driveStatus){
                        case SUCCESS:
                            printf("Device %s drive %d is a ATA-DRIVE\n",args.device, i);
                            PrintDriveInformation(buf);
                            break;
                        case TIME_OUT:
                            printf("Device %s drive %d not present\n",args.device, i);
                            break;
                        case DATA_PENDING:
                            printf("Device %s drive %d has data pending.\n",args.device, i);
                            break;
                        case READ_ERROR:
                            printf("Device %s drive %d has a read error\n",args.device, i);
                            break;
                        case WRITE_ERROR:
                            printf("Device %s drive %d has a write error\n",args.device, i);
                            break;
                        case ATAPI_DRIVE:
                            printf("Device %s drive %d is a ATAPI-DRIVE (CD/DVD-ROM)\n",args.device, i);
                            PrintDriveInformation(buf);
                            break;
                    }
                }
            }
            else{
                flagUseDrive[i]=1;
            }
        }

        if(defaultDevice)
            IDESoftReset();

        for(i=(*args.minUnit)/(*args.stepUnit); i<numOfDevices; ++i){
            if(flagUseDrive[i]){
                driveStatus=AddBootNodes(args.device,i*(*args.stepUnit),args.file,args.test);
                if(driveStatus<returnCode){
                    returnCode=driveStatus;
                }
            }
        }
        FreeMem(flagUseDrive,numOfDevices*sizeof(int));
    }

    if(rdargs)
        FreeArgs (rdargs);
    exit( returnCode  );
}


