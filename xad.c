//
// Created by jarokuczi on 26/08/2021.
//
#include <proto/xadmaster.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <dos/dosasl.h>
#include <utility/hooks.h>

#include <inline/dos.h>
#include <proto/exec.h>
#include <inline/stubs.h>
#include <memory.h>
#include <clib/dos_protos.h>
#include "xad.h"
#include "common.h"

struct xadMasterBase *xadMasterBase;
struct DosLibrary *DOSBase;
struct ExecBase *SysBase;

struct Config *config;

ULONG unpack();

int XadInit(struct Config *cnf) {
    config = cnf;
    return 1;
}

int XadProcess() {
    ULONG ret = unpack();
    FPrintf(config->output, "Unpack finished with %d code\n", ret);
    return 1;
}


#define MINPRINTSIZE    512    /* 0.5KB */
#define NAMEBUFSIZE    512
#define PATBUFSIZE    (NAMEBUFSIZE*2+10)

struct xHookArgs {
    STRPTR name;
    ULONG extractmode;
    ULONG flags;
    ULONG finish;
    ULONG lastprint;
};

ULONG progrhook(struct Hook *hook, void *o, struct xadProgressInfo *pi);

LONG CheckNameSize(STRPTR name, ULONG size);

LONG CheckName(STRPTR *pat, STRPTR name);

