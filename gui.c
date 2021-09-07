//
// Created by jarokuczi on 26/08/2021.
//

#include "gui.h"
#include "common.h"
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/intuition_protos.h>
#include <clib/gadtools_protos.h>

struct DosLibrary *DOSBase;
struct ExecBase *SysBase;
struct Library *GadToolsBase;
struct Library *IntuitionBase;

struct TextAttr Topaz80 = { "topaz.font", 8, 0, 0, };
struct Task *task;

#define START_BUTTON    (4)
#define SRC_BUTTON      (5)
#define SRC_FIELD       (7)
#define DST_BUTTON      (8)
#define DST_FIELD       (9)
#define PROGRESS        (10)

char *const TASK_NAME = "extractTask";

VOID handle_windows_events(struct Window *);
VOID gadtoolsWindow(VOID);
VOID extractArchiveTask(VOID);
struct Gadget *progressBar;
struct Window    *mywin;

int GuiInit() {
    config->progress = 0;
    return 1;
}

int GuiShow() {
    gadtoolsWindow();
    return 1;
}

VOID gadtoolsWindow(VOID)
{
    struct Screen    *mysc;
    struct Gadget    *glist, *button;
    struct NewGadget ng;
    void             *vi;

    glist = NULL;

    if ( (mysc = LockPubScreen(NULL)) != NULL )
    {
        if ( (vi = GetVisualInfo(mysc, TAG_END)) != NULL )
        {
            /* GadTools gadgets require this step to be taken */
            progressBar = CreateContext(&glist);

            /* create a button gadget centered below the window title */
            ng.ng_TextAttr   = &Topaz80;
            ng.ng_VisualInfo = vi;
            ng.ng_LeftEdge   = 80;
            ng.ng_TopEdge    = 5 + mysc->WBorTop + (mysc->Font->ta_YSize + 1);
            ng.ng_Width      = 300;
            ng.ng_Height     = 12;
            ng.ng_GadgetText = "Progress";
            ng.ng_GadgetID   = PROGRESS;
            ng.ng_Flags      = 0;
            progressBar = CreateGadget(SLIDER_KIND, progressBar, &ng, GTSL_Max, 100, GTSL_Level, 0, GA_Disabled , 0 , TAG_END);

            /* create a button gadget centered below the window title */
            ng.ng_TextAttr   = &Topaz80;
            ng.ng_VisualInfo = vi;
            ng.ng_LeftEdge   = 150;
            ng.ng_TopEdge    = 20 + mysc->WBorTop + (mysc->Font->ta_YSize + 1);
            ng.ng_Width      = 100;
            ng.ng_Height     = 12;
            ng.ng_GadgetText = "Click Here";
            ng.ng_GadgetID   = START_BUTTON;
            ng.ng_Flags      = 0;
            button = CreateGadget(BUTTON_KIND, progressBar, &ng, TAG_END);



            if (button != NULL && progressBar != NULL)
            {
                if ( (mywin = OpenWindowTags(NULL,
                                             WA_Title,     "xUnArc",
                                             WA_Gadgets,   glist,      WA_AutoAdjust,    TRUE,
                                             WA_Width,       400,      WA_InnerHeight,    100,
                                             WA_DragBar,    TRUE,      WA_DepthGadget,   TRUE,
                                             WA_Activate,   TRUE,      WA_CloseGadget,   TRUE,
                                             WA_IDCMP, IDCMP_CLOSEWINDOW |
                                                       IDCMP_REFRESHWINDOW | BUTTONIDCMP,
                                             WA_PubScreen,   mysc,
                                             TAG_END)) != NULL )
                {
                    GT_RefreshWindow(mywin, NULL);

                    handle_windows_events(mywin);

                    CloseWindow(mywin);
                }
            }
            /* FreeGadgets() must be called after the context has been
            ** created.  It does nothing if glist is NULL
            */
            FreeGadgets(glist);
            FreeVisualInfo(vi);
        }
        UnlockPubScreen(NULL, mysc);
    }
}

VOID handle_windows_events(struct Window *mywin)
{
    struct IntuiMessage *imsg;
    struct Gadget *gad;
    BOOL  terminated = FALSE;

    while (!terminated)
    {
        Wait (1 << mywin->UserPort->mp_SigBit);

        /* Use GT_GetIMsg() and GT_ReplyIMsg() for handling */
        /* IntuiMessages with GadTools gadgets.             */
        while ((!terminated) && (imsg = GT_GetIMsg(mywin->UserPort)))
        {
            /* GT_ReplyIMsg() at end of loop */

            switch (imsg->Class)
            {
                case IDCMP_GADGETUP:       /* Buttons only report GADGETUP */
                    gad = (struct Gadget *)imsg->IAddress;
                    if (gad->GadgetID == START_BUTTON) {
                        FPrintf(config->output,"Button was pressed.\n");
                        task = CreateTask(TASK_NAME, 0, extractArchiveTask, 16000);
                    }
                    break;
                case IDCMP_CLOSEWINDOW:
                    terminated = TRUE;
                    break;
                case IDCMP_REFRESHWINDOW:
                    /* This handling is REQUIRED with GadTools. */
                    GT_BeginRefresh(mywin);
                    GT_EndRefresh(mywin, TRUE);
                    break;
            }
            /* Use the toolkit message-replying function here... */
            GT_ReplyIMsg(imsg);
        }
    }
}


VOID extractArchiveTask(VOID) {
    while (config->progress < 100) {
        FPrintf(config->output, "Progress %ld\n", config->progress);
        config->progress++;
        GT_SetGadgetAttrs(progressBar, mywin, NULL, GTSL_Max, 100 - config->progress, TAG_END);
        Delay(1);
    }
}