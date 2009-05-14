/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "tcfengine.h"

#include "debuggerdialogs.h"
#include "breakhandler.h"
#include "debuggerconstants.h"
#include "debuggermanager.h"
#include "disassemblerhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"
#include "moduleshandler.h"
#include "gdbmi.h"

#include <utils/qtcassert.h>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QToolTip>

#include <QtNetwork/QTcpSocket>


using namespace Debugger;
using namespace Debugger::Internal;
using namespace Debugger::Constants;

#define DEBUG_TCF 1
#if DEBUG_TCF
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s

#define STRINGIFY_INTERNAL(x) #x
#define STRINGIFY(x) STRINGIFY_INTERNAL(x)
#define CB(callback) &TcfEngine::callback, STRINGIFY(callback)


//#define USE_CONGESTION_CONTROL

///////////////////////////////////////////////////////////////////////
//
// TcfData
//
///////////////////////////////////////////////////////////////////////


TcfData::TcfData(const QByteArray &data)
{
    fromString(data);
}


///////////////////////////////////////////////////////////////////////
//
// TcfCommand
//
///////////////////////////////////////////////////////////////////////


QString TcfEngine::TcfCommand::toString() const
{
    return quoteUnprintableLatin1(command);
}


///////////////////////////////////////////////////////////////////////
//
// TcfEngine
//
///////////////////////////////////////////////////////////////////////

TcfEngine::TcfEngine(DebuggerManager *parent)
{
    q = parent;
    qq = parent->engineInterface();

    m_congestion = 0;
    m_inAir = 0;

    m_sendTimer.setSingleShot(true);
    m_sendTimer.setInterval(100); // ms
    connect(&m_sendTimer, SIGNAL(timeout()), this, SLOT(handleSendTimer()));

    m_socket = new QTcpSocket(this);
    connect(m_socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
        this, SLOT(socketError(QAbstractSocket::SocketError)));

    //void aboutToClose ()
    //void bytesWritten ( qint64 bytes )
    //void readChannelFinished ()
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));

    //connect(m_socket, SIGNAL(hostFound())
    //connect(m_socket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy, QAuthenticator *)))
    //connect(m_socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
    //    thism SLOT(socketStateChanged(QAbstractSocket::SocketState)));

    connect(this, SIGNAL(tcfOutputAvailable(QString,QString)),
        q, SLOT(showDebuggerOutput(QString,QString)),
        Qt::QueuedConnection);
    connect(this, SIGNAL(tcfInputAvailable(QString,QString)),
        q, SLOT(showDebuggerInput(QString,QString)),
        Qt::QueuedConnection);
    connect(this, SIGNAL(applicationOutputAvailable(QString)),
        q, SLOT(showApplicationOutput(QString)),
        Qt::QueuedConnection);
}

TcfEngine::~TcfEngine()
{
}

void TcfEngine::socketReadyRead()
{
    //XSDEBUG("TcfEngine::socketReadyRead()");
    m_inbuffer.append(m_socket->readAll());
    int pos = 0;
    while (1) {
        int next = m_inbuffer.indexOf("\3\1", pos);
        //qDebug() << "pos: " << pos << "next: " << next;
        if (next == -1)
            break;
        handleResponse(m_inbuffer.mid(pos, next - pos));
        pos = next + 2; 
    }
    m_inbuffer.clear();
}

void TcfEngine::socketConnected()
{
    q->showStatusMessage("Socket connected.");
    m_socket->waitForConnected(2000);
    //sendCommand("Locator", "redirect", "ID");
}

void TcfEngine::socketDisconnected()
{
    XSDEBUG("FIXME:  TcfEngine::socketDisconnected()");
}

void TcfEngine::socketError(QAbstractSocket::SocketError)
{
    QString msg = tr("%1.").arg(m_socket->errorString());
    //QMessageBox::critical(q->mainWindow(), tr("Error"), msg);
    q->showStatusMessage(msg);
    qq->notifyInferiorExited();
}

