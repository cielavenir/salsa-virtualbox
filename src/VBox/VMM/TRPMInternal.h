/* $Id: TRPMInternal.h 14515 2008-11-24 12:33:00Z vboxsync $ */
/** @file
 * TRPM - Internal header file.
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

#ifndef ___TRPMInternal_h
#define ___TRPMInternal_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/stam.h>
#include <VBox/cpum.h>



/* Enable to allow trap forwarding in GC. */
#define TRPM_FORWARD_TRAPS_IN_GC

/** First interrupt handler. Used for validating input. */
#define TRPM_HANDLER_INT_BASE  0x20

__BEGIN_DECLS


/** @defgroup grp_trpm_int   Internals
 * @ingroup grp_trpm
 * @internal
 * @{
 */

/** @name   TRPMGCTrapIn* flags.
 * The lower bits are offsets into the CPUMCTXCORE structure.
 * @{ */
/** The mask for the operation. */
#define TRPM_TRAP_IN_OP_MASK    0xffff
/** Traps on MOV GS, eax. */
#define TRPM_TRAP_IN_MOV_GS     1
/** Traps on MOV FS, eax. */
#define TRPM_TRAP_IN_MOV_FS     2
/** Traps on MOV ES, eax. */
#define TRPM_TRAP_IN_MOV_ES     3
/** Traps on MOV DS, eax. */
#define TRPM_TRAP_IN_MOV_DS     4
/** Traps on IRET. */
#define TRPM_TRAP_IN_IRET       5
/** Set if this is a V86 resume. */
#define TRPM_TRAP_IN_V86        RT_BIT(30)
/** If set this is a hypervisor register set. If cleared it's a guest set. */
#define TRPM_TRAP_IN_HYPER      RT_BIT(31)
/** @} */


/**
 * Converts a TRPM pointer into a VM pointer.
 * @returns Pointer to the VM structure the TRPM is part of.
 * @param   pTRPM   Pointer to TRPM instance data.
 */
#define TRPM2VM(pTRPM)          ( (PVM)((char*)pTRPM - pTRPM->offVM) )


/**
 * TRPM Data (part of VM)
 *
 * IMPORTANT! Keep the nasm version of this struct up-to-date.
 */
