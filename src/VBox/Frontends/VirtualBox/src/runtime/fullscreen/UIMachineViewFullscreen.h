/* $Id: UIMachineViewFullscreen.h $ */
/** @file
 * VBox Qt GUI - UIMachineViewFullscreen class declaration.
 */

/*
 * Copyright (C) 2010-2022 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef FEQT_INCLUDED_SRC_runtime_fullscreen_UIMachineViewFullscreen_h
#define FEQT_INCLUDED_SRC_runtime_fullscreen_UIMachineViewFullscreen_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Local includes */
#include "UIMachineView.h"

class UIMachineViewFullscreen : public UIMachineView
{
    Q_OBJECT;

protected:

    /* Fullscreen machine-view constructor: */
    UIMachineViewFullscreen(UIMachineWindow *pMachineWindow, ulong uScreenId);
    /* Fullscreen machine-view destructor: */
    virtual ~UIMachineViewFullscreen() {}

private slots:

    /* Handler: Console callback stuff: */
    void sltAdditionsStateChanged();

private:

    /* Event handlers: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /* Prepare routines: */
    void prepareCommon();
    void prepareFilters();
    void prepareConsoleConnections();

    /* Cleanup routines: */
    //void cleanupConsoleConnections() {}
    //void cleanupFilters() {}
    //void cleanupCommon() {}

    /** Returns whether the guest-screen auto-resize is enabled. */
    virtual bool isGuestAutoresizeEnabled() const RT_OVERRIDE { return m_bIsGuestAutoresizeEnabled; }
    /** Defines whether the guest-screen auto-resize is @a fEnabled. */
    virtual void setGuestAutoresizeEnabled(bool bEnabled) RT_OVERRIDE;

    /** Adjusts guest-screen size to correspond current <i>working area</i> size. */
    void adjustGuestScreenSize();

    /* Helpers: Geometry stuff: */
    QRect workingArea() const;
    QSize calculateMaxGuestSize() const;

    /* Private variables: */
    bool m_bIsGuestAutoresizeEnabled : 1;

    /* Friend classes: */
    friend class UIMachineView;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_fullscreen_UIMachineViewFullscreen_h */

