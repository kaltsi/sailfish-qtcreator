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

#include "merhardwaredevicewizard.h"
#include <ssh/sshconnection.h>

using namespace QSsh;

namespace Mer {
namespace Internal {

MerHardwareDeviceWizard::MerHardwareDeviceWizard(QWidget *parent)
    : QWizard(parent),
      m_setupPage(this),
      m_keyPage(this),
      m_finalPage(this)
{
    setWindowTitle(tr("New Mer ARM Device  Setup"));
    addPage(&m_setupPage);
    addPage(&m_keyPage);
    addPage(&m_finalPage);
    m_finalPage.setCommitPage(true);
}

MerHardwareDeviceWizard::~MerHardwareDeviceWizard()
{

}

QString MerHardwareDeviceWizard::hostName() const
{
    return m_setupPage.hostName();
}

QString MerHardwareDeviceWizard::userName() const
{
    return m_setupPage.userName();
}

QString MerHardwareDeviceWizard::password() const
{
    return m_keyPage.password();
}

QString MerHardwareDeviceWizard::privateKeyFilePath() const
{
    return m_keyPage.privateKeyFilePath();
}

QString MerHardwareDeviceWizard::publicKeyFilePath() const
{
    return m_keyPage.publicKeyFilePath();
}

QString MerHardwareDeviceWizard::configurationName() const
{
    return m_setupPage.configName();
}

QString MerHardwareDeviceWizard::freePorts() const
{
     return m_setupPage.freePorts();
}

int MerHardwareDeviceWizard::sshPort() const
{
    return m_setupPage.sshPort();
}

int MerHardwareDeviceWizard::timeout() const
{
    return m_setupPage.timeout();
}

bool MerHardwareDeviceWizard::isNewSshKeysRquired() const
{
    return m_keyPage.isNewSshKeysRquired();
}

QString MerHardwareDeviceWizard::sharedSshPath() const
{
    return m_keyPage.sharedSshPath();
}

} // Internal
} // Mer
