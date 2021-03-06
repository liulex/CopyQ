/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "filterlineedit.h"

#include "gui/configurationmanager.h"
#include "gui/icons.h"
#include "gui/filtercompleter.h"

#include <QMenu>
#include <QPainter>
#include <QTimer>

/*!
    \class Utils::FilterLineEdit

    \brief The FilterLineEdit class is a fancy line edit customized for
    filtering purposes with a clear button.
*/

namespace Utils {

FilterLineEdit::FilterLineEdit(QWidget *parent)
   : FancyLineEdit(parent)
   , m_timerSearch( new QTimer(this) )
{
    setButtonVisible(Left, true);
    setButtonVisible(Right, true);
    connect(this, SIGNAL(rightButtonClicked()), this, SLOT(clear()));

    // search timer
    m_timerSearch->setSingleShot(true);
    m_timerSearch->setInterval(200);

    connect( m_timerSearch, SIGNAL(timeout()),
             this, SLOT(onTextChanged()) );
    connect( this, SIGNAL(textChanged(QString)),
             m_timerSearch, SLOT(start()) );

    // menu
    QMenu *menu = new QMenu(this);
    setButtonMenu(Left, menu);
    connect( menu, SIGNAL(triggered(QAction*)),
             this, SLOT(onMenuAction()) );

    m_actionRe = menu->addAction(tr("Regular Expression"));
    m_actionRe->setCheckable(true);

    m_actionCaseInsensitive = menu->addAction(tr("Case Insensitive"));
    m_actionCaseInsensitive->setCheckable(true);
}

QRegExp FilterLineEdit::filter() const
{
    Qt::CaseSensitivity sensitivity =
            m_actionCaseInsensitive->isChecked() ? Qt::CaseInsensitive : Qt::CaseSensitive;

    QString pattern;
    if (m_actionRe->isChecked()) {
        pattern = text();
    } else {
        foreach ( const QString &str, text().split(QRegExp("\\s+"), QString::SkipEmptyParts) ) {
            if ( !pattern.isEmpty() )
                pattern.append(".*");
            pattern.append( QRegExp::escape(str) );
        }
    }

    return QRegExp(pattern, sensitivity, QRegExp::RegExp2);
}

void FilterLineEdit::loadSettings()
{
    ConfigurationManager *cm = ConfigurationManager::instance();

    QVariant val;

    val = cm->value("filter_regular_expression");
    m_actionRe->setChecked(!val.isValid() || val.toBool());

    val = cm->value("filter_case_insensitive");
    m_actionCaseInsensitive->setChecked(!val.isValid() || val.toBool());

    // KDE has custom icons for this. Notice that icon namings are counter intuitive.
    // If these icons are not available we use the freedesktop standard name before
    // falling back to a bundled resource.
    QIcon icon1 = QIcon::fromTheme(layoutDirection() == Qt::LeftToRight ?
                     "edit-clear-locationbar-rtl" : "edit-clear-locationbar-ltr",
                     getIcon("edit-clear", IconRemove));
    setButtonIcon(Right, icon1);

    QIcon icon2 = getIcon("edit-find", IconSearch);
    setButtonIcon(Left, icon2);

    if ( cm->value("save_filter_history").toBool() ) {
        if ( !completer() ) {
            FilterCompleter::installCompleter(this);
            completer()->setProperty( "history", cm->value("filter_history") );
        }
    } else {
        FilterCompleter::removeCompleter(this);
        cm->setValue("filter_history", QString());
    }
}

void FilterLineEdit::hideEvent(QHideEvent *event)
{
    FancyLineEdit::hideEvent(event);

    if (completer()) {
        ConfigurationManager *cm = ConfigurationManager::instance();
        const QStringList history = completer()->property("history").toStringList();
        cm->setValue("filter_history", history);
    }
}

void FilterLineEdit::onTextChanged()
{
    emit filterChanged(filter());
}

void FilterLineEdit::onMenuAction()
{
    ConfigurationManager *cm = ConfigurationManager::instance();
    cm->setValue("filter_regular_expression", m_actionRe->isChecked());
    cm->setValue("filter_case_insensitive", m_actionCaseInsensitive->isChecked());

    const QRegExp re = filter();
    if ( !re.isEmpty() )
        emit filterChanged(re);
}

} // namespace Utils