ULONG unpack() {
    ULONG ret = RETURN_FAIL, numerr = 0;

    LONG err = 0;
    int totalFiles = 0;
    int currentFileNumber = 1;

    FPrintf(config->output, "Extracting %s into %s\n", config->src, config->dst);
    Flush(config->output);
    LONG namesize = 0;
    struct Hook prhook;
    UBYTE filename[NAMEBUFSIZE];
    struct xHookArgs xh;

    xh.name = filename;
    xh.flags = xh.finish = xh.lastprint = xh.extractmode = 0;

    /* Note! The hook may change the filename!!! */
    memset(&prhook, 0, sizeof(struct Hook));
    prhook.h_Entry = (ULONG (*)()) progrhook;
    prhook.h_Data = &xh;


    namesize = 30;

    if (config->dst) {
        ULONG numfile = 0, numdir = 0;
        struct xadDeviceInfo *dvi = 0;
        struct xadArchiveInfo *ai;
        struct TagItem ti[5];
        struct TagItem ti2[5];
        LONG loop = 2;
        ti[1].ti_Tag = XAD_NOEXTERN;
        ti[1].ti_Data = 0;
        ti[2].ti_Tag = TAG_IGNORE;
        ti[2].ti_Data = 0;
        ti[3].ti_Tag = TAG_IGNORE;
        ti[3].ti_Data = 1;
        ti[4].ti_Tag = TAG_DONE;

        ti2[1].ti_Tag = XAD_NOEMPTYERROR;
        ti2[1].ti_Data = TRUE;
        ti2[2].ti_Tag = XAD_PROGRESSHOOK;
        ti2[2].ti_Data = (ULONG) &prhook;
        ti2[3].ti_Tag = TAG_DONE;
        ti2[4].ti_Tag = TAG_DONE; /* needed later for loop */

        if ((ai = (struct xadArchiveInfo *)
                xadAllocObjectA(XADOBJ_ARCHIVEINFO, 0))) {

            err = xadGetInfo(ai, XAD_INFILENAME, config->src,
                             XAD_NOEXTERN, 0, TAG_IGNORE, 0, XAD_PROGRESSHOOK, &prhook, TAG_DONE);
            --loop;
            while (!err && loop) {
                if (ai->xai_Flags & XADAIF_FILECORRUPT)
                    FPrintf(config->output, "!!! The archive file has some corrupt data. !!!\n");
                struct xadFileInfo *fi;

                ret = 0;
                fi = ai->xai_FileInfo;


                while (fi) {
                    totalFiles++;
                    fi = fi->xfi_Next;
                }
                FPrintf(config->output, "Total files in archive: %d\n",  totalFiles);
                Flush(config->output);
                fi = ai->xai_FileInfo;

                while (fi && !(SetSignal(0L, 0L) & SIGBREAKF_CTRL_C) && !xh.finish) {
                    FPrintf(config->output, "Processing file %s #%d\n",
                            fi->xfi_FileName, currentFileNumber++);
                    Flush(config->output);
                    if (!config->pattern || CheckName(config->pattern, fi->xfi_FileName)) {
                        CopyMem(config->dst, filename, strlen(config->dst) + 1);
                        if (strcmp(config->dst, "NIL:") && strcmp(config->dst, "nil:")) {
                            if (!config->noabs)
                                AddPart(filename, fi->xfi_FileName, NAMEBUFSIZE);
                            else {
                                STRPTR fname = filename, f;

                                if (*config->dst) {
                                    fname += strlen(config->dst) - 1;
                                    if (*fname != ':' && *fname != '/')
                                        *(++fname) = '/';
                                    ++fname;
                                }
                                for (f = fi->xfi_FileName; *f == '/' || *f == ':'; ++f);
                                for (; *f; ++f)
                                    *(fname++) = *f == ':' ? '/' : *f;
                                *fname = 0;
                            }
                        }
                        if (fi->xfi_Flags & XADFIF_LINK) {
                            Printf("Skipped Link\n");
                        } else if (fi->xfi_Flags & XADFIF_DIRECTORY) {
                            BPTR a;
                            LONG err = 0, i = 0;
                            UBYTE r;
                            ++numdir;
                            while (filename[i] && !err) {
                                for (; filename[i] && filename[i] != '/'; ++i);
                                r = filename[i];
                                filename[i] = 0;
                                if ((a = Lock(filename, SHARED_LOCK)))
                                    UnLock(a);
                                else if ((a = CreateDir(filename)))
                                    UnLock(a);
                                else
                                    err = 1;
                                filename[i++] = r;
                            }
                            if (err) {
                                Printf("failed to create directory '%s'\n", fi->xfi_FileName);
                                ++numerr;
                            } else {
                                FPrintf(config->output, "Created directory   : %s\n", filename);
                                Flush(config->output);
                            }
                            if (!err) {
                                struct DateStamp d;
                                SetFileDate(filename, &d);
                                SetProtection(filename, fi->xfi_Protection);
                                SetComment(filename, fi->xfi_Comment);
                                /* SetOwner ??? */
                            }
                        } else {
                            struct DateStamp d;

                            if (namesize) {
                                xh.finish = CheckNameSize(FilePart(filename), namesize);
                            }

                            if (!xh.finish) {
                                LONG e;

                                ++numfile;

                                xh.extractmode = 1;
                                e = xadFileUnArc(ai, XAD_OUTFILENAME, filename,
                                                 XAD_ENTRYNUMBER, fi->xfi_EntryNumber, XAD_MAKEDIRECTORY,
                                                 1, XAD_OVERWRITE, 1,
                                                 XAD_NOKILLPARTIAL, 0, XAD_PROGRESSHOOK, &prhook, TAG_DONE);
                                xh.extractmode = 0;

                                if (!e) {
                                    SetFileDate(filename, &d);
                                    SetProtection(filename, fi->xfi_Protection);
                                    SetComment(filename, fi->xfi_Comment);
                                } else
                                    ++numerr;
                                /* IO-errors, abort */
                                if (e == XADERR_INPUT || e == XADERR_OUTPUT)
                                    xh.finish = 1;
                            }
                        }
                    }
                    fi = fi->xfi_Next;
                }
                ti2[3].ti_Tag = XAD_STARTCLIENT;
                ti2[3].ti_Data = (ULONG) ai->xai_Client->xc_Next;
                xadFreeInfo(ai);
                if (--loop) {
                    loop = 0;
                    if (ti2[3].ti_Data) {
                        xadFreeObjectA(ai, 0); /* realloc ai structure */
                        if ((ai = (struct xadArchiveInfo *) xadAllocObjectA(XADOBJ_ARCHIVEINFO, 0))) {
                            if (!xadGetDiskInfoA(ai, ti2))
                                loop = 2;
                        }
                    }
                }
                if (!loop && !(SetSignal(0L, 0L) & SIGBREAKF_CTRL_C)) {
                    FPrintf(config->output, "Processed");
                    if (numfile)
                        FPrintf(config->output, " %ld file%s%s", numfile, numfile == 1 ? "" : "s",
                                numdir ? " and" : "");
                    if (numdir)
                        FPrintf(config->output, " %ld director%s", numdir, numdir == 1 ? "y" : "ies");
                    if (!numfile && !numdir)
                        FPrintf(config->output, " nothing");
                    if (numerr)
                        FPrintf(config->output, ", %ld error%s", numerr, numerr == 1 ? "" : "s");
                    FPrintf(config->output, ".\n");
                }
            } /* xadGetInfo, loop */

            if (ai)
                xadFreeObjectA(ai, 0);
            if (dvi)
                xadFreeObjectA(dvi, 0);
        } /* xadAllocObject */
    } else
        SetIoErr(ERROR_REQUIRED_ARG_MISSING);
    if (SetSignal(0L, 0L) & SIGBREAKF_CTRL_C)
        SetIoErr(ERROR_BREAK);
    if (!ret && numerr)
        ret = RETURN_ERROR;
    return ret;
}

