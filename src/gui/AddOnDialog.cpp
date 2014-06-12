/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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

#include <QStandardItemModel>

#include "AddOnDialog.hpp"
#include "ui_addonDialog.h"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"

AddOnDialog::AddOnDialog(QObject* parent) : StelDialog(parent)
{
    ui = new Ui_addonDialogForm;
}

AddOnDialog::~AddOnDialog()
{
	delete ui;
	ui = NULL;
}

void AddOnDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void AddOnDialog::styleChanged()
{
}

void AddOnDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
		this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));

	// default tab
	ui->stackedWidget->setCurrentIndex(0);
	ui->stackListWidget->setCurrentRow(0);

	// CATALOGS
	setUpTableView(ui->catalogsTableView);
	initModel(ui->catalogsTableView);

	// LANDSCAPES
	setUpTableView(ui->landscapeTableView);
	initModel(ui->landscapeTableView);

	// LANGUAGE PACK
	setUpTableView(ui->languageTableView);
	initModel(ui->languageTableView);

	// SCRIPTS
	setUpTableView(ui->scriptsTableView);
	initModel(ui->scriptsTableView);

	// STARLORE
	setUpTableView(ui->starloreTbleView);
	initModel(ui->starloreTbleView);

	// TEXTURES
	setUpTableView(ui->texturesTableView);
	initModel(ui->texturesTableView);
}

void AddOnDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	if (!current)
	{
		current = previous;
	}
	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));
}

void AddOnDialog::setUpTableView(QTableView* tableView) {
	tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void AddOnDialog::initModel(QTableView* tableView)
{
	int rowCount = 0;
	QStandardItemModel* model = new QStandardItemModel(rowCount, ColumnCount);
	model->setHeaderData(ColumnTitle, Qt::Horizontal, q_("Title"));
	model->setHeaderData(ColumnInstalledVersion, Qt::Horizontal, q_("Installed Version"));
	model->setHeaderData(ColumnLastVersion, Qt::Horizontal, q_("Last Version"));
	tableView->setModel(model);
}
