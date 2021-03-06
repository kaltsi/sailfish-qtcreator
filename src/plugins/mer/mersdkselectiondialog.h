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

#ifndef MERSDKSELECTIONDIALOG_H
#define MERSDKSELECTIONDIALOG_H

#include <QDialog>

namespace Mer {
namespace Internal {

namespace Ui {
class MerSdkSelectionDialog;
}

class MerSdkSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MerSdkSelectionDialog(QWidget *parent = 0);
    ~MerSdkSelectionDialog();

    QString selectedSdkName() const;

private slots:
    void handleItemSelectionChanged();
    void handleItemDoubleClicked();

private:
    Ui::MerSdkSelectionDialog *m_ui;
};

} // Internal
} // Mer

#endif // MER_SDKSELECTIONDIALOG_H
