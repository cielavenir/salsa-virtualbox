/* $Id: message.cpp 28800 2010-04-27 08:22:32Z vboxsync $ */
/** @file
 * IPRT - Error reporting to standard error.
 */

/*
 * Copyright (C) 2009 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "internal/iprt.h"
#include <iprt/message.h>

#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/stream.h>
#include "internal/process.h"


static int rtMsgWorker(PRTSTREAM pDst, const char *pszPrefix, const char *pszFormat, va_list va)
{
    if (   !*pszFormat
        || !strcmp(pszFormat, "\n"))
        RTStrmPrintf(pDst, "\n");
    else
    {
        char *pszMsg;
        ssize_t cch = RTStrAPrintfV(&pszMsg, pszFormat, va);
        if (cch >= 0)
        {
            /* print it line by line. */
            char *psz = pszMsg;
            do
            {
                char *pszEnd = strchr(psz, '\n');
                if (!pszEnd)
                {
                    RTStrmPrintf(pDst, "%s: %s%s\n", &g_szrtProcExePath[g_offrtProcName], pszPrefix, psz);
                    break;
                }
                if (pszEnd == psz)
                    RTStrmPrintf(pDst, "\n");
                else
                {
                    *pszEnd = '\0';
                    RTStrmPrintf(pDst, "%s: %s%s\n", &g_szrtProcExePath[g_offrtProcName], pszPrefix, psz);
                }
                psz = pszEnd + 1;
            } while (*psz);
            RTStrFree(pszMsg);
        }
        else
        {
            /* Simple fallback for handling out-of-memory conditions. */
            RTStrmPrintf(pDst, "%s: %s", &g_szrtProcExePath[g_offrtProcName], pszPrefix);
            RTStrmPrintfV(pDst, pszFormat, va);
            if (!strchr(pszFormat, '\n'))
                RTStrmPrintf(pDst, "\n");
        }
    }

    return VINF_SUCCESS;
}

RTDECL(int)  RTMsgError(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    int rc = RTMsgErrorV(pszFormat, va);
    va_end(va);
    return rc;
}
RT_EXPORT_SYMBOL(RTMsgError);


RTDECL(int)  RTMsgErrorV(const char *pszFormat, va_list va)
{
    return rtMsgWorker(g_pStdErr, "error: ", pszFormat, va);
}
RT_EXPORT_SYMBOL(RTMsgErrorV);


RTDECL(RTEXITCODE) RTMsgErrorExit(RTEXITCODE enmExitCode, const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    RTMsgErrorV(pszFormat, va);
    va_end(va);
    return enmExitCode;
}
RT_EXPORT_SYMBOL(RTMsgErrorExitV);


RTDECL(RTEXITCODE) RTMsgErrorExitV(RTEXITCODE enmExitCode, const char *pszFormat, va_list va)
{
    RTMsgErrorV(pszFormat, va);
    return enmExitCode;
}
RT_EXPORT_SYMBOL(RTMsgErrorExitV);


RTDECL(int) RTMsgErrorRc(int rcRet, const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    RTMsgErrorV(pszFormat, va);
    va_end(va);
    return rcRet;
}
RT_EXPORT_SYMBOL(RTMsgErrorRcV);


RTDECL(int) RTMsgErrorRcV(int rcRet, const char *pszFormat, va_list va)
{
    RTMsgErrorV(pszFormat, va);
    return rcRet;
}
RT_EXPORT_SYMBOL(RTMsgErrorRcV);


RTDECL(RTEXITCODE) RTMsgInitFailure(int rcRTR3Init)
{
    if (   g_offrtProcName
        && g_offrtProcName < sizeof(g_szrtProcExePath)
        && g_szrtProcExePath[0]
        && g_szrtProcExePath[g_offrtProcName])
        RTStrmPrintf(g_pStdErr, "%s: fatal error: RTR3Init: %Rrc\n", &g_szrtProcExePath[g_offrtProcName], rcRTR3Init);
    else
        RTStrmPrintf(g_pStdErr, "fatal error: RTR3Init: %Rrc\n", rcRTR3Init);
    return RTEXITCODE_INIT;
}
RT_EXPORT_SYMBOL(RTMsgInitFailure);


RTDECL(int)  RTMsgWarning(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    int rc = RTMsgWarningV(pszFormat, va);
    va_end(va);
    return rc;
}
RT_EXPORT_SYMBOL(RTMsgInfo);


RTDECL(int)  RTMsgWarningV(const char *pszFormat, va_list va)
{
    return rtMsgWorker(g_pStdErr, "warning: ", pszFormat, va);
}
RT_EXPORT_SYMBOL(RTMsgWarningV);


RTDECL(int)  RTMsgInfo(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    int rc = RTMsgInfoV(pszFormat, va);
    va_end(va);
    return rc;
}
RT_EXPORT_SYMBOL(RTMsgInfo);


RTDECL(int)  RTMsgInfoV(const char *pszFormat, va_list va)
{
    return rtMsgWorker(g_pStdOut, "info: ", pszFormat, va);
}
RT_EXPORT_SYMBOL(RTMsgInfoV);
