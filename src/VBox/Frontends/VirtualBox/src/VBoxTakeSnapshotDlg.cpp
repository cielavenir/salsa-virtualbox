/* $Id: VBoxTakeSnapshotDlg.cpp 28935 2010-04-30 13:14:32Z vboxsync $ */
/** @file
 *
 * VBox frontends: Qt4 GUI ("VirtualBox"):
 * VBoxTakeSnapshotDlg class implementation
 */

/*
 * Copyright (C) 2006-2009 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include "precomp.h"
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */
/* Global includes */
#include <QPushButton>

/* Local includes */
#include "VBoxTakeSnapshotDlg.h"
#include "VBoxProblemReporter.h"
#include "VBoxUtils.h"
# ifdef Q_WS_MAC
#  ifdef VBOX_WITH_NEW_RUNTIME_CORE
#   include "UIMachineWindowNormal.h"
#   include "VBoxSnapshotsWgt.h"
#  endif /* !VBOX_WITH_NEW_RUNTIME_CORE */
# endif /* !VBOX_WITH_NEW_RUNTIME_CORE */

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

VBoxTakeSnapshotDlg::VBoxTakeSnapshotDlg(QWidget *pParent, const CMachine &machine)
    : QIWithRetranslateUI<QIDialog>(pParent)
{
#ifdef Q_WS_MAC
# ifdef VBOX_WITH_NEW_RUNTIME_CORE
    /* No sheets in another mode than normal for now. Firstly it looks ugly and
     * secondly in some cases it is broken. */
    if (   qobject_cast<UIMachineWindowNormal*>(pParent)
        || qobject_cast<VBoxSnapshotsWgt*>(pParent))
        setWindowFlags (Qt::Sheet);
# endif /* !VBOX_WITH_NEW_RUNTIME_CORE */
# ifndef VBOX_FORCE_NEW_RUNTIME_CORE_ALWAYS
    /* Sheets are broken if the window is in fullscreen mode. So make it a
     * normal window in that case. */
    VBoxConsoleWnd *cwnd = qobject_cast<VBoxConsoleWnd*> (pParent);
    if (cwnd == NULL ||
        (!cwnd->isTrueFullscreen() &&
         !cwnd->isTrueSeamless()))
        setWindowFlags (Qt::Sheet);
# endif /* VBOX_FORCE_NEW_RUNTIME_CORE_ALWAYS */
#endif /* Q_WS_MAC */

    /* Apply UI decorations */
    Ui::VBoxTakeSnapshotDlg::setupUi(this);

    /* Alt key filter */
    QIAltKeyFilter *altKeyFilter = new QIAltKeyFilter(this);
    altKeyFilter->watchOn(mLeName);

    /* Setup connections */
    connect (mButtonBox, SIGNAL(helpRequested()), &vboxProblem(), SLOT(showHelpHelpDialog()));
    connect (mLeName, SIGNAL(textChanged(const QString &)), this, SLOT(nameChanged(const QString &)));

    /* Check if machine have immutable attachments */
    int immutableMediums = 0;

    if (machine.GetState() == KMachineState_Paused)
    {
        foreach (const CMediumAttachment &attachment, machine.GetMediumAttachments())
        {
            CMedium medium = attachment.GetMedium();
            if (!medium.isNull() && !medium.GetParent().isNull() && medium.GetBase().GetType() == KMediumType_Immutable)
                ++ immutableMediums;
        }
    }

    if (immutableMediums)
    {
        mLbInfo->setText(tr("Warning: You are taking a snapshot of a running machine which has %n immutable image(s) "
                            "attached to it. As long as you are working from this snapshot the immutable image(s) "
                            "will not be reset to avoid loss of data.", "",
                            immutableMediums));
        mLbInfo->useSizeHintForWidth(400);
    }
    else
    {
        QGridLayout *lt = qobject_cast<QGridLayout*>(layout());
        lt->removeWidget (mLbInfo);
        mLbInfo->setHidden (true);

        lt->removeWidget (mButtonBox);
        lt->addWidget (mButtonBox, 2, 0, 1, 2);
    }

    retranslateUi();
}

void VBoxTakeSnapshotDlg::retranslateUi()
{
    /* Translate uic generated strings */
    Ui::VBoxTakeSnapshotDlg::retranslateUi(this);
}

void VBoxTakeSnapshotDlg::nameChanged(const QString &strName)
{
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(!strName.trimmed().isEmpty());
}
