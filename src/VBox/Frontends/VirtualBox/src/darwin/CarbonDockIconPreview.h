/* $Id: CarbonDockIconPreview.h $ */
/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * CarbonDockIconPreview class declaration
 */

/*
 * Copyright (C) 2009 Sun Microsystems, Inc.
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

#ifndef ___CarbonDockIconPreview_h___
#define ___CarbonDockIconPreview_h___

/* VBox includes */
#include "AbstractDockIconPreview.h"

class CarbonDockIconPreview: public AbstractDockIconPreview, protected AbstractDockIconPreviewHelper
{
public:
    CarbonDockIconPreview (VBoxConsoleWnd *aMainWnd, const QPixmap& aOverlayImage);
    ~CarbonDockIconPreview();

    virtual void updateDockOverlay();
    virtual void updateDockPreview (CGImageRef aVMImage);
    virtual void updateDockPreview (VBoxFrameBuffer *aFrameBuffer);

private:
    inline void initOverlayData (int aBitmapByteCount);
    void updateDockPreviewImpl (CGContextRef aContext, CGImageRef aVMImage);

    void *mBitmapData;
};

#endif /* ___CarbonDockIconPreview_h___ */
