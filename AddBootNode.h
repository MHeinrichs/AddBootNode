#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <stdlib.h>
#include <stdio.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>
#include <libraries/configvars.h>
#include <libraries/configregs.h>
#include <dos/filehandler.h>
#include <resources/filesysres.h>
#include <clib/exec_protos.h>
#include <clib/expansion_protos.h>
#include "rdblib.h"

#define VERSION_STRING "2.5"
struct Library *ExpansionBase=NULL;
int writeMountlist(APTR *blocklist, int isKick13, char* device, int devicenum, char* fileName);

int AddBootNodes(char* device, int device, char* outfile, long test, long force);
UWORD printDeviceInfo(APTR* unit);
UWORD printOldDeviceInfo(APTR* unit);
UWORD GetDriveInfo(char* device, int devicenum, int verbose);

