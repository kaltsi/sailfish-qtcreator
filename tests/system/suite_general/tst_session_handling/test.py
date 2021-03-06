#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of Qt Creator.
##
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
#############################################################################

source("../../shared/qtcreator.py")

def main():
    projects = prepareTestExamples()
    if not projects:
        return
    sessionName = "SampleSession"
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    createAndSwitchToSession(sessionName)
    mainWindow = waitForObject(":Qt Creator_Core::Internal::MainWindow")
    test.verify(waitFor("sessionName in str(mainWindow.windowTitle)", 2000),
                "Verifying window title contains created session name.")
    checkWelcomePage(sessionName, True)
    for project in projects:
        openQmakeProject(project)
    progressBarWait(20000)
    checkNavigator(46, "Verifying whether all projects have been opened.")
    openDocument("propertyanimation.QML.qml.color-animation\\.qml")
    openDocument("declarative-music-browser.Headers.utility\\.h")
    checkOpenDocuments(2, "Verifying whether 2 files are open.")
    originalText = str(waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget").plainText)
    switchSession("default")
    test.verify(waitFor("'Qt Creator' == str(mainWindow.windowTitle)", 2000),
                "Verifying window title is set to default.")
    checkWelcomePage(sessionName, False)
    switchViewTo(ViewConstants.EDIT)
    checkNavigator(1, "Verifying that no more project is opened.")
    checkOpenDocuments(0, "Verifying whether all files have been closed.")
    switchSession(sessionName)
    test.verify(waitFor("sessionName in str(mainWindow.windowTitle)", 2000),
                "Verifying window title contains created session name.")
    checkNavigator(46, "Verifying whether all projects have been re-opened.")
    checkOpenDocuments(2, "Verifying whether 2 files have been re-opened.")
    if test.verify("utility.h" in str(mainWindow.windowTitle),
                   "Verifying whether utility.h has been opened."):
        current = str(waitForObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget").plainText)
        test.verify(originalText == current, "Verifying that same file has been opened.")
    checkForSessionFile(sessionName, projects)
    invokeMenuItem("File", "Exit")

def prepareTestExamples():
    examples = [os.path.join(sdkPath, "Examples", "4.7", "declarative", "animation", "basics",
                             "property-animation", "propertyanimation.pro"),
                os.path.join(sdkPath, "Examples", "QtMobility", "declarative-music-browser",
                             "declarative-music-browser.pro")
                ]
    projects = []
    for sourceExample in examples:
        if not neededFilePresent(sourceExample):
            return None
    # copy example projects to temp directory
    for sourceExample in examples:
        templateDir = prepareTemplate(os.path.dirname(sourceExample))
        projects.append(os.path.join(templateDir, os.path.basename(sourceExample)))
    return projects

def switchSession(toSession):
    test.log("Switching to session '%s'" % toSession)
    invokeMenuItem("File", "Session Manager...")
    clickItem(waitForObject("{name='sessionList' type='QListWidget' visible='1' "
                            "window=':Session Manager_ProjectExplorer::Internal::SessionDialog'}"),
                            toSession, 5, 5, 0, Qt.LeftButton)
    clickButton(waitForObject("{name='btSwitch' text='Switch to' type='QPushButton' visible='1' "
                              "window=':Session Manager_ProjectExplorer::Internal::SessionDialog'}"))

def createAndSwitchToSession(toSession):
    sessionInputDialog = ("{type='ProjectExplorer::Internal::SessionNameInputDialog' unnamed='1' "
                          "visible='1' windowTitle='New session name'}")
    test.log("Switching to session '%s' after creating it." % toSession)
    invokeMenuItem("File", "Session Manager...")
    clickButton(waitForObject("{name='btCreateNew' text='New' type='QPushButton' visible='1' "
                              "window=':Session Manager_ProjectExplorer::Internal::SessionDialog'}"))
    lineEdit = waitForObject("{type='QLineEdit' unnamed='1' visible='1' window=%s}"
                             % sessionInputDialog)
    replaceEditorContent(lineEdit, toSession)
    clickButton(waitForObject("{text='Switch to' type='QPushButton' unnamed='1' visible='1' "
                              "window=%s}" % sessionInputDialog))

def checkWelcomePage(sessionName, isCurrent=False):
    switchViewTo(ViewConstants.WELCOME)
    mouseClick(waitForObject("{clip='false' container=':Qt Creator_QDeclarativeView' enabled='true'"
                             " text='Develop' type='LinkedText' unnamed='1' visible='true'}"),
                             5, 5, 0, Qt.LeftButton)
    waitForObject("{clip='false' container=':Qt Creator_QDeclarativeView' enabled='true' "
                  "text='Sessions' type='Text' unnamed='1' visible='true'}")
    if isCurrent:
        sessions = ["default", "%s (current session)" % sessionName]
    else:
        sessions = ["default (current session)", sessionName]
    for sessionName in sessions:
        test.verify(object.exists("{clip='false' container=':Qt Creator_QDeclarativeView' "
                                  "enabled='true' type='LinkedText' "
                                  "unnamed='1' visible='true' text='%s'}" % sessionName),
                                  "Verifying session '%s' exists." % sessionName)

def checkNavigator(expectedRows, message):
    navigatorModel = waitForObject(":Qt Creator_Utils::NavigationTreeView").model()
    test.compare(expectedRows, len(__iterateChildren__(navigatorModel, QModelIndex())), message)

def checkOpenDocuments(expectedRows, message):
    selectFromCombo(":Qt Creator_Core::Internal::NavComboBox", "Open Documents")
    openDocsWidget = waitForObject(":OpenDocuments_Widget")
    test.compare(openDocsWidget.model().rowCount(), expectedRows, message)

def checkForSessionFile(sessionName, proFiles):
    global tmpSettingsDir
    sessionFile = os.path.join(tmpSettingsDir, "QtProject", "qtcreator", "%s.qws" % sessionName)
    if test.verify(os.path.exists(sessionFile),
                   "Verifying whether session file '%s' has been created." % sessionFile):
        content = readFile(sessionFile)
        for proFile in proFiles:
            if platform.system() in ('Microsoft', 'Windows'):
                proFile = proFile.replace('\\', '/')
            test.verify(proFile in content, "Verifying whether expected .pro file (%s) is listed "
                        "inside session file." % proFile)

def init():
    removeQmlDebugFolderIfExists()
