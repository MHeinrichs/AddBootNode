/*

Big thanks to Thomas Rapp, who originally coded these functions!

visit him on:

http://thomas-rapp.homepage.t-online.de

*/
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

#include "rdblib.h"
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

struct iohandle *open_file (char *filename)

{
	struct iohandle *h;

	if (h = AllocMem (sizeof(struct iohandle),MEMF_CLEAR))
	{
		h->type    = IOHT_FILE;
		h->blksize = BLK_SIZE;
		h->name    = filename;
		if (h->hand.f = Open (filename,MODE_READWRITE))
		return (h);

		FreeMem (h, sizeof(struct iohandle));
	}

	return (NULL);
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

struct iohandle *open_device (char *device,long unit)

{
	struct iohandle *h;
	char *portName = "rdb-lib";
	//printf("10\n");
	if (h = AllocMem (sizeof(struct iohandle),MEMF_CLEAR))
	{
		struct MsgPort *port;
		h->type    = IOHT_DEVICE;
		h->blksize = BLK_SIZE;
		h->name    = device;
		//printf("11\n");

		if (port = (struct MsgPort*)CreatePort(portName,0))
		{
			//printf("12\n");

			if (h->hand.req = (struct IOStdReq *) CreateExtIO (port,sizeof(struct IOExtTD)))
			{
				//printf("13\n");
				if (!OpenDevice (device,unit,(struct IORequest *)h->hand.req,1))
				return (h);

				DeleteExtIO ((struct IORequest *)h->hand.req);
			}
			DeletePort (port);
		}
		FreeMem (h,sizeof(struct iohandle));
	}

