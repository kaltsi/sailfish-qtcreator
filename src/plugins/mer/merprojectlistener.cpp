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

#include "merprojectlistener.h"
#include "mersdkmanager.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4nodes.h>

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/fileutils.h>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Qt4ProjectManager;

namespace Mer {
namespace Internal {

MerProjectListener::MerProjectListener(QObject *parent)
    : QObject(parent)
{
    connect(MerSdkManager::instance(), SIGNAL(initialized()), SLOT(init()));
}

void MerProjectListener::init()
{
    // This is called just once after the MerSdkManager is initialized.
    const SessionManager *sessionManager = ProjectExplorerPlugin::instance()->session();
    const QList<Project*> projects = sessionManager->projects();
    // O(n2)
    foreach (Project *project, projects)
        onProjectAdded(project);

    connect(sessionManager, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            SLOT(onProjectAdded(ProjectExplorer::Project*)));
    connect(sessionManager, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            SLOT(onProjectRemoved(ProjectExplorer::Project*)));
}

void MerProjectListener::onTargetAddedToProject(Target *target)
{
    Project *project = target->project();
    if (MerSdkManager::instance()->merKits().contains(target->kit()))
        handleProject_private(project);
}

void MerProjectListener::onTargetRemovedFromProject(Target *target)
{
    Project *project = target->project();
    const QList<Kit *> merKits = MerSdkManager::instance()->merKits();
    int merTargetsCount = 0;
    // Counting how many Mer Targets are left after this removal
    foreach (Target *projectTarget, project->targets())
        if (merKits.contains(projectTarget->kit()))
            merTargetsCount++;
    if (merTargetsCount == 0)
        forgetProject(project);
}

void MerProjectListener::onProjectAdded(Project *project)
{
    connect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            SLOT(onTargetAddedToProject(ProjectExplorer::Target*)));
    connect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
            SLOT(onTargetRemovedFromProject(ProjectExplorer::Target*)));

    const QList<Kit *> merKits = MerSdkManager::instance()->merKits();
    foreach (Kit *kit, merKits) {
        if (project->supportsKit(kit)) {
            if (handleProject_private(project))
                break;
        }
    }
}

void MerProjectListener::onProjectRemoved(Project *project)
{
    forgetProject(project);
}

bool MerProjectListener::handleProject_private(Project *project)
{
    Qt4Project *qt4Project = qobject_cast<Qt4Project*>(project);
    if (!qt4Project)
        return false;
    return handleProject(qt4Project);
}

} // Internal
} // Mer
