/*
 * Stellarium TelescopeControl plug-in
 * 
 * Copyright (C) 2009-2011 Bogdan Marinov (this file)
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

#include "Dialog.hpp"
#include "ui_telescopePropertiesWindow.h"
#include "TelescopePropertiesWindow.hpp"
#include "TelescopeControl.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelStyle.hpp"
#include "StelTranslator.hpp"

#include <QDebug>
#include <QCompleter>
#include <QFrame>
#include <QStandardItemModel>
#include <QTimer>
#include <QTreeView>

TelescopePropertiesWindow::TelescopePropertiesWindow()
{
	ui = new Ui_widgetTelescopeProperties;
	
	deviceManager = GETSTELMODULE(TelescopeControl);
	//After the removal of the separate telescope server executables,
	//this list can't be changed after Stellarium has been started:
	//TODO: Simply call telescopeManager->getDeviceModels every time instead of
	//storing it?
	deviceModelNames = deviceManager->getDeviceModels().keys();
	deviceModelNames.sort();

	//Test the update for JSON
	clientNameValidator = new QRegExpValidator (QRegExp("[^:\"]+"), this);
	//TODO: Write a proper host/IP regexp?
	hostNameValidator = new QRegExpValidator (QRegExp("[a-zA-Z0-9\\-\\.]+"), this);
	#ifdef Q_OS_WIN32
	serialPortValidator = new QRegExpValidator (QRegExp("COM[0-9]+"), this);
	#else
	serialPortValidator = new QRegExpValidator (QRegExp("/dev/.*"), this);
	#endif
}

TelescopePropertiesWindow::~TelescopePropertiesWindow()
{	
	delete ui;
	
	delete clientNameValidator;
	delete hostNameValidator;
	delete serialPortValidator;
}

void TelescopePropertiesWindow::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

// Initialize the dialog widgets and connect the signals/slots
void TelescopePropertiesWindow::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Inherited connect
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()),this, SLOT(discardChanges()));
	connect(dialog, SIGNAL(rejected()), this, SLOT(discardChanges()));
	
	//Connect: sender, signal, receiver, member
	connect(ui->pushButtonDirectConnection, SIGNAL(clicked()),
	        this, SLOT(prepareDirectConnection()));
	connect(ui->pushButtonIndirectConnection, SIGNAL(clicked()),
	        this, SLOT(prepareIndirectConnection()));
	connect(ui->pushButtonSave, SIGNAL(clicked()),
	        this, SLOT(saveChanges()));
	connect(ui->pushButtonDiscard, SIGNAL(clicked()),
	        this, SLOT(discardChanges()));
	
	connect(ui->comboBoxDeviceModel,
	        SIGNAL(currentIndexChanged(const QString&)),
	        this,
	        SLOT(deviceModelSelected(const QString&)));

#ifdef Q_OS_WIN32
	connect(ui->pushButtonAscomSelect, SIGNAL(clicked()),
	        this, SLOT(showAscomSelector()));
	connect(ui->pushButtonAscomDeviceSetup, SIGNAL(clicked()),
	        this, SLOT(showAscomDeviceSetup()));
#endif
	
	//Setting validators
	ui->lineEditName->setValidator(clientNameValidator);
	ui->lineEditHostName->setValidator(hostNameValidator);
	ui->lineEditSerialPort->setValidator(serialPortValidator);
}

void TelescopePropertiesWindow::prepareNewStellariumConfiguration(const QString& id)
{
	configuredId = id;
	configuredConnectionInterface = ConnectionStellarium;

	ui->stelWindowTitle->setText("New Stellarium Telescope");

	//The user must choose between direct and indirect connection
	ui->stackedWidget->setCurrentWidget(ui->pageType);

	//Prepare the rest of the window
	ui->tabWidget->setCurrentWidget(ui->tabGeneral);
	ui->lineEditName->setText(configuredId);
	ui->doubleSpinBoxDelay->setValue(SECONDS_FROM_MICROSECONDS(DEFAULT_DELAY));
	ui->radioButtonJ2000->setChecked(true);
	ui->checkBoxConnectAtStartup->setChecked(false);
	fovCircleList.clear();
	populateShortcutNumberList();

	showConnectionTab(true);
	showAscomTab(false);
	showIndiTab(false);
}

void TelescopePropertiesWindow::prepareNewIndiConfiguration(const QString& id)
{
	configuredId = id;
	configuredConnectionInterface = ConnectionIndi;

	ui->stelWindowTitle->setText("New INDI Connection");

	//The user must choose between direct and indirect connection
	ui->stackedWidget->setCurrentWidget(ui->pageType);

	//Prepare the rest of the window
	ui->tabWidget->setCurrentWidget(ui->tabGeneral);
	ui->lineEditName->setText(configuredId);
	ui->doubleSpinBoxDelay->setValue(SECONDS_FROM_MICROSECONDS(DEFAULT_DELAY));
	ui->radioButtonJ2000->setChecked(true);
	ui->checkBoxConnectAtStartup->setChecked(false);
	fovCircleList.clear();
	populateShortcutNumberList();

	showConnectionTab(true);
	showAscomTab(false);
}

void TelescopePropertiesWindow::prepareNewVirtualConfiguration(const QString& id)
{
	configuredId = id;
	configuredConnectionInterface = ConnectionVirtual;
	configuredConnectionIsRemote = false;

	ui->stelWindowTitle->setText("New Simulated Telescope");

	ui->tabWidget->setCurrentWidget(ui->tabGeneral);
	showConnectionTab(false);
	showSerialTab(false);
	showNetworkTab(false);
	showAscomTab(false);
	showIndiTab(false);

	ui->lineEditName->setText(configuredId);
	ui->checkBoxConnectAtStartup->setChecked(true);
	fovCircleList.clear();
	populateShortcutNumberList();

	ui->stackedWidget->setCurrentWidget(ui->pageProperties);
}

#ifdef Q_OS_WIN32
void TelescopePropertiesWindow::prepareNewAscomConfiguration(const QString& id)
{
	if (!deviceManager->canUseAscom())
	{
		emit changesDiscarded();
		return;
	}

	configuredId = id;
	configuredConnectionInterface = ConnectionAscom;
	configuredConnectionIsRemote = false;

	ui->stelWindowTitle->setText("New ASCOM Connection");

	ui->tabWidget->setCurrentWidget(ui->tabGeneral);
	showConnectionTab(true);
	showAscomTab(true);
	showSerialTab(false);
	showNetworkTab(false);
	showIndiTab(false);

	ui->lineEditName->setText(configuredId);
	ui->doubleSpinBoxDelay->setValue(SECONDS_FROM_MICROSECONDS(DEFAULT_DELAY));
	ui->radioButtonJ2000->setChecked(true);
	ui->checkBoxConnectAtStartup->setChecked(false);
	//ui->groupBoxFovCircles->setChecked(false);
	//ui->lineEditFovCircleSizes->clear();
	populateShortcutNumberList();

	ui->lineEditAscomControlId->clear();
	ascomDriverObjectId.clear();

	ui->stackedWidget->setCurrentWidget(ui->pageProperties);
}
#endif

void TelescopePropertiesWindow::prepareForExistingConfiguration(const QString& id)
{
	configuredId = id;

	ui->stelWindowTitle->setText("Connection configuration");
	ui->tabWidget->setCurrentWidget(ui->tabGeneral);
	showConnectionTab(true);
	ui->stackedWidget->setCurrentWidget(ui->pageProperties);
	
	//Read the telescope properties
	const QVariantMap& properties = deviceManager->getConnection(id);
	if(properties.isEmpty())
	{
		//TODO: Add debug
		return;
	}

	QString name = properties.value("name").toString();
	ui->lineEditName->setText(name);

	//Properties that are valid for all or have default values:
	int delay = properties.value("delay", DEFAULT_DELAY).toInt();
	ui->doubleSpinBoxDelay->setValue(SECONDS_FROM_MICROSECONDS(delay));

	if (properties.value("equinox").toString() == "JNow")
		ui->radioButtonJNow->setChecked(true);
	else
		ui->radioButtonJ2000->setChecked(true);

	bool connectAtStartup = properties.value("connectsAtStartup", false).toBool();
	ui->checkBoxConnectAtStartup->setChecked(connectAtStartup);

	fovCircleList = properties.value("fovCircles").toList();
	
	populateShortcutNumberList();
	int shortcutNumber = properties.value("shortcutNumber").toInt();
	QString shortcutNumberString = QString::number(shortcutNumber);
	if (shortcutNumber > 0 && shortcutNumber < 10)
	{
		ui->comboBoxShortcutNumber->addItem(shortcutNumberString,
		                                    shortcutNumber);
	}
	else
	{
		shortcutNumber = 0;
	}
	int index = ui->comboBoxShortcutNumber->findData(shortcutNumber);
	ui->comboBoxShortcutNumber->setCurrentIndex(index);

	//Detecting protocol/interface and connection type
	QString interface = properties.value("interface").toString();
	bool isRemote = properties.value("isRemoteConnection", false).toBool();

	if (interface == "Stellarium")
	{
		configuredConnectionInterface = ConnectionStellarium;
		showAscomTab(false);
		showIndiTab(false);

		if (isRemote)
		{
			configuredConnectionIsRemote = true;
			showNetworkTab(true);
			showSerialTab(false);

			//TCP port
			int tcpPort = properties.value("tcpPort", deviceManager->getFreeTcpPort()).toInt();
			ui->spinBoxTcpPort->setValue(tcpPort);

			//Host name/address
			ui->lineEditHostName->setText(properties.value("host", "localhost").toString());
		}
		else
		{
			configuredConnectionIsRemote = false;
			showNetworkTab(false);
			showSerialTab(true);

			QString driver = properties.value("driverId").toString();
			QString deviceModelName = properties.value("deviceModel").toString();
			if (driver.isEmpty() || deviceModelName.isEmpty())
			{
				emit changesDiscarded();
				return;
			}

			if(!deviceModelName.isEmpty())
			{
				populateDeviceModelList();
				//Make the current device model selected in the list
				int index = ui->comboBoxDeviceModel->findText(deviceModelName);
				if(index < 0)
				{
					qDebug() << "TelescopeConfigurationDialog: Current device model is not in the list?";
					emit changesDiscarded();
					return;
				}
				else
				{
					ui->comboBoxDeviceModel->setCurrentIndex(index);
				}
			}
			else
			{
				//TODO: Abnormal exit
				emit changesDiscarded();
				return;
			}

			//Initialize the serial port value
			QString serialPort = properties.value("serialPort").toString();
			ui->lineEditSerialPort->setText(serialPort);
		}
	}
	else if (interface == "INDI")
	{
		//TODO: Reuse the code from the Stellarium case
		configuredConnectionInterface = ConnectionIndi;
		showSerialTab(false);
		showAscomTab(false);

		if (isRemote)
		{
			configuredConnectionIsRemote = true;
			showNetworkTab(true);
			showIndiTab(false);

			//TCP port
			int tcpPort = properties.value("tcpPort", deviceManager->getFreeTcpPort()).toInt();
			ui->spinBoxTcpPort->setValue(tcpPort);

			//Host name/address
			ui->lineEditHostName->setText(properties.value("host", "localhost").toString());
		}
		else
		{
			configuredConnectionIsRemote = false;
			showNetworkTab(false);
			showIndiTab(true);

			QString driver = properties.value("driverId").toString();
			if (driver.isEmpty())
			{
				emit changesDiscarded();
				return;
			}
			QString deviceModelName = properties.value("deviceModel").toString();
			if(!deviceModelName.isEmpty())
			{
				populateIndiDeviceModelList();
				//Make the current device model selected in the list
				bool deviceFound = false;
				QAbstractItemModel* model = ui->comboBoxIndiDeviceModel->model();
				for (int i = 0; i < model->rowCount(); i++)
				{
					QModelIndex index = model->index(i, 0);
					int rows = model->rowCount(index);
					for (int j = 0; j < rows; j++)
					{
						QModelIndex childIndex = model->index(j, 0, index);
						if (model->data(childIndex) == deviceModelName)
						{
							QModelIndex oldRoot = ui->comboBoxDeviceModel->rootModelIndex();
							ui->comboBoxIndiDeviceModel->setRootModelIndex(index);
							ui->comboBoxIndiDeviceModel->setCurrentIndex(j);
							ui->comboBoxIndiDeviceModel->setRootModelIndex(oldRoot);
							deviceFound = true;
							break;
						}
					}
					if (deviceFound)
						break;
				}
			}
			else
			{
				//TODO: Abnormal exit
				emit changesDiscarded();
				return;
			}
		}

	}
#ifdef Q_OS_WIN32
	else if (interface == "ASCOM")
	{
		if (!deviceManager->canUseAscom())
		{
			emit changesDiscarded();
			return;
		}

		QString driverId = properties.value("driverId").toString();
		if (driverId.isEmpty())
		{
			emit changesDiscarded();
			return;
		}
		configuredConnectionInterface = ConnectionAscom;
		ascomDriverObjectId = driverId;

		showAscomTab(true);
		showSerialTab(false);
		showIndiTab(false);
		showNetworkTab(false);

		ui->lineEditAscomControlId->setText(driverId);
		return;//Normal exit
	}
#endif
	else
	{
		configuredConnectionInterface = ConnectionVirtual;
		configuredConnectionIsRemote = false;

		showConnectionTab(false);
		showAscomTab(false);
		showIndiTab(false);
		showSerialTab(false);
		showNetworkTab(false);
	}
}

void TelescopePropertiesWindow::prepareDirectConnection()
{
	configuredConnectionIsRemote = false;
	showConnectionTab(true);
	showAscomTab(false);
	showNetworkTab(false);

	if (configuredConnectionInterface == ConnectionStellarium)
	{
		showSerialTab(true);
		showIndiTab(false);

		ui->lineEditSerialPort->clear();
		//TODO: FIX THIS:
		ui->lineEditSerialPort->setCompleter(new QCompleter(SERIAL_PORT_NAMES, this));
		ui->lineEditSerialPort->setText(SERIAL_PORT_NAMES.value(0));
		populateDeviceModelList();
		ui->comboBoxDeviceModel->setCurrentIndex(0);
	}
	else if (configuredConnectionInterface == ConnectionIndi)
	{
		showIndiTab(true);
		showSerialTab(false);

		populateIndiDeviceModelList();
	}

	ui->stackedWidget->setCurrentWidget(ui->pageProperties);
}

void TelescopePropertiesWindow::prepareIndirectConnection()
{
	configuredConnectionIsRemote = true;
	showConnectionTab(true);
	showNetworkTab(true);
	showAscomTab(false);
	showIndiTab(false);
	showSerialTab(false);

	ui->lineEditHostName->setText("localhost");
	if (configuredConnectionInterface == ConnectionIndi)
	{
		//TODO: Remove the magic number
		ui->spinBoxTcpPort->setValue(7624);
	}
	else
		ui->spinBoxTcpPort->setValue(deviceManager->getFreeTcpPort());

	ui->stackedWidget->setCurrentWidget(ui->pageProperties);
}

void TelescopePropertiesWindow::saveChanges()
{
	//Main telescope properties
	QString name = ui->lineEditName->text().trimmed();
	if(name.isEmpty())
		return;

	QVariantMap newTelescopeProperties;
	newTelescopeProperties.insert("name", name);

	bool connectAtStartup = ui->checkBoxConnectAtStartup->isChecked();
	newTelescopeProperties.insert("connectsAtStartup", connectAtStartup);
	
	if (!fovCircleList.isEmpty())
		newTelescopeProperties.insert("fovCircles", fovCircleList);

	int index = ui->comboBoxShortcutNumber->currentIndex();
	int shortcutNumber = ui->comboBoxShortcutNumber->itemData(index).toInt();
	if (shortcutNumber > 0)
		newTelescopeProperties.insert("shortcutNumber", shortcutNumber);
	
	//Interface properties
	//TODO: When adding, check for success!
	QString interface = "virtual";

	if (configuredConnectionInterface != ConnectionVirtual)
	{
		//All non-virtual client types require equinox information.
		QString equinox("J2000");
		if (ui->radioButtonJNow->isChecked())
			equinox = "JNow";
		newTelescopeProperties.insert("equinox", equinox);

		int delay = MICROSECONDS_FROM_SECONDS(ui->doubleSpinBoxDelay->value());
		newTelescopeProperties.insert("delay", delay);
	}

	if (configuredConnectionInterface == ConnectionStellarium)
	{
		interface = "Stellarium";
		newTelescopeProperties.insert("isRemoteConnection",
		                              configuredConnectionIsRemote);
		if (configuredConnectionIsRemote)
		{
			int tcpPort = ui->spinBoxTcpPort->value();
			newTelescopeProperties.insert("tcpPort", tcpPort);

			QString hostName = ui->lineEditHostName->text().trimmed();
			if(hostName.isEmpty())//TODO: Validate host
				return;
			if(hostName != "localhost")
			{
				newTelescopeProperties.insert("host", hostName);
			}
		}
		else
		{
			//Read the serial port
			QString serialPortName = ui->lineEditSerialPort->text();
			if(!serialPortName.startsWith(SERIAL_PORT_PREFIX))
				return;//TODO: Add more validation!
			newTelescopeProperties.insert("serialPort", serialPortName);

			QString deviceModel = ui->comboBoxDeviceModel->currentText();
			newTelescopeProperties.insert("deviceModel", deviceModel);

			QString driver = deviceManager->getDeviceModels().value(deviceModel).driver;
			newTelescopeProperties.insert("driverId", driver);
		}
	}
	else if (configuredConnectionInterface == ConnectionIndi)
	{
		interface = "INDI";
		newTelescopeProperties.insert("isRemoteConnection",
		                              configuredConnectionIsRemote);
		if (configuredConnectionIsRemote)
		{
			int tcpPort = ui->spinBoxTcpPort->value();
			newTelescopeProperties.insert("tcpPort", tcpPort);

			QString hostName = ui->lineEditHostName->text().trimmed();
			if(hostName.isEmpty())//TODO: Validate host
				return;
			if(hostName != "localhost")
			{
				newTelescopeProperties.insert("host", hostName);
			}
		}
		else
		{
			QModelIndex index = ui->comboBoxIndiDeviceModel->view()->currentIndex();
			if (!index.isValid())
				return;
			QAbstractItemModel* model = ui->comboBoxIndiDeviceModel->model();
			QString deviceModel = model->data(index).toString();
			index = model->index(index.row(), 1, index.parent());
			QString driver = model->data(index, Qt::UserRole).toString();
			
			newTelescopeProperties.insert("deviceModel", deviceModel);
			newTelescopeProperties.insert("driverId", driver);
		}
	}
#ifdef Q_OS_WIN32
	else if (configuredConnectionInterface == ConnectionAscom)
	{
		QString ascomControlId = ui->lineEditAscomControlId->text();
		if (ascomControlId.isEmpty())
			return;
		newTelescopeProperties.insert("driverId", ascomControlId);

		interface = "ASCOM";
	}
#endif

	newTelescopeProperties.insert("interface", interface);

	//Deal with renaming:
	if (configuredId != name)
		deviceManager->removeConnection(configuredId);
	deviceManager->addConnection(newTelescopeProperties);

	emit changesSaved(name);
}

void TelescopePropertiesWindow::discardChanges()
{
	emit changesDiscarded();
}

void TelescopePropertiesWindow::deviceModelSelected(const QString& deviceModelName)
{
	ui->labelDeviceModelDescription->setText(deviceManager->getDeviceModels().value(deviceModelName).description);
	ui->doubleSpinBoxDelay->setValue(SECONDS_FROM_MICROSECONDS(deviceManager->getDeviceModels().value(deviceModelName).defaultDelay));
}

void TelescopePropertiesWindow::showTab(QWidget *tab, const QString& label)
{
	if (ui->tabWidget->indexOf(tab) < 0)
		ui->tabWidget->addTab(tab, label);
}

void TelescopePropertiesWindow::hideTab(QWidget *tab)
{
	ui->tabWidget->removeTab(ui->tabWidget->indexOf(tab));
}

void TelescopePropertiesWindow::populateDeviceModelList()
{
	//Populate the device model list
	ui->comboBoxDeviceModel->clear();
	if (!deviceModelNames.isEmpty())
	{
		ui->comboBoxDeviceModel->addItems(deviceModelNames);
	}
}

void TelescopePropertiesWindow::populateIndiDeviceModelList()
{
	ui->comboBoxIndiDeviceModel->clear();
	QTreeView* view = new QTreeView(ui->comboBoxIndiDeviceModel);
	QStandardItemModel* model = deviceManager->getIndiDeviceModels();
	view->setUniformRowHeights(true);
	view->setModel(model);
	view->setSelectionBehavior(QTreeView::SelectRows);
	view->setSelectionMode(QTreeView::SingleSelection);
	QModelIndex rootIndex = model->indexFromItem(model->invisibleRootItem());
	for (int i = 0; i < model->rowCount(); i++)
	{
		if (!model->item(i)->isSelectable())
			view->setFirstColumnSpanned(i, rootIndex, true);
	}
	view->setAllColumnsShowFocus(true);
	view->header()->setResizeMode(0, QHeaderView::ResizeToContents);
	
	ui->comboBoxIndiDeviceModel->setModel(model);
	ui->comboBoxIndiDeviceModel->setView(view);
	ui->comboBoxIndiDeviceModel->setCurrentIndex(-1);
}

void TelescopePropertiesWindow::populateShortcutNumberList()
{
	ui->comboBoxShortcutNumber->clear();
	ui->comboBoxShortcutNumber->addItem("None", 0);
	QList<int> usedShortcutNumbers = deviceManager->listUsedShortcutNumbers();
	for (int i = 1; i < 10; i++)
	{
		if (!usedShortcutNumbers.contains(i))
		{
			ui->comboBoxShortcutNumber->addItem(QString::number(i), i);
		}
	}
	if (configuredConnectionInterface == ConnectionIndi)
		ui->comboBoxShortcutNumber->setCurrentIndex(0);
	else if (ui->comboBoxShortcutNumber->count() > 1)
		ui->comboBoxShortcutNumber->setCurrentIndex(1);
	else
		ui->comboBoxShortcutNumber->setCurrentIndex(0);
}

void TelescopePropertiesWindow::showConnectionTab(bool show)
{
	if (show)
		showTab(ui->tabConnection, "Connection");
	else
		hideTab(ui->tabConnection);
}

void TelescopePropertiesWindow::showAscomTab(bool show)
{
	if (show)
	{
#ifdef Q_OS_WIN32
		showTab(ui->tabAscom, "ASCOM");
#endif
	}
	else
		hideTab(ui->tabAscom);
}

void TelescopePropertiesWindow::showIndiTab(bool show)
{
	if (show)
		showTab(ui->tabIndi, "INDI");
	else
		hideTab(ui->tabIndi);
}

void TelescopePropertiesWindow::showSerialTab(bool show)
{
	if (show)
		showTab(ui->tabSerial, "Serial");
	else
		hideTab(ui->tabSerial);
}

void TelescopePropertiesWindow::showNetworkTab(bool show)
{
	if (show)
		showTab(ui->tabNetwork, "Network");
	else
		hideTab(ui->tabNetwork);
}

#ifdef Q_OS_WIN32
void TelescopePropertiesWindow::showAscomSelector()
{
	if (!deviceManager->canUseAscom())
		return;

	QAxObject ascomChooser(this);
	if (!ascomChooser.setControl("ASCOM.Utilities.Chooser"))
	{
		emit changesDiscarded();
		return;
	}
	connect(&ascomChooser, SIGNAL(exception(int,QString,QString,QString)),
	        this, SLOT(handleAscomException(int,QString,QString,QString)));

	//TODO: Switch to windowed mode
	ascomDriverObjectId = ascomChooser.dynamicCall("Choose(QString)", ascomDriverObjectId).toString();
	ui->lineEditAscomControlId->setText(ascomDriverObjectId);
}

void TelescopePropertiesWindow::showAscomDeviceSetup()
{
	if (!deviceManager->canUseAscom() || ascomDriverObjectId.isEmpty())
		return;

	QAxObject ascomDriver(this);
	if (!ascomDriver.setControl(ascomDriverObjectId))
	{
		ascomDriverObjectId.clear();
		return;
	}
	connect(&ascomDriver, SIGNAL(exception(int,QString,QString,QString)),
	        this, SLOT(handleAscomException(int,QString,QString,QString)));
	ascomDriver.dynamicCall("SetupDialog()");
}

void TelescopePropertiesWindow::handleAscomException(int code,
                                                     const QString &source,
                                                     const QString &desc,
                                                     const QString &help)
{
	Q_UNUSED(help);
	QString errorMessage = QString("%1: ASCOM driver error:\n"
	                               "Code: %2\n"
	                               "Source: %3\n"
	                               "Description: %4")
	                               .arg(configuredId)
	                               .arg(code).arg(source).arg(desc);
	qDebug() << errorMessage << flush;
}
#endif