	return (NULL);
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

void close_io (struct iohandle *h)

{
	struct MsgPort *port;

	if (h)
	{
		switch (h->type)
		{
			case IOHT_FILE:
			Close (h->hand.f);
			break;
			case IOHT_DEVICE:
			CloseDevice ((struct IORequest *)h->hand.req);
			port = h->hand.req->io_Message.mn_ReplyPort;
			DeleteExtIO ((struct IORequest *)h->hand.req);
			DeletePort (port);
			break;
		}

		FreeMem (h, sizeof(struct iohandle));
	}
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

APTR read_block (struct iohandle *h,ULONG block)

{
	APTR buffer;
	//printf("20\n");

	if (buffer = AllocMem (h->blksize,MEMF_CLEAR))
	{
		switch (h->type)
		{
			case IOHT_FILE:
			Seek (h->hand.f,block * h->blksize,OFFSET_BEGINNING);
			if (Read (h->hand.f,buffer,h->blksize) == h->blksize)
			return (buffer);
			break;
			case IOHT_DEVICE:
			h->hand.req->io_Command = CMD_READ;
			h->hand.req->io_Offset  = block * h->blksize;
			h->hand.req->io_Data    = buffer;
			h->hand.req->io_Length  = h->blksize;
			if (!DoIO ((struct IORequest *)h->hand.req))
			if (h->hand.req->io_Actual == h->blksize)
			return (buffer);
			break;
		}

		free_block (buffer);
	}

	return (NULL);
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

BOOL write_block (struct iohandle *h,ULONG block,APTR buffer)

{
	if (buffer)
	{
		switch (h->type)
		{
			case IOHT_FILE:
			Seek (h->hand.f,block * h->blksize,OFFSET_BEGINNING);
			if (Write (h->hand.f,buffer,h->blksize) == h->blksize)
			return (TRUE);
			break;
			case IOHT_DEVICE:
			h->hand.req->io_Command = CMD_WRITE;
			h->hand.req->io_Offset  = block * h->blksize;
			h->hand.req->io_Data    = buffer;
			h->hand.req->io_Length  = h->blksize;
			if (!DoIO ((struct IORequest *)h->hand.req))
			if (h->hand.req->io_Actual == h->blksize)
			return (TRUE);
			break;
		}
	}

	return (FALSE);
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

ULONG check_sum (APTR buffer)

{
	ULONG sum;
	ULONG *p = (ULONG *)buffer;
	long n,i;

	n = p[1];
	sum = p[0] + p[1];

	for (i = 3; i < n; i++)
	sum += p[i];

	return (-sum);
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

BOOL read_list (struct iohandle *h,APTR *blocklist,long *toblock,long *list_addr,ULONG idname,char *name)

{
	ULONG prevblock = 0;
	ULONG fromblock = *list_addr;
	BOOL ok = TRUE;
	long n = 0;
	//printf("30\n");

	while (fromblock != 0xffffffff)
	{
		struct FileSysHeaderBlock *b = read_block (h,fromblock);
		if (!b)
		{
			ok = FALSE;
			fromblock = 0xffffffff;
		}
		else if (b->fhb_ID != idname)
		{
			ok = FALSE;
			fromblock = 0xffffffff;
			free_block (b);
		}
		else if (check_sum(b) != b->fhb_ChkSum)
		{
			ok = FALSE;
			fromblock = 0xffffffff;
			free_block (b);
		}
		else
		{
			blocklist[*toblock] = b;
			n ++;
			if (prevblock)
			{
				((struct FileSysHeaderBlock *)(blocklist[prevblock]))->fhb_Next = *toblock;
				((struct FileSysHeaderBlock *)(blocklist[prevblock]))->fhb_ChkSum = check_sum(blocklist[prevblock]);
			}
			else
			*list_addr = *toblock;
			prevblock = *toblock;
			(*toblock) ++;
			fromblock = b->fhb_Next;
			if (idname == IDNAME_FILESYSHEADER)
			{
				read_list (h,blocklist,toblock,&b->fhb_SegListBlocks,IDNAME_LOADSEG,"file sys code");
				b->fhb_ChkSum = check_sum(b);
			}
		}
	}

	return (ok);
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

BOOL read_rdb (struct iohandle *h,APTR *blocklist)

{
	struct RigidDiskBlock *rdb;
	long fromblock,toblock;
	BOOL ok = TRUE;
	//printf("4\n");

	for (fromblock = 0; fromblock < RDB_LOCATION_LIMIT; fromblock ++)
	{
		rdb = read_block (h,fromblock);
		if (!rdb)
		{
			return (FALSE);
		}
		if (rdb->rdb_ID == IDNAME_RIGIDDISK)
		break;
		free_block(rdb);
	}

	if (rdb->rdb_ID != IDNAME_RIGIDDISK)
	{
		return (FALSE);
	}

	if (check_sum(rdb) != rdb->rdb_ChkSum)
	{
		return (FALSE);
	}

	blocklist[0] = rdb;
	toblock = 1;

	if (!read_list (h,blocklist,&toblock,(long *)&rdb->rdb_BadBlockList,IDNAME_BADBLOCK,"bad block")) ok = FALSE;
	if (!read_list (h,blocklist,&toblock,(long *)&rdb->rdb_PartitionList,IDNAME_PARTITION,"partition")) ok = FALSE;
	if (!read_list (h,blocklist,&toblock,(long *)&rdb->rdb_FileSysHeaderList,IDNAME_FILESYSHEADER,"file sys header")) ok = FALSE;
	if (!read_list (h,blocklist,&toblock,(long *)&rdb->rdb_DriveInit,IDNAME_LOADSEG,"drive init code")) ok = FALSE;

	rdb->rdb_ChkSum = check_sum(rdb);

	return (ok);
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

void move_rdb (APTR *blocklist,long startblock)

{
	long i;

	for (i = 0; blocklist[i]; i++)
	{
		switch (((struct RigidDiskBlock *)(blocklist[i]))->rdb_ID)
		{
			case IDNAME_RIGIDDISK:
			if (((struct RigidDiskBlock *)(blocklist[i]))->rdb_BadBlockList != 0xffffffff)
			((struct RigidDiskBlock *)(blocklist[i]))->rdb_BadBlockList += startblock;
			if (((struct RigidDiskBlock *)(blocklist[i]))->rdb_PartitionList != 0xffffffff)
			((struct RigidDiskBlock *)(blocklist[i]))->rdb_PartitionList += startblock;
			if (((struct RigidDiskBlock *)(blocklist[i]))->rdb_FileSysHeaderList != 0xffffffff)
			((struct RigidDiskBlock *)(blocklist[i]))->rdb_FileSysHeaderList += startblock;
			if (((struct RigidDiskBlock *)(blocklist[i]))->rdb_DriveInit != 0xffffffff)
			((struct RigidDiskBlock *)(blocklist[i]))->rdb_DriveInit += startblock;
			break;
			case IDNAME_FILESYSHEADER:
			if (((struct FileSysHeaderBlock *)(blocklist[i]))->fhb_SegListBlocks != 0xffffffff)
			((struct FileSysHeaderBlock *)(blocklist[i]))->fhb_SegListBlocks += startblock;
			case IDNAME_BADBLOCK:
			case IDNAME_PARTITION:
			case IDNAME_LOADSEG:
			if (((struct FileSysHeaderBlock *)(blocklist[i]))->fhb_Next != 0xffffffff)
			((struct FileSysHeaderBlock *)(blocklist[i]))->fhb_Next += startblock;
			break;
		}

		((struct RigidDiskBlock *)(blocklist[i]))->rdb_ChkSum = check_sum (blocklist[i]);
	}
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

BOOL write_rdb (struct iohandle *h,APTR *blocklist,long startblock,BOOL erase)

{
	long i;
	ULONG toblock = 0;

	if (erase && startblock)
	{
		UBYTE *buffer;
		if (buffer = AllocMem (h->blksize,MEMF_CLEAR))
		{
			while (toblock < startblock)
			{
				write_block (h,toblock,buffer);
				toblock ++;
			}
			FreeMem (buffer, h->blksize);
		}
	}
	else
	toblock = startblock;

	if (startblock)
	move_rdb (blocklist,startblock);

	for (i = 0; blocklist[i]; i++)
	{
		if (!write_block (h,toblock,blocklist[i]))
		{
			return (FALSE);
		}
		toblock ++;
	}


	return (TRUE);
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

void free_rdb (APTR *blocklist)

{
	long i;

	for (i = 0; i < MAXBLK; i++)
	if (blocklist[i])
	free_block (blocklist[i]);
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/

BOOL copy_rdb (struct iohandle *from,struct iohandle *to,long startblock,BOOL erase,BOOL force)

{
	APTR *blocklist;
	BOOL ok = FALSE;

	if (blocklist = AllocMem (MAXBLK * sizeof(APTR),MEMF_CLEAR)){
		if (read_rdb (from,blocklist) || force){
			ok = write_rdb (to,blocklist,startblock,erase);
		}
		free_rdb (blocklist);
		FreeMem (blocklist,MAXBLK * sizeof(APTR));
	}

	return (ok);
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
