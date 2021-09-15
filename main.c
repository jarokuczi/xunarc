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
#include <intuition/intuitionbase.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <workbench/icon.h>
#include <datatypes/pictureclass.h>
#include <libraries/asl.h>
#include <libraries/commodities.h>
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
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/iffparse.h>
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
struct Library *GadToolsBase;
struct Library *IntuitionBase;

extern struct Config * config;


void openNewCon();

const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " (" __DATE__ ")\n\0";

short init(int shell) {
    config = AllocMem(sizeof(struct Config), MEMF_ANY|MEMF_CLEAR);
    config->output = NULL;
    config->quiet = 1;
    if (shell) {
        //openNewCon();
        config->output = Output();
        config->outputClosable = 0;
        config->quiet = 0;
    }
    DOSBase = (struct DosLibrary*) OpenLibrary(DOSNAME, 37);
    IconBase = OpenLibrary(ICONNAME, 37);
    IntuitionBase = OpenLibrary("intuition.library", 37);
    GadToolsBase = OpenLibrary("gadtools.library", 37);
    if (DOSBase == NULL || IconBase == NULL || IntuitionBase == NULL || GadToolsBase == NULL) {
        return 0;
    }
    if ((xadMasterBase = (struct xadMasterBase *)OpenLibrary("xadmaster.library", 1))==NULL) {
        openNewCon();
        FPrintf(config->output, "Can't open xadmaster.library");
        return 0;
    }

    XadInit(config);
    return 1;
}

void openNewCon() {
    if (!config->output) {
        config->output = Open("CON:0/0/640/200/xUnArc", MODE_NEWFILE);
        config->outputClosable = 1;
        config->quiet = 0;
    }
}

short cleanUp() {
    Flush(config->output);
    if (config->outputClosable) {
        Delay(500);
        Close(config->output);
    }
    FreeMem(config, sizeof (struct Config));
    if (DOSBase) {
        CloseLibrary(DOSBase);
    }
    if (IconBase) {
        CloseLibrary(IconBase);
    }
    if (IntuitionBase) {
        CloseLibrary(IntuitionBase);
    }
    if (GadToolsBase) {
        CloseLibrary(GadToolsBase);
    }
    if (xadMasterBase) {
        CloseLibrary(xadMasterBase);
    }
}


int main(int argc, char **argv) {
    uint nogui = 0;
    char filename[512];
    LONG src,dst;
    LONG args[]={ (LONG)&src,
                  (LONG)&dst};


    struct RDArgs *rda=NULL;
    struct DiskObject *dob=NULL;
    BPTR out;

    if (!init(argc)) {
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
            config->src = (STRPTR)args[0];
            config->dst = (STRPTR)args[1];
        }
    } else {
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
            if ((FindToolType(dob->do_ToolTypes, "VERBOSE"))) {
                openNewCon();
            }
            if ((FindToolType(dob->do_ToolTypes, "NOGUI"))) {
                nogui = 1;
                openNewCon();
            }
            if (!(config->dst=FindToolType(dob->do_ToolTypes, "DST"))) {
                config->dst = "ram:";
            }
        }
        CurrentDir(oldcd);
    }

    if (config->output && !config->quiet) {
        FPrintf(config->output, "%s",  VersionTag+6);
        FPrintf(config->output, "SRC: %s\n",  (STRPTR)config->src);
        FPrintf(config->output, "DST: %s\n",  (STRPTR)config->dst);
    }
    if (argc || nogui) {
        XadProcess();
    } else {
        GuiInit();
        GuiShow();
    }
    if (rda) FreeArgs(rda);
    if (dob) FreeDiskObject(dob);
    cleanUp();
}