/* -----------------------------------------------------------------------------

  <INSERT PROJECT DESCRIPTION HERE AND DELETE THIS LINE>

*/

/* /// "definitions" */
#define LOG_ENABLED
#define boolean short
#define true 1
#define false 0
/* <EDIT PROGRAM NAME AND VERSION AND DELETE THIS LINE> */

#define PROGRAMNAME     "xunarc"
#define VERSION         1
#define REVISION        0
#define VERSIONSTRING   "1.0"

/* define command line syntax and number of options */

#define RDARGS_TEMPLATE ""
#define RDARGS_OPTIONS  0

/* #define or #undef GENERATEWBMAIN to support workbench startup */

#define GENERATEWBMAIN

/* use classic libray syntax under OS4 */

#ifdef __amigaos4__
#define __USE_INLINE__
#endif

/* /// */
/* /// "includes" */

/* typical standard headers */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/* typical Amiga headers */

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

/* typical proto files */

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
#include "xad.h"
#include "gui.h"

/* /// */

/* /// "prototypes" */

int            main   (int argc, char **argv);
extern int            wbmain (struct WBStartup *wbs);
extern struct Config *Init   (void);
extern int            Main   (struct Config *config);
extern void           CleanUp(struct Config *config);
extern void           Log(char * str);

/* /// */
/* /// "structures" */

/* ---------------------------------- Config -----------------------------------

 Global data for this program

*/

struct Config
{
    struct RDArgs *RDArgs;
    boolean wb;
    LONG argc;
    struct WBStartup * wbStartup;
    char *src;
    char *dst;
    /* values of command line options (the result of ReadArgs())*/

#if RDARGS_OPTIONS
    LONG Options[RDARGS_OPTIONS];
#endif

    /* <INSERT YOUR GLOBAL DATA HERE AND DELETE THIS LINE> */
};

/* /// */
/* /// "globals" */
struct xadMasterBase *	xadMasterBase;
struct DosLibrary *	 DOSBase;
struct ExecBase *	 SysBase;
#ifdef LOG_ENABLED
BPTR output;
#endif
/* version tag */

#if defined(__SASC)
const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " "  __AMIGADATE__ "\n\0";
#elif defined(_DCC)
const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " (" __COMMODORE_DATE__ ")\n\0";
#else
const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " (" __DATE__ ")\n\0";
#endif

/* /// */
/* /// "entry points" */

/* ----------------------------------- main ------------------------------------

 Shell/wb entry point

*/

int
main(int argc, char **argv)
{
    for (int i = 0; i < argc; ++i) {
        printf("arg: %s\n",argv[i]);
    }

    int rc = 20;

    /* shell startup (indicated by argc != 0)? */

    if (argc)
    {
        struct Config *config;

        if (config = Init())
        {
            config->wb = false;
            config->src = argv[1];
            if (argc>2) {
                config->dst = argv[2];
            } else {
                config->dst = "ram:";
            }
#if RDARGS_OPTIONS

            /* parse startup options */

                if (config->RDArgs = ReadArgs(RDARGS_TEMPLATE, config->Options, NULL))
                    rc = Main(config);
                else
                    PrintFault(IoErr(), PROGRAMNAME);
#else

            rc = Main(config);

#endif

            CleanUp(config);
        }
    }
    else
        rc = wbmain((struct WBStartup *)argv);

    return(rc);
}

/* ---------------------------------- wbmain -----------------------------------

 Workbench entry point

*/

int
wbmain(struct WBStartup *wbs)
{
    int rc = 20;

#ifdef GENERATEWBMAIN

    struct Config *config;

    if (config = Init())
    {
        config->wb = true;
        config->wbStartup = wbs;
        config->argc = wbs->sm_NumArgs;
        rc = Main(config);

        CleanUp(config);
    }

#endif

    return(rc);
}

/* /// */
/* /// "initialize" */

/* ----------------------------------- Init ------------------------------------

 Initialize program. Allocate a config structure to store configuration data.

*/

struct Config *
Init()
{
    struct Config *config;

    if (config = (struct Config *)malloc(sizeof(struct Config)))
    {
        memset(config, 0, sizeof(struct Config));

        DOSBase = (struct DosLibrary *)OpenLibrary(DOSNAME, 37);
        if (DOSBase == 0) {
            exit(-100);
        } else {
#ifdef LOG_ENABLED
            output = Open("CON:0/0/640/200/xUnArc", MODE_NEWFILE);
#endif
        }
        if (!XadInit()) {
            puts("Can't opent xadmaster.library\n");
            exit(-101);
        }
        /* <INSERT YOUR INITIALIZATION CODE HERE AND DELETE THIS LINE> */
    }

    return(config);
}

/* /// */
/* /// "main" */

/* ----------------------------------- Main ------------------------------------

 Main program (return codes: 0=ok, 5=warn, 20=fatal)

*/

int
Main(struct Config *config)
{
    int rc = 0;
    /* <INSERT YOUR MAIN PROGRAM CODE HERE AND DELETE THIS LINE> */
#ifdef LOG_ENABLED
    Log("<<unarc log>>\n");
    Log(config->wb ? "wb   \n" : "shell\n");

    if (config->wb) {
        FPrintf(output,"Number of wb args %d\n", config->argc);
        for (int i = 0; i < config->wbStartup->sm_NumArgs; ++i) {
            //FPrintf(output, "  %d :%s\n", i, config->wbStartup->sm_ArgList[i].wa_Name);
        }
    }
    Log(config->src);
    Log(config->dst);
    Log("\n<<unarc log end>>");
#endif
    return(rc);
}

/* /// */
/* /// "cleanup" */


/* ---------------------------------- CleanUp ----------------------------------

 Free allocated resources

*/

void
CleanUp(struct Config *config)
{
    if (config)
    {
        /* <INSERT YOUR CLEAN-UP CODE HERE AND DELETE THIS LINE> */

        /* free command line options */

#if RDARGS_OPTIONS

        if (config->RDArgs)

                FreeArgs(config->RDArgs);
#endif

        free(config);
#ifdef LOG_ENABLED
        Flush(output);
        Delay(500L);
        Close(output);
#endif
    }
}

void Log(char * str) {
#ifdef LOG_ENABLED
    FPrintf(output, str);
#endif
}
/* /// */