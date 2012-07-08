/*
 * Stellarium
 * Copyright (C) 2012 Anton Samoylov
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelApp.hpp"
#include "StelShortcutMgr.hpp"

#include <QDebug>
#include <QDialog>
#include <QLineEdit>

#include "ShortcutsDialog.hpp"
#include "ui_shortcutsDialog.h"

ShortcutLineEdit::ShortcutLineEdit(QWidget *parent) :
	QLineEdit(parent)
{
	clear();
}

QKeySequence ShortcutLineEdit::getKeySequence()
{
	return QKeySequence(m_keys[0], m_keys[1], m_keys[2], m_keys[3]);
}

void ShortcutLineEdit::clear()
{
	m_keyNum = m_keys[0] = m_keys[1] = m_keys[2] = m_keys[3] = 0;
	QLineEdit::clear();
	emit contentsChanged();
}

void ShortcutLineEdit::keyPressEvent(QKeyEvent *e)
{
	int nextKey = e->key();
	if ( m_keyNum > 3 || // too long shortcut
			 nextKey == Qt::Key_Control ||
			 nextKey == Qt::Key_Shift ||
			 nextKey == Qt::Key_Meta ||
			 nextKey == Qt::Key_Alt )
		return;
	nextKey |= getModifiers(e->modifiers(), e->text());
	m_keys[m_keyNum] = nextKey;
	++m_keyNum;
	QKeySequence ks(m_keys[0], m_keys[1], m_keys[2], m_keys[3]);
	setText(ks);
	emit contentsChanged();
	e->accept();
}

void ShortcutLineEdit::focusInEvent(QFocusEvent *e)
{
	emit focusChanged(true);
	QLineEdit::focusInEvent(e);
}

void ShortcutLineEdit::focusOutEvent(QFocusEvent *e)
{
	emit focusChanged(false);
	QLineEdit::focusOutEvent(e);
}


int ShortcutLineEdit::getModifiers(Qt::KeyboardModifiers state,
																				 const QString &text)
{
	int result = 0;
	// The shift modifier only counts when it is not used to type a symbol
	// that is only reachable using the shift key anyway
	if ((state & Qt::ShiftModifier) && (text.size() == 0
																			|| !text.at(0).isPrint()
																			|| text.at(0).isLetterOrNumber()
																			|| text.at(0).isSpace()))
		result |= Qt::SHIFT;
	if (state & Qt::ControlModifier)
		result |= Qt::CTRL;
	if (state & Qt::MetaModifier)
		result |= Qt::META;
	if (state & Qt::AltModifier)
		result |= Qt::ALT;
	return result;
}

ShortcutsDialog::ShortcutsDialog() :
	ui(new Ui_shortcutsDialogForm)
{
	shortcutMgr = StelApp::getInstance().getStelShortcutManager();
}

ShortcutsDialog::~ShortcutsDialog()
{
	delete ui; ui = NULL;
}

void ShortcutsDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateText();
	}
}

void ShortcutsDialog::initEditors()
{
	if (ui->shortcutsTreeWidget->currentItem()->isSelected()) {
		ui->primaryShortcutEdit->setEnabled(true);
		ui->altShortcutEdit->setEnabled(true);
		ui->primaryShortcutEdit->setText(
					ui->shortcutsTreeWidget->currentItem()->text(1));
		ui->altShortcutEdit->setText(
					ui->shortcutsTreeWidget->currentItem()->text(2));
		handleChanges();
	}
	else {
		ui->primaryShortcutEdit->setEnabled(false);
		ui->altShortcutEdit->setEnabled(false);
		ui->applyButton->setEnabled(false);
		ui->primaryShortcutEdit->clear();
		ui->altShortcutEdit->clear();
	}
}

void ShortcutsDialog::setActionsEnabled(bool enable)
{
	if (enable)
	{
		shortcutMgr->disableAllActions();
	}
	else
	{
		shortcutMgr->enableAllActions();
	}
}

void ShortcutsDialog::handleChanges()
{
	// updating apply button
	if (ui->primaryShortcutEdit->text() == ui->shortcutsTreeWidget->currentItem()->text(1) &&
			ui->altShortcutEdit->text() == ui->shortcutsTreeWidget->currentItem()->text(2))
	{
		// nothing to apply
		ui->applyButton->setEnabled(false);
	}
	else
	{
		ui->applyButton->setEnabled(true);
	}
	// updating clear buttons
	if (ui->primaryShortcutEdit->text().isEmpty())
	{
		ui->clearPrimaryButton->setEnabled(false);
	}
	else
	{
		ui->clearPrimaryButton->setEnabled(true);
	}
	if (ui->altShortcutEdit->text().isEmpty())
	{
		ui->clearAltButton->setEnabled(false);
	}
	else
	{
		ui->clearAltButton->setEnabled(true);
	}
}

void ShortcutsDialog::applyChanges()
{
	QString actionId = ui->shortcutsTreeWidget->currentItem()->data(0, Qt::UserRole).toString();
	QString groupId = ui->shortcutsTreeWidget->currentItem()->parent()->
			data(0, Qt::UserRole).toString();
	shortcutMgr->changeActionPrimaryKey(actionId, groupId, ui->primaryShortcutEdit->getKeySequence());
	shortcutMgr->changeActionAltKey(actionId, groupId, ui->altShortcutEdit->getKeySequence());
	ui->shortcutsTreeWidget->currentItem()->setText(1, ui->primaryShortcutEdit->text());
	ui->shortcutsTreeWidget->currentItem()->setText(2, ui->altShortcutEdit->text());
}

void ShortcutsDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->shortcutsTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(initEditors()));
	// apply/clear buttons logic
	connect(ui->applyButton, SIGNAL(released()), this, SLOT(applyChanges()));
	connect(ui->clearPrimaryButton, SIGNAL(released()), ui->primaryShortcutEdit, SLOT(clear()));
	connect(ui->clearAltButton, SIGNAL(released()), ui->altShortcutEdit, SLOT(clear()));
	// we need to disable all shortcut actions, so we can enter shortcuts without activating any actions
	connect(ui->primaryShortcutEdit, SIGNAL(focusChanged(bool)), this, SLOT(setActionsEnabled(bool)));
	connect(ui->altShortcutEdit, SIGNAL(focusChanged(bool)), this, SLOT(setActionsEnabled(bool)));
	// handling changes in editlines
	connect(ui->primaryShortcutEdit, SIGNAL(contentsChanged()), this, SLOT(handleChanges()));
	connect(ui->altShortcutEdit, SIGNAL(contentsChanged()), this, SLOT(handleChanges()));

	// Creating shortcuts tree
	QList<StelShortcutGroup*> groups = shortcutMgr->getGroupList();
	foreach (StelShortcutGroup* group, groups)
	{
		QTreeWidgetItem* groupItem = new QTreeWidgetItem(ui->shortcutsTreeWidget);
		groupItem->setFlags(Qt::ItemIsEnabled);
		groupItem->setText(0, group->getId());
		groupItem->setData(0, Qt::UserRole, group->getId());
		groupItem->setExpanded(true);
		// setup bold font for group lines
		QFont rootFont = groupItem->font(0);
		rootFont.setBold(true); rootFont.setPixelSize(14);
		groupItem->setFont(0, rootFont);
		QList<StelShortcut*> shortcuts = group->getActionList();
		foreach (StelShortcut* shortcut, shortcuts)
		{
			QTreeWidgetItem* shortcutItem = new QTreeWidgetItem(groupItem);
			shortcutItem->setText(0, shortcut->getText());
			shortcutItem->setText(1, shortcut->getPrimaryKey());
			shortcutItem->setText(2, shortcut->getAltKey());
			shortcutItem->setData(0, Qt::UserRole, QVariant(shortcut->getId()));
		}
	}
	updateText();
}

void ShortcutsDialog::updateText()
{
}