void TcfEngine::executeDebuggerCommand(const QString &command)
{
    Q_UNUSED(command);
    XSDEBUG("FIXME:  TcfEngine::executeDebuggerCommand()");
}

void TcfEngine::shutdown()
{
    m_congestion = 0;
    m_inAir = 0;
    m_services.clear();
    exitDebugger(); 
}

void TcfEngine::exitDebugger()
{
    SDEBUG("TcfEngine::exitDebugger()");
    qq->notifyInferiorExited();
}

bool TcfEngine::startDebugger()
{
    qq->notifyInferiorRunningRequested();
    int pos = q->m_remoteChannel.indexOf(':');
    QString host = q->m_remoteChannel.left(pos);
    quint16 port = q->m_remoteChannel.mid(pos + 1).toInt();
    //QTimer::singleShot(0, this, SLOT(runInferior()));
    m_socket->connectToHost(host, port);
    return true;
}

void TcfEngine::continueInferior()
{
    SDEBUG("TcfEngine::continueInferior()");
}

void TcfEngine::runInferior()
{
}

void TcfEngine::interruptInferior()
{
    XSDEBUG("TcfEngine::interruptInferior()");
}

void TcfEngine::stepExec()
{
    //SDEBUG("TcfEngine::stepExec()");
}

void TcfEngine::stepIExec()
{
    //SDEBUG("TcfEngine::stepIExec()");
}

void TcfEngine::stepOutExec()
{
    //SDEBUG("TcfEngine::stepOutExec()");
}

void TcfEngine::nextExec()
{
    //SDEBUG("TcfEngine::nextExec()");
}

void TcfEngine::nextIExec()
{
    //SDEBUG("TcfEngine::nextIExec()");
}

void TcfEngine::runToLineExec(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName);
    Q_UNUSED(lineNumber);
    SDEBUG("FIXME:  TcfEngine::runToLineExec()");
}

void TcfEngine::runToFunctionExec(const QString &functionName)
{
    Q_UNUSED(functionName);
    XSDEBUG("FIXME:  TcfEngine::runToFunctionExec()");
}

void TcfEngine::jumpToLineExec(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName);
    Q_UNUSED(lineNumber);
    XSDEBUG("FIXME:  TcfEngine::jumpToLineExec()");
}

void TcfEngine::activateFrame(int index)
{
    Q_UNUSED(index);
}

void TcfEngine::selectThread(int index)
{
    Q_UNUSED(index);
}

void TcfEngine::attemptBreakpointSynchronization()
{
}

void TcfEngine::reloadDisassembler()
{
}

void TcfEngine::loadSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName);
}

void TcfEngine::loadAllSymbols()
{
}

void TcfEngine::reloadModules()
{
}

QList<Symbol> TcfEngine::moduleSymbols(const QString & /*moduleName*/)
{
    return QList<Symbol>();
}


