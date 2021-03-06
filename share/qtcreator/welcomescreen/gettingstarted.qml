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

import QtQuick 1.1
import widgets 1.0

Rectangle {
    id: gettingStartedRoot
    width: 920
    height: 600

    PageCaption {
        id: pageCaption

        x: 32
        y: 8

        anchors.rightMargin: 16
        anchors.right: parent.right
        anchors.leftMargin: 16
        anchors.left: parent.left

        caption: qsTr("Getting Started")
    }

    Item {
        id: canvas

        width: 920
        height: 200

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 0

        GettingStartedItem {
            x: 688
            y: 83

            anchors.rightMargin: 32
            anchors.right: parent.right

            description: qsTr("To select a tutorial and learn how to develop applications.")
            title: qsTr("Start Developing")
            number: 4
            imageUrl: "widgets/images/gettingStarted04.png"
            onClicked: tab.currentIndex = 3;

        }

        GettingStartedItem {
            x: 468
            y: 83

            description: qsTr("To check that the Qt SDK installation was successful, open an example application and run it.")
            title: qsTr("Building and Running an Example Application")
            number: 3
            imageUrl: "widgets/images/gettingStarted03.png"
            onClicked: gettingStarted.openSplitHelp("qthelp://org.qt-project.qtcreator/doc/creator-build-example-application.html")
        }

        GettingStartedItem {
            id: gettingStartedItem

            x: 30
            y: 83

            imageUrl: "widgets/images/gettingStarted01.png"
            title: qsTr("IDE Overview")
            description: qsTr("To find out what kind of integrated environment (IDE) Qt Creator is.")
            number: 1

            onClicked: gettingStarted.openHelp("qthelp://org.qt-project.qtcreator/doc/creator-overview.html")
        }

        GettingStartedItem {
            x: 250
            y: 83

            description: qsTr("To become familiar with the parts of the Qt Creator user interface and to learn how to use them.")
            title: qsTr("User Interface")
            imageUrl: "widgets/images/gettingStarted02.png"
            number: 2
            onClicked: gettingStarted.openHelp("qthelp://org.qt-project.qtcreator/doc/creator-quick-tour.html")
        }

        Grid {
            id: grid

            x: 36
            y: 424

            spacing: 24

            rows: gettingStartedRoot.height > 640 ? 3 : 1

            IconAndLink {
                iconName: "userguideIcon"
                linkText: qsTr("User Guide")
                onClicked: gettingStarted.openHelp("qthelp://org.qt-project.qtcreator/doc/index.html")
            }

            IconAndLink {
                iconName: "communityIcon"
                linkText: qsTr("Online Community")
                onClicked: gettingStarted.openUrl("http://qt-project.org/forums")
            }

            IconAndLink {
                iconName: "labsIcon"
                linkText: qsTr("Blogs")
                onClicked: gettingStarted.openUrl("http://planet.qt-project.org")
            }
        }

        Image {
            x: 231
            y: 172
            source: "widgets/images/arrowBig.png"
        }

        Image {
            x: 451
            y: 172
            source: "widgets/images/arrowBig.png"
        }

        Image {
            x: 669
            y: 172
            source: "widgets/images/arrowBig.png"
        }

    }
}
