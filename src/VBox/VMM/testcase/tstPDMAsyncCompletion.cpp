/* $Id: tstPDMAsyncCompletion.cpp 20429 2009-06-09 11:45:08Z vboxsync $ */
/** @file
 * PDM Asynchronous Completion Testcase.
 *
 * This testcase is for testing the async completion interface.
 * It implements a file copy program which uses the interface to copy the data.
 *
 * Use: ./tstPDMAsyncCompletion <source> <destination>
 */

/*
 * Copyright (C) 2008-2009 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_PDM_ASYNC_COMPLETION

#include "../VMInternal.h" /* UVM */
#include <VBox/vm.h>
#include <VBox/uvm.h>
#include <VBox/pdmasynccompletion.h>
#include <VBox/vmm.h>
#include <VBox/cpum.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/pdmapi.h>
#include <iprt/alloc.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/file.h>
#include <iprt/initterm.h>
#include <iprt/semaphore.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/thread.h>

#define TESTCASE "tstPDMAsyncCompletion"

/*
 * Number of simultaneous active tasks.
 */
#define NR_TASKS      80
#define BUFFER_SIZE 4096 /* 4K */

/* Buffers to store data in .*/
uint8_t                *g_AsyncCompletionTasksBuffer[NR_TASKS];
PPDMASYNCCOMPLETIONTASK g_AsyncCompletionTasks[NR_TASKS];
volatile uint32_t       g_cTasksLeft;
RTSEMEVENT              g_FinishedEventSem;

void pfnAsyncTaskCompleted(PVM pVM, void *pvUser, void *pvUser2)
{
    LogFlow((TESTCASE ": %s: pVM=%p pvUser=%p pvUser2=%p\n", __FUNCTION__, pVM, pvUser, pvUser2));

    uint32_t cTasksStillLeft = ASMAtomicDecU32(&g_cTasksLeft);

    if (!cTasksStillLeft)
    {
        /* All tasks processed. Wakeup main. */
        RTSemEventSignal(g_FinishedEventSem);
    }
}

