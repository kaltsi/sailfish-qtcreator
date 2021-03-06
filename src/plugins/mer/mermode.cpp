/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#include "mermode.h"
#include "merconstants.h"
#if !defined(QT_NO_WEBKIT)
#include "mermanagementwebview.h"
#endif
#include <coreplugin/modemanager.h>
#include <QDebug>

namespace Mer {
namespace Internal {

MerMode::MerMode()
{
#if !defined(QT_NO_WEBKIT)
    setWidget(new MerManagementWebView);
#else
    setWidget(new QWidget);
#endif
    setContext(Core::Context("Mer.MerMode"));
    setDisplayName(tr("SailfishOS"));
    setIcon(QIcon(QLatin1String(Constants::MER_OPTIONS_CATEGORY_ICON)));
    setPriority(80); // between "Projects" and "Analyze" modes
    setId("Mer.MerMode");
    setContextHelpId(QString());
    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*,Core::IMode*)),
            this, SLOT(handleUpdateContext(Core::IMode*,Core::IMode*)));
}

void MerMode::handleUpdateContext(Core::IMode *newMode, Core::IMode *oldMode)
{
#if !defined(QT_NO_WEBKIT)
    MerManagementWebView* view = qobject_cast<MerManagementWebView*>(widget());
    if (view && newMode == this) {
        view->setAutoFailReload(true);
    } else if (view && oldMode == this) {
        view->setAutoFailReload(false);
    }
#endif
}

} // namespace Internal
} // namespace Mer
