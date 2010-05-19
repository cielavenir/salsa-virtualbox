/* $Id: VBoxGuestR3LibCredentials.cpp 28800 2010-04-27 08:22:32Z vboxsync $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, user credentials.
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
#include <iprt/asm.h>
#include <iprt/string.h>
#include <iprt/rand.h>
#include <VBox/log.h>

#include "VBGLR3Internal.h"


/**
 * Checks whether user credentials are available to the guest or not.
 *
 * @returns IPRT status value; VINF_SUCCESS if credentials are available,
 *          VERR_NOT_FOUND if not. Otherwise an error is occured.
 */
VBGLR3DECL(int) VbglR3CredentialsQueryAvailability(void)
{
    VMMDevCredentials Req;
    RT_ZERO(Req);
    vmmdevInitRequest((VMMDevRequestHeader*)&Req, VMMDevReq_QueryCredentials);
    Req.u32Flags |= VMMDEV_CREDENTIALS_QUERYPRESENCE;

    int rc = vbglR3GRPerform(&Req.header);
    if (RT_SUCCESS(rc))
    {
        if ((Req.u32Flags & VMMDEV_CREDENTIALS_PRESENT) == 0)
            rc = VERR_NOT_FOUND;
    }
    return rc;
}


/**
 * Retrieves and clears the user credentials for logging into the guest OS.
 *
 * @returns IPRT status value
 * @param   ppszUser        Receives pointer of allocated user name string.
 *                          The returned pointer must be freed using VbglR3CredentialsDestroy().
 * @param   ppszPassword    Receives pointer of allocated user password string.
 *                          The returned pointer must be freed using VbglR3CredentialsDestroy().
 * @param   ppszDomain      Receives pointer of allocated domain name string.
 *                          The returned pointer must be freed using VbglR3CredentialsDestroy().
 */
VBGLR3DECL(int) VbglR3CredentialsRetrieve(char **ppszUser, char **ppszPassword, char **ppszDomain)
{
    VMMDevCredentials Req;
    RT_ZERO(Req);
    vmmdevInitRequest((VMMDevRequestHeader*)&Req, VMMDevReq_QueryCredentials);
    Req.u32Flags |= VMMDEV_CREDENTIALS_READ | VMMDEV_CREDENTIALS_CLEAR;

    int rc = vbglR3GRPerform(&Req.header);
    if (RT_SUCCESS(rc))
    {
        rc = RTStrDupEx(ppszUser, Req.szUserName);
        if (RT_SUCCESS(rc))
        {
            rc = RTStrDupEx(ppszPassword, Req.szPassword);
            if (RT_SUCCESS(rc))
            {
                rc = RTStrDupEx(ppszDomain, Req.szDomain);
                if (RT_SUCCESS(rc))
                    return VINF_SUCCESS;

                RTStrFree(*ppszPassword);
            }
            RTStrFree(*ppszUser);
        }
    }
    return rc;
}


/**
 * Clears and frees the three strings.
 *
 * @param   pszUser        Receives pointer of the user name string to destroy.
 *                         Optional.
 * @param   pszPassword    Receives pointer of the password string to destroy.
 *                         Optional.
 * @param   pszDomain      Receives pointer of allocated domain name string.
 *                         Optional.
 * @param   cPasses        Number of wipe passes.  The more the better + slower.
 */
VBGLR3DECL(void) VbglR3CredentialsDestroy(char *pszUser, char *pszPassword, char *pszDomain, uint32_t cPasses)
{
    size_t const    cchUser     = pszUser     ? strlen(pszUser)     : 0;
    size_t const    cchPassword = pszPassword ? strlen(pszPassword) : 0;
    size_t const    cchDomain   = pszDomain   ? strlen(pszDomain)   : 0;

    do
    {
        if (cchUser)
            memset(pszUser,     0xff, cchUser);
        if (cchPassword)
            memset(pszPassword, 0xff, cchPassword);
        if (cchDomain)
            memset(pszDomain,   0xff, cchDomain);
        ASMMemoryFence();

        if (cchUser)
            memset(pszUser,     0x00, cchUser);
        if (cchPassword)
            memset(pszPassword, 0x00, cchPassword);
        if (cchDomain)
            memset(pszDomain,   0x00, cchDomain);
        ASMMemoryFence();

        if (cchUser)
            RTRandBytes(pszUser,     cchUser);
        if (cchPassword)
            RTRandBytes(pszPassword, cchPassword);
        if (cchDomain)
            RTRandBytes(pszDomain,   cchDomain);
        ASMMemoryFence();

    } while (cPasses-- > 0);

    RTStrFree(pszUser);
    RTStrFree(pszPassword);
    RTStrFree(pszDomain);
}