/* Because of SAS-err, this cannot be SAVEDS */
ULONG progrhook(struct Hook *hook, void *o, struct xadProgressInfo *pi) {
    ULONG ret = 0;
    STRPTR name = ((struct xHookArgs *) (hook->h_Data))->name;
    if (pi->xpi_Mode == XADPMODE_PROGRESS) {
        if (pi->xpi_FileInfo->xfi_Flags & XADFIF_NOUNCRUNCHSIZE)
            FPrintf(config->output, "Wrote %8lu bytes: %s          \r", pi->xpi_CurrentSize, name);
        else
            FPrintf(config->output, "Wrote %8lu of %8lu bytes: %s          \r",
                    pi->xpi_CurrentSize, pi->xpi_FileInfo->xfi_Size, name);
        ((struct xHookArgs *) (hook->h_Data))->lastprint = pi->xpi_CurrentSize;
    }
    if (!(SetSignal(0L, 0L) & SIGBREAKF_CTRL_C)) /* clear ok flag */
        ret |= XADPIF_OK;
    return ret;
}


LONG CheckNameSize(STRPTR name, ULONG size) {
    LONG ret = 0;
    LONG r;

    if ((r = strlen(name)) > size) {
        UBYTE buf[NAMEBUFSIZE];

        FPrintf(config->output,
                "\r\033[KFilename '%s' exceeds name limit of %ld by %ld, rename? (Y|\033[1mN\033[0m|Q): ", name, size,
                r - size);

        Flush(config->output);
        SetMode(Input(), TRUE);
        r = FGetC(Input());
        SetMode(Input(), FALSE);
        switch (r) {
            case 'q':
            case 'Q':
                ret = 1;
                break;
            case 'y':
            case 'Y':
                Printf("\r\033[KEnter new name for '%s':", name);
                Flush(Output());
                FGets(Input(), buf, NAMEBUFSIZE - 1); /* 1 byte less to correct bug before V39 */
                r = strlen(buf);
                if (buf[r - 1] == '\n') /* skip return character */
                    buf[--r] = 0;
                Printf("\033[1F\033[K"); /* go up one line and clear it */
                if (!(ret = CheckNameSize(buf, size))) {
                    for (r = 0; buf[r]; ++r)
                        *(name++) = buf[r];
                    *name = 0;
                }
                break;
        }
    }
    return ret;
}

/* would be better to store the pattern parse stuff and do it only once,
but so it is a lot easier */
LONG CheckName(STRPTR *pat, STRPTR name) {
    UBYTE buf[PATBUFSIZE];
    while (*pat) {
        if (ParsePatternNoCase(*(pat++), buf, PATBUFSIZE) >= 0) {
            if (MatchPatternNoCase(buf, name))
                return 1;
        } /* A scan failure means no recognition, should be an error print here */
    }
    return 0;
}


