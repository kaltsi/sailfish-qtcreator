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

#include "mertarget.h"
#include "mersdk.h"
#include "merconstants.h"
#include "mersdkmanager.h"
#include "mertoolchain.h"
#include "merqtversion.h"
#include "mersdkkitinformation.h"
#include "mertargetkitinformation.h"
#include <debugger/debuggerkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionfactory.h>
#include <projectexplorer/toolchainmanager.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <QDir>
#include <QStringList>

namespace Mer {
namespace Internal {

using namespace Mer::Constants;

const char* wrapperScripts[] =
{
    MER_WRAPPER_QMAKE,
    MER_WRAPPER_MAKE,
    MER_WRAPPER_GCC,
    MER_WRAPPER_DEPLOY,
    MER_WRAPPER_RPM,
    MER_WRAPPER_RPMBUILD
};

MerTarget::MerTarget(MerSdk* mersdk):
    m_sdk(mersdk),
    m_defaultGdb(QLatin1String(Constants::MER_DEBUGGER_DEFAULT_FILENAME))
{
}

MerTarget::~MerTarget()
{
}

QString MerTarget::name() const
{
    return m_name;
}

void MerTarget::setName(const QString &name)
{
    m_name = name;
}

void MerTarget::setQmakeQuery(const QString &qmakeQuery)
{
    m_qmakeQuery = qmakeQuery;
}

void MerTarget::setGccDumpMachine(const QString &gccMachineDump)
{
    m_gccMachineDump = gccMachineDump;
    // the dump contains a linefeed
    m_gccMachineDump.remove(QRegExp(QLatin1String("\\n")));
}

void MerTarget::setDefaultGdb(const QString &name)
{
    m_defaultGdb=name;
}

bool MerTarget::fromMap(const QVariantMap &data)
{
    m_name = data.value(QLatin1String(Constants::MER_TARGET_NAME)).toString();
    m_qmakeQuery = data.value(QLatin1String(Constants::MER_TARGET_QMAKE_DUMP)).toString();
    m_gccMachineDump = data.value(QLatin1String(Constants::MER_TARGET_GCC_DUMP)).toString();
    m_defaultGdb = data.value(QLatin1String(Constants::MER_TARGET_DEFAULT_DEBUGGER)).toString();
    return isValid();
}

QVariantMap MerTarget::toMap() const
{
    QVariantMap result;
    result.insert(QLatin1String(Constants::MER_TARGET_NAME), m_name);
    result.insert(QLatin1String(Constants::MER_TARGET_QMAKE_DUMP), m_qmakeQuery);
    result.insert(QLatin1String(Constants::MER_TARGET_GCC_DUMP), m_gccMachineDump);
    result.insert(QLatin1String(Constants::MER_TARGET_DEFAULT_DEBUGGER), m_defaultGdb);
    return result;
}

bool MerTarget::isValid() const
{
    return !m_name.isEmpty() && !m_qmakeQuery.isEmpty() && !m_gccMachineDump.isEmpty();
}

QString MerTarget::targetPath() const
{
    return MerSdkManager::sdkToolsDirectory() + m_sdk->virtualMachineName() + QLatin1Char('/') + m_name;
}

bool MerTarget::createScripts() const
{
    if (!isValid())
        return false;

    const QString targetPath(this->targetPath());

    QDir targetDir(targetPath);
    if (!targetDir.exists() && !targetDir.mkpath(targetPath)) {
        qWarning() << "Could not create target directory." << targetDir;
        return false;
    }
    bool result = true;
    for (size_t i = 0; i < sizeof(wrapperScripts) / sizeof(wrapperScripts[0]); ++i)
         result &= createScript(targetPath, i);

    const QString qmakepath = targetPath + QLatin1Char('/') + QLatin1String(Constants::QMAKE_QUERY);
    const QString gccpath = targetPath + QLatin1Char('/') + QLatin1String(Constants::GCC_DUMPMACHINE);

    QString patchedQmakeQuery = m_qmakeQuery;
    patchedQmakeQuery.replace(QLatin1String(":/"),
                              QString::fromLatin1(":%1/%2/").arg(m_sdk->sharedTargetsPath()).arg(m_name));

    result &= createCacheFile(qmakepath, patchedQmakeQuery);
    result &= createCacheFile(gccpath, m_gccMachineDump);

    return result;
}

void MerTarget::deleteScripts() const
{
    Utils::FileUtils::removeRecursively(Utils::FileName::fromString(targetPath()));
}

ProjectExplorer::Kit* MerTarget::createKit() const
{
    if (!isValid())
        return 0;
    const QString sysroot(m_sdk->sharedTargetsPath() + QLatin1String("/") + m_name);

    Utils::FileName path = Utils::FileName::fromString(sysroot);
    if (!path.toFileInfo().exists()) {
        qWarning() << "Sysroot does not exist" << sysroot;
        return 0;
    }

    ProjectExplorer::Kit *k = new ProjectExplorer::Kit();
    k->setAutoDetected(true);
    k->setDisplayName(QString::fromLatin1("%1-%2").arg(m_sdk->virtualMachineName(), m_name));
    k->setIconPath(QLatin1String(Constants::MER_OPTIONS_CATEGORY_ICON));
    ProjectExplorer::SysRootKitInformation::setSysRoot(k, Utils::FileName::fromUserInput(sysroot));

    if (m_gccMachineDump.contains(QLatin1String("i486"))) {
        ProjectExplorer::DeviceTypeKitInformation::setDeviceTypeId(k, Constants::MER_DEVICE_TYPE_I486);
    } else if (m_gccMachineDump.contains(QLatin1String("arm"))) {
        ProjectExplorer::DeviceTypeKitInformation::setDeviceTypeId(k, Constants::MER_DEVICE_TYPE_ARM);
    }

    const QString gdb = Utils::HostOsInfo::withExecutableSuffix(m_defaultGdb);
    QString gdbDir = QCoreApplication::applicationDirPath();
    if (Utils::HostOsInfo::isMacHost()) {
        QDir dir = QDir(gdbDir);
        dir.cdUp();
        dir.cdUp();
        dir.cdUp();
        gdbDir = dir.path();
    }
    Utils::FileName gdbFileName = Utils::FileName::fromString(gdbDir + QLatin1String("/") + gdb);

    Debugger::DebuggerKitInformation::setEngineType(k,Debugger::GdbEngineType);
    Debugger::DebuggerKitInformation::setDebuggerCommand(k,gdbFileName);
    MerSdkKitInformation::setSdk(k,m_sdk);
    MerTargetKitInformation::setTargetName(k,name());
    return k;
}

MerQtVersion* MerTarget::createQtVersion() const
{
    const Utils::FileName qmake =
            Utils::FileName::fromString(targetPath() + QLatin1Char('/') +
                                        QLatin1String(Constants::MER_WRAPPER_QMAKE));
    QtSupport::QtVersionManager *qtvm = QtSupport::QtVersionManager::instance();
    // Is there a qtversion present for this qmake?
    QtSupport::BaseQtVersion *qtv = qtvm->qtVersionForQMakeBinary(qmake);
    if (qtv && !qtv->isValid()) {
        qtvm->removeVersion(qtv);
        qtv = 0;
    }
    if (!qtv)
        qtv = new MerQtVersion(qmake, true, targetPath());

    //QtSupport::QtVersionFactory::createQtVersionFromQMakePath(qmake, true, targetPath());

    QTC_ASSERT(qtv && qtv->type() == QLatin1String(Constants::MER_QT), return 0);

    MerQtVersion *merqtv = static_cast<MerQtVersion *>(qtv);
    const QString vmName = m_sdk->virtualMachineName();
    merqtv->setVirtualMachineName(vmName);
    merqtv->setTargetName(m_name);
    merqtv->setDisplayName(
                QString::fromLatin1("Qt %1 in %2 %3").arg(qtv->qtVersionString(),
                                                          vmName, m_name));
    return merqtv;
}

MerToolChain* MerTarget::createToolChain() const
{
    const Utils::FileName gcc =
            Utils::FileName::fromString(targetPath() + QLatin1Char('/') +
                                        QLatin1String(Constants::MER_WRAPPER_GCC));
    ProjectExplorer::ToolChainManager *tcm = ProjectExplorer::ToolChainManager::instance();
    QList<ProjectExplorer::ToolChain *> toolChains = tcm->toolChains();

    foreach (ProjectExplorer::ToolChain *tc, toolChains) {
        if (tc->compilerCommand() == gcc && tc->isAutoDetected()) {
            QTC_ASSERT(tc->type() == QLatin1String(Constants::MER_TOOLCHAIN_TYPE), return 0);
        }
    }

    MerToolChain* mertoolchain = new MerToolChain(true);
    const QString vmName = m_sdk->virtualMachineName();
    mertoolchain->setDisplayName(QString::fromLatin1("GCC (%1 %2)").arg(vmName, m_name));
    mertoolchain->setVirtualMachine(vmName);
    mertoolchain->setTargetName(m_name);
    mertoolchain->setCompilerCommand(gcc);
    return mertoolchain;
}

bool MerTarget::createScript(const QString &targetPath, int scriptIndex) const
{
    bool ok = false;
    const char* wrapperScriptCopy = wrapperScripts[scriptIndex];
    const QFile::Permissions rwrw = QFile::ReadOwner|QFile::ReadUser|QFile::ReadGroup
            |QFile::WriteOwner|QFile::WriteUser|QFile::WriteGroup;
    const QFile::Permissions rwxrwx = rwrw|QFile::ExeOwner|QFile::ExeUser|QFile::ExeGroup;

    QString wrapperScriptCommand = QLatin1String(wrapperScriptCopy);
    if (Utils::HostOsInfo::isWindowsHost())
        wrapperScriptCommand.chop(4); // remove the ".cmd"

    QString wrapperBinaryPath = QCoreApplication::applicationDirPath();
    if (Utils::HostOsInfo::isWindowsHost())
        wrapperBinaryPath += QLatin1String("/merssh.exe");
    else if (Utils::HostOsInfo::isMacHost())
        wrapperBinaryPath += QLatin1String("/../Resources/merssh");
    else
        wrapperBinaryPath += QLatin1String("/merssh");

    using namespace Utils;
    const QString scriptCopyPath = targetPath + QLatin1Char('/') + QLatin1String(wrapperScriptCopy);
    QDir targetDir(targetPath);
    const QString targetName = targetDir.dirName();
    targetDir.cdUp();
    const QString merDevToolsDir = QDir::toNativeSeparators(targetDir.canonicalPath());
    QFile script(scriptCopyPath);
    ok = script.open(QIODevice::WriteOnly);
    if (!ok) {
        qWarning() << "Could not open script" << scriptCopyPath;
        return false;
    }

    QString scriptContent;

    if (HostOsInfo::isWindowsHost()) {
        scriptContent += QLatin1String("@echo off\nSetLocal EnableDelayedExpansion\n");
        scriptContent += QLatin1String("set ARGUMENTS=\nFOR %%a IN (%*) DO set ARGUMENTS=!ARGUMENTS! ^ '%%a'\n");
        scriptContent += QLatin1String("set ") +
                QLatin1String(Mer::Constants::MER_SSH_TARGET_NAME) +
                QLatin1Char('=') + targetName + QLatin1Char('\n');
        scriptContent += QLatin1String("set ") +
                QLatin1String(Mer::Constants::MER_SSH_SDK_TOOLS) +
                QLatin1Char('=') + merDevToolsDir + QDir::separator() + targetName + QLatin1Char('\n');
        scriptContent += QLatin1String("SetLocal DisableDelayedExpansion\n");
        scriptContent += QLatin1Char('"') +
                QDir::toNativeSeparators(wrapperBinaryPath) + QLatin1String("\" ") +
                wrapperScriptCommand + QLatin1Char(' ') + QLatin1String("%ARGUMENTS%\n");
    }

    if (HostOsInfo::isAnyUnixHost()) {
        scriptContent += QLatin1String("#!/bin/sh\n");
        scriptContent += QLatin1String("ARGUMENTS=\"\";for ARGUMENT in \"$@\"; do ARGUMENTS=\"${ARGUMENTS} '${ARGUMENT}'\" ; done;\n");
        scriptContent += QLatin1String("export  ") +
                QLatin1String(Mer::Constants::MER_SSH_TARGET_NAME) +
                QLatin1Char('=') + targetName + QLatin1Char('\n');
        scriptContent += QLatin1String("export  ") +
                QLatin1String(Mer::Constants::MER_SSH_SDK_TOOLS) +
                QLatin1Char('=') + merDevToolsDir + QDir::separator() + targetName + QLatin1Char('\n');
        scriptContent += QLatin1String("exec ");
        scriptContent += QLatin1Char('"') +
                QDir::toNativeSeparators(wrapperBinaryPath) + QLatin1String("\" ") +
                wrapperScriptCommand + QLatin1Char(' ') + QLatin1String("${ARGUMENTS}");
    }


    ok = script.write(scriptContent.toUtf8()) != -1;
    if (!ok) {
        qWarning() << "Could not write script" << scriptCopyPath;
        return false;
    }

    ok = QFile::setPermissions(scriptCopyPath, rwxrwx);
    if (!ok) {
        qWarning() << "Could not set file permissions on script" << scriptCopyPath;
        return false;
    }
    return ok;
}

bool MerTarget::createCacheFile(const QString &fileName, const QString &content) const
{
    bool ok = false;

    QFile file(fileName);
    ok = file.open(QIODevice::WriteOnly);
    if (!ok) {
        qWarning() << "Could not open cache file" << fileName;
        return false;
    }

    ok = file.write(content.toUtf8()) != -1;
    if (!ok) {
        qWarning() << "Could not write cache file " << fileName;
        return false;
    }
    file.close();
    return ok;
}

bool MerTarget::operator==(const MerTarget &other) const
{
    return m_name == other.m_name && m_qmakeQuery == other.m_qmakeQuery && m_gccMachineDump == other.m_gccMachineDump;
}

}
}
