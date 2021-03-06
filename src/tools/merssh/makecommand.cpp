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

#include "makecommand.h"
#include "merremoteprocess.h"
#include <mer/merconstants.h>
#include <QStringList>
#include <QEventLoop>

MakeCommand::MakeCommand()
{

}

QString MakeCommand::name() const
{
    return QLatin1String("make");
}

int MakeCommand::execute()
{
    const QString projectPathParameter = projectPath().isEmpty() ? QString() : QLatin1String(" -p ") +
    QLatin1String("'") + projectPath() +   QLatin1String("'");

    QString command = QLatin1String("mb2") +
                      projectPathParameter +
                      QLatin1String(" -t ") +
                      targetName() +
                      QLatin1Char(' ') + arguments().join(QLatin1String(" ")) + QLatin1Char(' ');

    MerRemoteProcess process;
    process.setSshParameters(sshParameters());
    process.setCommand(remotePathMapping(command));
    return process.executeAndWait();
}

bool MakeCommand::isValid() const
{
    return Command::isValid() && !targetName().isEmpty() ;
}
