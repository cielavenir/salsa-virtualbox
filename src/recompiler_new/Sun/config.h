/* $Id: config.h 15278 2008-12-10 19:22:20Z vboxsync $ */
/** @file
 * Sun config - Maintained by hand
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
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

#include "config-host.h"
#define CONFIG_QEMU_PREFIX "/usr/gnemul/qemu-i386"
#define TARGET_ARCH "i386"
#define TARGET_I386 1
#define CONFIG_SOFTMMU 1

#ifdef VBOX_WITH_64_BITS_GUESTS
# if defined(__x86_64__) || defined (VBOX_ENABLE_VBOXREM64)
#  define TARGET_X86_64
# endif
#endif

/* Uncomment to see all phys memory accesses */
/* #define VBOX_DEBUG_PHYS */
/* Uncomment to see emulated CPU state changes */
/* #define VBOX_DUMP_STATE */
/* Uncomment to see QEMU logging, goes to /tmp/vbox-qemu.log */
/* #define DEBUG_ALL_LOGGING */
/* Uncomment to see generated code */
/* #define DEBUG_DISAS */

#if 0 /*defined(RT_ARCH_AMD64) && defined(VBOX_STRICT)*/
# define VBOX_CHECK_ADDR(ptr) do { if ((uintptr_t)(ptr) >= _4G) __asm__("int3"); } while (0)
#else
# define VBOX_CHECK_ADDR(ptr) do { } while (0)
#endif