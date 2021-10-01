//
// Created by jarokuczi on 01/10/2021.
//
#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define SIG_HANDSHAKE 31337L

struct Conf {
    STRPTR src;
    STRPTR dst;
};
struct DosLibrary *	 DOSBase;
struct ExecBase *	 SysBase;

void theProcess();
void theProcessExited();

int processRunning = 1;

int main(int argc, char **argv) {
    struct Conf * config = AllocMem(sizeof (struct Conf), MEMF_ANY);
    struct Process *subProcess;
    DOSBase = (struct DosLibrary*) OpenLibrary(DOSNAME, 37);
    if (argc < 3) {
        Printf("Program requires two arguments\n");
        return 10;
    }
    if (DOSBase == NULL) {
        return 1000;
    }
    config->src = argv[1];
    config->dst = argv[2];
    Printf("Started App\nsrc: %s\ndst: %s\n", config->src, config->dst);
    subProcess = CreateNewProcTags(NP_Name, "the process", NP_Entry, (APTR)theProcess, NP_Output, Output(),
                      NP_ExitCode, (APTR)theProcessExited, NP_CloseOutput, FALSE, TAG_END);

    Delay(100);
    if (subProcess) {
        //we are passing a config as a task user data
        subProcess->pr_Task.tc_UserData = config;
        Printf("Signalling task...\n");
        //now we are signalling our tast that the data is ready
        Signal(&subProcess->pr_Task, SIG_HANDSHAKE);
    } else {
        Printf("Can't create the process, exiting.\n");
    }
    while (subProcess && processRunning) {
        Printf("Waiting...\n");
        Delay(100);
    }
    CloseLibrary((struct Library *)DOSBase);
    FreeMem(config, sizeof (struct Conf));
}


void theProcess() {
    struct Process *self;
    struct Conf *conf;
    self = FindTask(NULL);
    //waiting for the signal from the parent process which indicates data is presend in tc_UserData
    Wait(SIG_HANDSHAKE);
    //reading the data
    conf = self->pr_Task.tc_UserData;
    DOSBase = (struct DosLibrary*) OpenLibrary(DOSNAME, 37);
    if (DOSBase == NULL) {
        return;
    }
    Printf("Process Started \nsrc: %s\ndst: %s\n", conf->src,conf->dst);
    for (int i = 0; i < 20; ++i) {
        Delay(10);
        Printf("Count %ld\n", i);
    }
    CloseLibrary((struct Library*)DOSBase);
}

void theProcessExited() {
    processRunning = 0;
}