int main(int argc, char *argv[])
{
    int rcRet = 0; /* error count */
    int rc = VINF_SUCCESS;
    PPDMASYNCCOMPLETIONENDPOINT pEndpointSrc, pEndpointDst;

    RTR3InitAndSUPLib();

    if (argc != 3)
    {
        RTPrintf(TESTCASE ": Usage is ./tstPDMAsyncCompletion <source> <dest>\n");
        return 1;
    }

    PVM pVM;
    rc = VMR3Create(1, NULL, NULL, NULL, NULL, &pVM);
    if (RT_SUCCESS(rc))
    {
        PPDMASYNCCOMPLETIONTEMPLATE pTemplate;

        /*
         * Little hack to avoid the VM_ASSERT_EMT assertion.
         */
        RTTlsSet(pVM->pUVM->vm.s.idxTLS, &pVM->pUVM->aCpus[0]);
        pVM->pUVM->aCpus[0].pUVM = pVM->pUVM;
        pVM->pUVM->aCpus[0].vm.s.NativeThreadEMT = RTThreadNativeSelf();

        /*
         * Create the template.
         */
        rc = PDMR3AsyncCompletionTemplateCreateInternal(pVM, &pTemplate, pfnAsyncTaskCompleted, NULL, "Test");
        if (RT_FAILURE(rc))
        {
            RTPrintf(TESTCASE ": Error while creating the template!! rc=%d\n", rc);
            return 1;
        }

        /*
         * Create event semaphor.
         */
        rc = RTSemEventCreate(&g_FinishedEventSem);
        AssertRC(rc);

        /*
         * Create the temporary buffers.
         */
        int i;

        for (i=0; i < NR_TASKS; i++)
        {
            g_AsyncCompletionTasksBuffer[i] = (uint8_t *)RTMemAllocZ(BUFFER_SIZE);
            if (!g_AsyncCompletionTasksBuffer[i])
            {
                RTPrintf(TESTCASE ": out of memory!\n");
                rcRet++;
                return rcRet;
            }
        }

        /* Create the destination as the async completion API can't do this. */
        RTFILE FileTmp;
        rc = RTFileOpen(&FileTmp, argv[2], RTFILE_O_READWRITE | RTFILE_O_OPEN_CREATE);
        if (RT_FAILURE(rc))
        {
            RTPrintf(TESTCASE ": Error while creating the destination!! rc=%d\n", rc);
            return 1;
            rcRet++;
            return rcRet;
        }
        RTFileClose(FileTmp);

        /* Create our file endpoint */
        rc = PDMR3AsyncCompletionEpCreateForFile(&pEndpointSrc, argv[1], 0, pTemplate);
        if (RT_SUCCESS(rc))
        {
            rc = PDMR3AsyncCompletionEpCreateForFile(&pEndpointDst, argv[2], 0, pTemplate);
            if (RT_SUCCESS(rc))
            {
                PDMR3PowerOn(pVM);

                /* Wait for all threads to finish initialization. */
                RTThreadSleep(100);

                int fReadPass = true;
                uint64_t cbSrc, cbLeft;
                size_t   offSrc = 0;
                size_t   offDst = 0;
                uint32_t cTasksUsed = 0;

                rc = PDMR3AsyncCompletionEpGetSize(pEndpointSrc, &cbSrc);
                if (RT_SUCCESS(rc))
                {
                    /* Copy the data. */
                    for (;;)
                    {
                        if (fReadPass)
                        {
                            cTasksUsed = (BUFFER_SIZE * NR_TASKS) <= (cbSrc - offSrc)
                                         ? NR_TASKS
                                         :   ((cbSrc - offSrc) / BUFFER_SIZE)
                                           + ((cbSrc - offSrc) % BUFFER_SIZE) > 0
                                         ? 1
                                         : 0;

                            g_cTasksLeft = cTasksUsed;

                            for (uint32_t i = 0; i < cTasksUsed; i++)
                            {
                                size_t cbRead = ((size_t)offSrc + BUFFER_SIZE) <= cbSrc ? BUFFER_SIZE : cbSrc - offSrc;
                                PDMDATASEG DataSeg;

                                DataSeg.pvSeg = g_AsyncCompletionTasksBuffer[i];
                                DataSeg.cbSeg = cbRead;

                                rc = PDMR3AsyncCompletionEpRead(pEndpointSrc, offSrc, &DataSeg, 1, cbRead, NULL,
                                                                &g_AsyncCompletionTasks[i]);
                                AssertRC(rc);
                                offSrc += cbRead;
                                if (offSrc == cbSrc)
                                    break;
                            }
                        }
                        else
                        {
                            g_cTasksLeft = cTasksUsed;

                            for (uint32_t i = 0; i < cTasksUsed; i++)
                            {
                                size_t cbWrite = (offDst + BUFFER_SIZE) <= cbSrc ? BUFFER_SIZE : cbSrc - offDst;
                                PDMDATASEG DataSeg;

                                DataSeg.pvSeg = g_AsyncCompletionTasksBuffer[i];
                                DataSeg.cbSeg = cbWrite;

                                rc = PDMR3AsyncCompletionEpWrite(pEndpointDst, offDst, &DataSeg, 1, cbWrite, NULL,
                                                                 &g_AsyncCompletionTasks[i]);
                                AssertRC(rc);
                                offDst += cbWrite;
                                if (offDst == cbSrc)
                                    break;
                            }
                        }

                        rc = RTSemEventWait(g_FinishedEventSem, RT_INDEFINITE_WAIT);
                        AssertRC(rc);

                        if (!fReadPass && (offDst == cbSrc))
                            break;
                        else if (fReadPass)
                            fReadPass = false;
                        else
                        {
                            cTasksUsed = 0;
                            fReadPass = true;
                        }
                    }
                }
                else
                {
                    RTPrintf(TESTCASE ": Error querying size of the endpoint!! rc=%d\n", rc);
                    rcRet++;
                }

                PDMR3PowerOff(pVM);
                PDMR3AsyncCompletionEpClose(pEndpointDst);
            }
            PDMR3AsyncCompletionEpClose(pEndpointSrc);
        }

        rc = VMR3Destroy(pVM);
        AssertMsg(rc == VINF_SUCCESS, ("%s: Destroying VM failed rc=%Rrc!!\n", __FUNCTION__, rc));

        /*
         * Clean up.
         */
        for (uint32_t i = 0; i < NR_TASKS; i++)
        {
            RTMemFree(g_AsyncCompletionTasksBuffer[i]);
        }
    }
    else
    {
        RTPrintf(TESTCASE ": failed to create VM!! rc=%Rrc\n", rc);
        rcRet++;
    }

    return rcRet;
}
