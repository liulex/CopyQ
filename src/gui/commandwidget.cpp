/*
    Copyright (c) 2015, Lukas Holecek <hluk@email.cz>

    This file is part of CopyQ.

    CopyQ is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CopyQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CopyQ.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "commandwidget.h"
#include "ui_commandwidget.h"

#include "common/command.h"
#include "common/mimetypes.h"
#include "gui/configurationmanager.h"
#include "gui/iconfactory.h"
#include "gui/icons.h"
#include "gui/shortcutdialog.h"
#include "item/itemfactory.h"

#include <QAction>
#include <QFontMetrics>
#include <QMenu>

namespace {

const char globalShortcutsDisabled[] = "DISABLED";

const QIcon iconClipboard() { return getIcon("", IconPaste); }
const QIcon iconMenu() { return getIcon("", IconBars); }
const QIcon iconShortcut() { return getIcon("", IconKeyboard); }

QStringList serializeShortcuts(const QList<QKeySequence> &shortcuts, bool enabled = true)
{
    QStringList shortcutTexts;

    foreach (const QKeySequence &shortcut, shortcuts)
        shortcutTexts.append(shortcut.toString(QKeySequence::PortableText));

    if (!enabled && !shortcutTexts.isEmpty())
        shortcutTexts.append(globalShortcutsDisabled);

    return shortcutTexts;
}

void deserializeShortcuts(
        const QStringList &serializedShortcuts, ShortcutButton *shortcutButton,
        QCheckBox *checkBoxEnabled = NULL
        )
{
    shortcutButton->resetShortcuts();

    bool enabled = !serializedShortcuts.isEmpty();

    foreach (const QString &shortcutText, serializedShortcuts) {
        if (shortcutText == globalShortcutsDisabled)
            enabled = false;
        else
            shortcutButton->addShortcut(shortcutText);
    }

    if (checkBoxEnabled)
        checkBoxEnabled->setChecked(enabled);
}

} // namespace

CommandWidget::CommandWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CommandWidget)
{
    ui->setupUi(this);

    updateWidgets();

#ifdef NO_GLOBAL_SHORTCUTS
    ui->checkBoxGlobalShortcut->hide();
    ui->shortcutButtonGlobalShortcut->hide();
#else
    ui->checkBoxGlobalShortcut->setIcon(iconShortcut());
    ui->shortcutButtonGlobalShortcut->setExpectModifier(true);
#endif

    ui->groupBoxCommand->setFocusProxy(ui->commandEdit);

    ui->checkBoxAutomatic->setIcon(iconClipboard());
    ui->checkBoxInMenu->setIcon(iconMenu());

    ConfigurationManager *cm = ConfigurationManager::instance();

    // Add tab names to combo boxes.
    cm->initTabComboBox(ui->comboBoxCopyToTab);
    cm->initTabComboBox(ui->comboBoxOutputTab);

    // Add formats to combo boxex.
    QStringList formats = cm->itemFactory()->formatsToSave();
    formats.prepend(mimeText);
    formats.removeDuplicates();

    setComboBoxItems(ui->comboBoxInputFormat, formats);
    setComboBoxItems(ui->comboBoxOutputFormat, formats);
}

CommandWidget::~CommandWidget()
{
#if !defined(COPYQ_WS_X11) && !defined(Q_OS_WIN)
    ui->lineEditWindow->hide();
    ui->labelWindow->hide();
#endif
    delete ui;
}

Command CommandWidget::command() const
{
    Command c;
    c.name   = ui->lineEditName->text();
    c.re     = QRegExp( ui->lineEditMatch->text() );
    c.wndre  = QRegExp( ui->lineEditWindow->text() );
    c.matchCmd = ui->lineEditMatchCmd->text();
    c.cmd    = ui->commandEdit->command();
    c.sep    = ui->lineEditSeparator->text();
    c.input  = ui->comboBoxInputFormat->currentText();
    c.output = ui->comboBoxOutputFormat->currentText();
    c.wait   = ui->checkBoxWait->isChecked();
    c.automatic = ui->checkBoxAutomatic->isChecked();
    c.inMenu  = ui->checkBoxInMenu->isChecked();
    c.transform = ui->checkBoxTransform->isChecked();
    c.remove = ui->checkBoxIgnore->isChecked();
    c.hideWindow = ui->checkBoxHideWindow->isChecked();
    c.enable = true;
    c.icon   = ui->buttonIcon->currentIcon();
    c.shortcuts = serializeShortcuts( ui->shortcutButton->shortcuts() );
    c.globalShortcuts = serializeShortcuts(
                ui->shortcutButtonGlobalShortcut->shortcuts(),
                ui->checkBoxGlobalShortcut->isChecked() );
    c.tab    = ui->comboBoxCopyToTab->currentText();
    c.outputTab = ui->comboBoxOutputTab->currentText();

    return c;
}

void CommandWidget::setCommand(const Command &c)
{
    ui->lineEditName->setText(c.name);
    ui->lineEditMatch->setText( c.re.pattern() );
    ui->lineEditWindow->setText( c.wndre.pattern() );
    ui->lineEditMatchCmd->setText(c.matchCmd);
    ui->commandEdit->setCommand(c.cmd);
    ui->lineEditSeparator->setText(c.sep);
    ui->comboBoxInputFormat->setEditText(c.input);
    ui->comboBoxOutputFormat->setEditText(c.output);
    ui->checkBoxWait->setChecked(c.wait);
    ui->checkBoxAutomatic->setChecked(c.automatic);
    ui->checkBoxInMenu->setChecked(c.inMenu);
    ui->checkBoxTransform->setChecked(c.transform);
    ui->checkBoxIgnore->setChecked(c.remove);
    ui->checkBoxHideWindow->setChecked(c.hideWindow);
    ui->buttonIcon->setCurrentIcon(c.icon);
    deserializeShortcuts(c.shortcuts, ui->shortcutButton);
    deserializeShortcuts(
                c.globalShortcuts,
                ui->shortcutButtonGlobalShortcut,
                ui->checkBoxGlobalShortcut);
    ui->comboBoxCopyToTab->setEditText(c.tab);
    ui->comboBoxOutputTab->setEditText(c.outputTab);
}

QString CommandWidget::currentIcon() const
{
    return ui->buttonIcon->currentIcon();
}

void CommandWidget::on_lineEditName_textChanged(const QString &name)
{
    emit nameChanged(name);
}

void CommandWidget::on_buttonIcon_currentIconChanged(const QString &iconString)
{
    emit iconChanged(iconString);
}

void CommandWidget::on_checkBoxAutomatic_stateChanged(int)
{
    updateWidgets();
    emit automaticChanged(ui->checkBoxAutomatic->isChecked());
}

void CommandWidget::on_checkBoxInMenu_stateChanged(int)
{
    updateWidgets();
}

void CommandWidget::on_checkBoxGlobalShortcut_stateChanged(int)
{
    updateWidgets();
}

void CommandWidget::on_shortcutButtonGlobalShortcut_shortcutAdded(const QKeySequence &)
{
    updateWidgets();
}

void CommandWidget::on_shortcutButtonGlobalShortcut_shortcutRemoved(const QKeySequence &)
{
    updateWidgets();
}

void CommandWidget::on_commandEdit_changed()
{
    updateWidgets();
}

void CommandWidget::updateWidgets()
{
    bool inMenu = ui->checkBoxInMenu->isChecked();
    bool copyOrExecute = inMenu || ui->checkBoxAutomatic->isChecked();
#ifdef NO_GLOBAL_SHORTCUTS
    bool globalShortcut = false;
#else
    bool globalShortcut =
            ui->checkBoxGlobalShortcut->isChecked()
            && ui->shortcutButtonGlobalShortcut->shortcutCount() > 0;
#endif

    ui->shortcutButtonGlobalShortcut->setEnabled(ui->checkBoxGlobalShortcut->isChecked());
    ui->groupBoxMatchItems->setVisible(copyOrExecute);
    ui->groupBoxCommand->setVisible(copyOrExecute || globalShortcut);
    ui->groupBoxAction->setVisible(copyOrExecute);
    ui->groupBoxInMenu->setVisible(inMenu);
    ui->groupBoxCommandOptions->setHidden(!copyOrExecute || ui->commandEdit->isEmpty());
}