#pragma pack(4)
typedef struct TRPM
{
    /** Offset to the VM structure.
     * See TRPM2VM(). */
    RTINT                   offVM;

    /** Active Interrupt or trap vector number.
     * If not ~0U this indicates that we're currently processing
     * a interrupt, trap, fault, abort, whatever which have arrived
     * at that vector number.
     */
    RTUINT                  uActiveVector;

    /** Active trap type. */
    TRPMEVENT               enmActiveType;

    /** Errorcode for the active interrupt/trap. */
    RTGCUINT                uActiveErrorCode; /**< @todo don't use RTGCUINT */

    /** CR2 at the time of the active exception. */
    RTGCUINTPTR             uActiveCR2;

    /** Saved trap vector number. */
    RTGCUINT                uSavedVector; /**< @todo don't use RTGCUINT */

    /** Saved trap type. */
    TRPMEVENT               enmSavedType;

    /** Saved errorcode. */
    RTGCUINT                uSavedErrorCode;

    /** Saved cr2. */
    RTGCUINTPTR             uSavedCR2;

    /** Previous trap vector # - for debugging. */
    RTGCUINT                uPrevVector;

    /** IDT monitoring and sync flag (HWACC). */
    bool                    fDisableMonitoring; /** @todo r=bird: bool and 7 byte achPadding1. */

    /** Whether monitoring of the guest IDT is enabled or not.
     *
     * This configuration option is provided for speeding up guest like Solaris
     * that put the IDT on the same page as a whole lot of other data that is
     * freqently updated. The updates will cause #PFs and have to be interpreted
     * by PGMInterpretInstruction which is slow compared to raw execution.
     *
     * If the guest is well behaved and doesn't change the IDT after loading it,
     * there is no problem with dropping the IDT monitoring.
     *
     * @cfgm    /TRPM/SafeToDropGuestIDTMonitoring   boolean     defaults to false.
     */
    bool                    fSafeToDropGuestIDTMonitoring;

    /** Padding to get the IDTs at a 16 byte alignement. */
#if GC_ARCH_BITS == 32
    uint8_t                 abPadding1[6];
#else
    uint8_t                 abPadding1[14];
#endif
    /** IDTs. Aligned at 16 byte offset for speed. */
    VBOXIDTE                aIdt[256];

    /** Bitmap for IDTEs that contain PATM handlers. (needed for relocation) */
    uint32_t                au32IdtPatched[8];

    /** Temporary Hypervisor trap handlers.
     * NULL means default action. */
    RCPTRTYPE(void *)       aTmpTrapHandlers[256];

    /** RC Pointer to the IDT shadow area (aIdt) in HMA. */
    RCPTRTYPE(void *)       pvMonShwIdtRC;
    /** Current (last) Guest's IDTR. */
    VBOXIDTR                GuestIdtr;

    /** padding. */
    uint8_t                 au8Padding[2];

    /** Checked trap & interrupt handler array */
    RCPTRTYPE(void *)       aGuestTrapHandler[256];

    /** RC: The number of times writes to the Guest IDT were detected. */
    STAMCOUNTER             StatRCWriteGuestIDTFault;
    STAMCOUNTER             StatRCWriteGuestIDTHandled;

    /** HC: Profiling of the TRPMR3SyncIDT() method. */
    STAMPROFILE             StatSyncIDT;
    /** GC: Statistics for the trap handlers. */
    STAMPROFILEADV          aStatGCTraps[0x14];

    STAMPROFILEADV          StatForwardProfR3;
    STAMPROFILEADV          StatForwardProfRZ;
    STAMCOUNTER             StatForwardFailNoHandler;
    STAMCOUNTER             StatForwardFailPatchAddr;
    STAMCOUNTER             StatForwardFailR3;
    STAMCOUNTER             StatForwardFailRZ;

    STAMPROFILE             StatTrap0dDisasm;
    STAMCOUNTER             StatTrap0dRdTsc;    /**< Number of RDTSC #GPs. */

#ifdef VBOX_WITH_STATISTICS
    /* R3: Statistics for interrupt handlers (allocated on the hypervisor heap). */
    R3PTRTYPE(PSTAMCOUNTER) paStatForwardedIRQR3;
    /* R0: Statistics for interrupt handlers (allocated on the hypervisor heap). */
    R0PTRTYPE(PSTAMCOUNTER) paStatForwardedIRQR0;
    /* RC: Statistics for interrupt handlers (allocated on the hypervisor heap). */
    RCPTRTYPE(PSTAMCOUNTER) paStatForwardedIRQRC;
#endif
} TRPM;
#pragma pack()

/** Pointer to TRPM Data. */
typedef TRPM *PTRPM;

VMMRCDECL(int) trpmRCGuestIDTWriteHandler(PVM pVM, RTGCUINT uErrorCode, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, RTGCPTR pvRange, uintptr_t offRange);
VMMRCDECL(int) trpmRCShadowIDTWriteHandler(PVM pVM, RTGCUINT uErrorCode, PCPUMCTXCORE pRegFrame, RTGCPTR pvFault, RTGCPTR pvRange, uintptr_t offRange);

/**
 * Clear guest trap/interrupt gate handler
 *
 * @returns VBox status code.
 * @param   pVM         The VM to operate on.
 * @param   iTrap       Interrupt/trap number.
 */
VMMDECL(int) trpmClearGuestTrapHandler(PVM pVM, unsigned iTrap);


#ifdef IN_RING3

/**
 * Clear passthrough interrupt gate handler (reset to default handler)
 *
 * @returns VBox status code.
 * @param   pVM         The VM to operate on.
 * @param   iTrap       Trap/interrupt gate number.
 */
VMMR3DECL(int) trpmR3ClearPassThroughHandler(PVM pVM, unsigned iTrap);

#endif


#ifdef IN_RING0

/**
 * Calls the interrupt gate as if we received an interrupt while in Ring-0.
 *
 * @param   uIP     The interrupt gate IP.
 * @param   SelCS   The interrupt gate CS.
 * @param   RSP     The interrupt gate RSP. ~0 if no stack switch should take place. (only AMD64)
 */
DECLASM(void) trpmR0DispatchHostInterrupt(RTR0UINTPTR uIP, RTSEL SelCS, RTR0UINTPTR RSP);

/**
 * Issues a software interrupt to the specified interrupt vector.
 *
 * @param   uActiveVector   The vector number.
 */
DECLASM(void) trpmR0DispatchHostInterruptSimple(RTUINT uActiveVector);

#endif /* IN_RING0 */

/** @} */

__END_DECLS

#endif