void TcfEngine::handleResponse(const QByteArray &response)
{
    static QTime lastTime;

    //emit tcfOutputAvailable(_("            "), currentTime());
    QList<QByteArray> parts = response.split('\0');
    if (parts.size() < 2 || !parts.last().isEmpty()) {
        SDEBUG("WRONG RESPONSE PACKET LAYOUT" << parts);
        return;
    }
    parts.removeLast(); // always empty
    QByteArray tag = parts.at(0);
    int n = parts.size();
    if (n == 2 && tag == "N") { // unidentified command
        int token = parts.at(1).toInt();
        TcfCommand tcf = m_cookieForToken[token];
        SDEBUG("COMMAND NOT RECOGNIZED FOR TOKEN" << token << tcf.toString());
        emit tcfOutputAvailable("", QString::number(token) + "^"
               + "NOT RECOQNIZED: " + quoteUnprintableLatin1(response));
        acknowledgeResult();
    } else if (n == 2 && tag == "F") { // flow control
        m_congestion = parts.at(1).toInt();
        SDEBUG("CONGESTION: " << m_congestion);
    } else if (n == 4 && tag == "R") { // result data
        acknowledgeResult();
        int token = parts.at(1).toInt();
        QByteArray message = parts.at(2);
        TcfData data(parts.at(3));
        emit tcfOutputAvailable("", QString::number(token) + "^"
            + quoteUnprintableLatin1(response) + data.toString());
        TcfCommand tcf = m_cookieForToken[token];
        TcfData result(data);
        SDEBUG("GOOD RESPONSE: " << quoteUnprintableLatin1(response));
        if (tcf.callback)
            (this->*(tcf.callback))(result, tcf.cookie);
    } else if (n == 3 && tag == "P") { // progress data (partial result)
        //int token = parts.at(1).toInt();
        QByteArray data = parts.at(2);
        SDEBUG(_("\nTCF PARTIAL:") << quoteUnprintableLatin1(response));
    } else if (n == 4 && tag == "E") { // an event
        QByteArray service = parts.at(1);
        QByteArray eventName = parts.at(2);
        TcfData data(parts.at(3));
        if (eventName != "peerHeartBeat")
            SDEBUG(_("\nTCF EVENT:") << quoteUnprintableLatin1(response)
                << data.toString());
        if (service == "Locator" && eventName == "Hello") {
            m_services.clear();
            foreach (const GdbMi &service, data.children())
                m_services.append(service.data());
            QTimer::singleShot(0, this, SLOT(startDebugging()));
        }
    } else {
        SDEBUG("UNKNOWN RESPONSE PACKET:"
            << quoteUnprintableLatin1(response) << parts);
    }
}

void TcfEngine::startDebugging()
{
    foreach (const QByteArray &service, m_services) {
        postCommand(CB(handleRunControlGetChildren),
            service, "getChildren", "\"\"");
    }

    postCommand(CB(handleRunControlGetChildren), "Diagnostics", "getChildren");
    postCommand(CB(handleRunControlGetChildren), "Streams", "getChildren");
    postCommand(CB(handleRunControlGetChildren), "Expressions", "getChildren");
    postCommand(CB(handleRunControlGetChildren), "SysMonitor", "getChildren");
    //postCommand(CB(handleRunControlGetChildren), "FileSystem", "getChildren");
    postCommand(CB(handleRunControlGetChildren), "Processes", "getChildren");
    postCommand(CB(handleRunControlGetChildren), "LineNumbers", "getChildren");
    postCommand(CB(handleRunControlGetChildren), "Symbols", "getChildren");
    postCommand(CB(handleRunControlGetChildren), "StackTrace", "getChildren");
    postCommand(CB(handleRunControlGetChildren), "Registers", "getChildren");
    postCommand(CB(handleRunControlGetChildren), "Memory", "getChildren");
    postCommand(CB(handleRunControlGetChildren), "Breakpoints", "getChildren");
    postCommand(CB(handleRunControlGetChildren), "RunControl", "getChildren");
    postCommand(CB(handleRunControlGetChildren), "Locator", "getChildren");


    //postCommand(CB(handleRunControlSuspend),
    //    "RunControl", "suspend", "\"Thread1\"");
    //postCommand(CB(handleRunControlSuspend),
    //    "RunControl", "getContext", "\"P12318\"");

    //postCommand("Locator", "sync", "");
    //postCommand("Locator", "redirect", "ID");
}

