#define LOG_ENABLED 1

#define PROGRAMNAME     "xunarc"
#define VERSION         0
#define REVISION        1
#define VERSIONSTRING   "0.1"

#define TEMPLATE "SRC/A,DST/A"

#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <dos/datetime.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <graphics/layers.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <workbench/icon.h>
#include <datatypes/pictureclass.h>
#include <libraries/asl.h>
#include <libraries/commodities.h>
#include <libraries/gadtools.h>
#include <libraries/iffparse.h>
#include <libraries/locale.h>
#include <rexx/rxslib.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <utility/hooks.h>
#include <proto/asl.h>
#include <proto/commodities.h>
#include <proto/xadmaster.h>
#include <proto/datatypes.h>
#include <proto/diskfont.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/iffparse.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/locale.h>
#include <proto/rexxsyslib.h>
#include <proto/utility.h>
#include <proto/wb.h>
#include <stdlib.h>
#include "xad.h"
#include "gui.h"
#include "common.h"

struct xadMasterBase *	xadMasterBase;
struct DosLibrary *	 DOSBase;
struct Library * IconBase;
struct ExecBase *	 SysBase;
extern struct Config * config;



const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " (" __DATE__ ")\n\0";

short init() {
    config = AllocMem(sizeof(struct Config), MEMF_ANY);
    DOSBase = (struct DosLibrary*) OpenLibrary(DOSNAME, 37);
    IconBase = OpenLibrary(ICONNAME, 37);
    if (DOSBase == NULL || IconBase == NULL) {
        return 0;
    }
    if ((xadMasterBase = (struct xadMasterBase *)OpenLibrary("xadmaster.library", 1))==NULL) {
        Log("Can't open xadmaster.library");
        return 0;
    }
    XadInit(config);
    return 1;
}

short cleanUp() {
#if LOG_ENABLED
    Flush(config->output);
    if (config->outputClosable) {
        Delay(500);
        Close(config->output);
    }
#endif
    FreeMem(config, sizeof (struct Config));
    if (DOSBase)
    CloseLibrary(DOSBase);
    if (IconBase)
    CloseLibrary(IconBase);
    if (xadMasterBase)
    CloseLibrary(xadMasterBase);
}


int main(int argc, char **argv) {
    char filename[512];
    LONG src,dst;
    LONG args[]={ (LONG)&src,
                  (LONG)&dst};


    struct RDArgs *rda=NULL;
    struct DiskObject *dob=NULL;
    BPTR out;
    if (!init()) {
        cleanUp();
        return 10;
    }
    config->noabs = 0;
    config->pattern = "#?";
    if (argc) {
        if (!(rda=ReadArgs(TEMPLATE, args, NULL)))
        {
            PrintFault(IoErr(), argv[0]);
            return 10;
        } else {
              config->output = Output();
              config->outputClosable = 0;
//            config->output = Open("CON:0/0/640/200/xUnArc", MODE_NEWFILE);
//            config->outputClosable = 1;
            config->src = (STRPTR)args[0];
            config->dst = (STRPTR)args[1];

        }
    } else {
        config->output = Open("CON:0/0/640/200/xUnArc", MODE_NEWFILE);
        config->outputClosable = 1;
        struct WBStartup *wbs=(struct WBStartup*)argv;
        struct WBArg *wba=&wbs->sm_ArgList[wbs->sm_NumArgs-1];
        BPTR oldcd;
        if (!(*wba->wa_Name)) return 10;
        //NameFromLock(wba->wa_Lock, filename, 512);
        oldcd=CurrentDir(wba->wa_Lock);
        if ((dob=GetDiskObjectNew(wba->wa_Name))) {
            GetCurrentDirName(filename, 512);
            AddPart(filename, (STRPTR)wba->wa_Name, 512);
            Flush(config->output);
            config->src = filename;
            if (!(config->dst=FindToolType(dob->do_ToolTypes, "DST"))) {
                config->dst = "ram:";
            }
        }
        CurrentDir(oldcd);
    }

    if (config->output) {
        FPrintf(config->output, "%s",  VersionTag+6);
        FPrintf(config->output, "SRC: %s\n",  (STRPTR)config->src);
        FPrintf(config->output, "DST: %s\n",  (STRPTR)config->dst);
    }
    XadProcess();
    if (rda) FreeArgs(rda);
    if (dob) FreeDiskObject(dob);
cleanUp();
//printf("podano opcje: SPACE=%d, PICTURE=%s, STATIC=%d, SIZE=%d\n", Odstep, nazwa_podkladu, Statik, SkalaIkon);
}