void TcfEngine::postCommand(TcfCommandCallback callback,
    const char *callbackName,
    const QByteArray &service,
    const QByteArray &cmd,
    const QByteArray &args)
{
    static int token = 20;
    ++token;
    
    const char delim = 0;
    //const char marker_eom = -1;
    //const char marker_eos = -2;
    //const char marker_null = -3;

    QByteArray ba = "C";
    ba.append(delim);
    ba.append(QByteArray::number(token));
    ba.append(delim);
    ba.append(service);
    ba.append(delim);
    ba.append(cmd);
    ba.append(delim);
    ba.append(args);
    ba.append(delim);
    ba.append(3);
    ba.append(1);

    TcfCommand tcf;
    tcf.command = ba;
    tcf.callback = callback;
    tcf.callbackName = callbackName;
    tcf.token = token;

    m_cookieForToken[token] = tcf;

    enqueueCommand(tcf);  // congestion based
}

// Congestion control does not seem to work that way. Basically it's
// already too late when we get a flow control packet
void TcfEngine::enqueueCommand(const TcfCommand &cmd)
{
#ifdef USE_CONGESTION_CONTROL
    // congestion controled
    if (m_congestion <= 0 && m_sendQueue.isEmpty()) {
        //SDEBUG("DIRECT SEND" << cmd.toString());
        sendCommandNow(cmd);
    } else {
        SDEBUG("QUEUE " << cmd.toString());
        m_sendQueue.enqueue(cmd);
        m_sendTimer.start();
    }
#else
    // synchrounously
    if (m_inAir == 0)
        sendCommandNow(cmd);
    else
        m_sendQueue.enqueue(cmd);
#endif
}

void TcfEngine::handleSendTimer()
{
    QTC_ASSERT(!m_sendQueue.isEmpty(), return);

    if (m_congestion > 0) {
        // not ready...
        SDEBUG("WAITING FOR CONGESTION TO GO DOWN...");
        m_sendTimer.start();
    } else {
        // go!
        sendCommandNow(m_sendQueue.dequeue());
    }
}

void TcfEngine::sendCommandNow(const TcfCommand &cmd)
{
    ++m_inAir;
    int result = m_socket->write(cmd.command);
    m_socket->flush();
    emit tcfInputAvailable("send", QString::number(cmd.token)
        + " " + cmd.toString() + " " + QString::number(result));
    SDEBUG("SEND " <<  cmd.toString() << " " << QString::number(result));
}

void TcfEngine::acknowledgeResult()
{
#if !defined(USE_CONGESTION_CONTROL)
    QTC_ASSERT(m_inAir == 1, /**/);
    m_inAir = 0;
    if (!m_sendQueue.isEmpty())
        sendCommandNow(m_sendQueue.dequeue());
#endif
}

void TcfEngine::handleRunControlSuspend(const TcfData &data, const QVariant &)
{
    SDEBUG("HANDLE RESULT");
}

void TcfEngine::handleRunControlGetChildren(const TcfData &data, const QVariant &)
{
    SDEBUG("HANDLE RUN CONTROL GET CHILDREN" << data.toString());
}

void TcfEngine::handleSysMonitorGetChildren(const TcfData &data, const QVariant &)
{
    SDEBUG("HANDLE RUN CONTROL GET CHILDREN" << data.toString());
}


//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////

static WatchData m_toolTip;
static QPoint m_toolTipPos;
static QHash<QString, WatchData> m_toolTipCache;

void TcfEngine::setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos)
{
    Q_UNUSED(mousePos)
    Q_UNUSED(editor)
    Q_UNUSED(cursorPos)
}

//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void TcfEngine::assignValueInDebugger(const QString &expression,
    const QString &value)
{
    XSDEBUG("ASSIGNING: " << expression + '=' + value);
    updateLocals();
}

void TcfEngine::updateLocals()
{
}

void TcfEngine::updateWatchModel()
{
    qq->watchHandler()->rebuildModel();
    q->showStatusMessage(tr("Stopped."), 5000);
}

void TcfEngine::updateSubItem(const WatchData &data0)
{
    QTC_ASSERT(false, return);
}

IDebuggerEngine *createTcfEngine(DebuggerManager *parent, QList<Core::IOptionsPage*> *)
{
    return new TcfEngine(parent);
}

