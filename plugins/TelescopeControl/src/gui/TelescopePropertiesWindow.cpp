/*
 * Stellarium TelescopeControl Plug-in
 * 
 * Copyright (C) 2009-2010 Bogdan Marinov (this file)
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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

#include <QDebug>
#include <QCompleter>
#include <QFrame>
#include <QTimer>

TelescopePropertiesWindow::TelescopePropertiesWindow()
{
	ui = new Ui_widgetTelescopeProperties;
	
	telescopeManager = GETSTELMODULE(TelescopeControl);

	telescopeNameValidator = new QRegExpValidator (QRegExp("[^:\"]+"), this);//Test the update for JSON
	hostNameValidator = new QRegExpValidator (QRegExp("[a-zA-Z0-9\\-\\.]+"), this);//TODO: Write a proper host/IP regexp?
	circleListValidator = new QRegExpValidator (QRegExp("[0-9,\\.\\s]+"), this);
	#ifdef Q_OS_WIN32
	serialPortValidator = new QRegExpValidator (QRegExp("COM[0-9]+"), this);
	#else
	serialPortValidator = new QRegExpValidator (QRegExp("/dev/.*"), this);
	#endif
}

TelescopePropertiesWindow::~TelescopePropertiesWindow()
{	
	delete ui;
	
	delete telescopeNameValidator;
	delete hostNameValidator;
	delete circleListValidator;
	delete serialPortValidator;
}

void TelescopePropertiesWindow::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

// Initialize the dialog widgets and connect the signals/slots
void TelescopePropertiesWindow::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Inherited connect
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(buttonDiscardPressed()));
	connect(dialog, SIGNAL(rejected()), this, SLOT(buttonDiscardPressed()));
	
	//Connect: sender, signal, receiver, member
	connect(ui->radioButtonTelescopeLocalNative, SIGNAL(toggled(bool)), this, SLOT(toggleTypeLocalNative(bool)));
	connect(ui->radioButtonTelescopeConnection, SIGNAL(toggled(bool)), this, SLOT(toggleTypeConnection(bool)));
	connect(ui->radioButtonTelescopeVirtual, SIGNAL(toggled(bool)), this, SLOT(toggleTypeVirtual(bool)));
#ifdef Q_OS_WIN32
	connect(ui->radioButtonTelescopeLocalAscom, SIGNAL(toggled(bool)), this, SLOT(toggleTypeLocalAscom(bool)));
#endif
	
	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(buttonSavePressed()));
	connect(ui->pushButtonDiscard, SIGNAL(clicked()), this, SLOT(buttonDiscardPressed()));
	
	connect(ui->comboBoxDeviceModel, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(deviceModelSelected(const QString&)));

#ifdef Q_OS_WIN32
	connect(ui->pushButtonAscomSelect, SIGNAL(clicked()), this, SLOT(showAscomSelector()));
	connect(ui->pushButtonAscomDeviceSetup, SIGNAL(clicked()), this, SLOT(showAscomDeviceSetup()));
#endif
	
	//Setting validators
	ui->lineEditTelescopeName->setValidator(telescopeNameValidator);
	ui->lineEditHostName->setValidator(hostNameValidator);
	ui->lineEditCircleList->setValidator(circleListValidator);
	ui->lineEditSerialPort->setValidator(serialPortValidator);
}

//Set the configuration panel in a predictable state
void TelescopePropertiesWindow::initConfigurationDialog()
{
	//Reusing code used in both methods that call this one
	deviceModelNames = telescopeManager->getDeviceModels().keys();
	
	//Type
	ui->radioButtonTelescopeLocalNative->setEnabled(true);
	ui->groupBoxType->setVisible(true);

	ui->groupBoxVirtualTelescope->setVisible(false);
	
	//Telescope properties
	ui->lineEditTelescopeName->clear();
	ui->labelEquinox->setVisible(false);
	ui->frameEquinox->setVisible(false);
	ui->frameDelay->setVisible(true);
	ui->radioButtonJ2000->setChecked(true);
	ui->checkBoxConnectAtStartup->setChecked(false);
	
	//Device settings
	ui->groupBoxDeviceSettings->setVisible(true);
	ui->lineEditSerialPort->clear();
	ui->lineEditSerialPort->setCompleter(new QCompleter(SERIAL_PORT_NAMES, this));
	ui->lineEditSerialPort->setText(SERIAL_PORT_NAMES.value(0));
	//Populate the list of available devices
	ui->comboBoxDeviceModel->clear();
	if (!deviceModelNames.isEmpty())
	{
		deviceModelNames.sort();
		ui->comboBoxDeviceModel->addItems(deviceModelNames);
	}
	ui->comboBoxDeviceModel->setCurrentIndex(0);

	//Connection settings
	ui->groupBoxConnectionSettings->setVisible(true);
	
	//FOV circles
	ui->checkBoxCircles->setChecked(false);
	ui->lineEditCircleList->clear();

	//ASCOM GUI
#ifdef Q_OS_WIN32
	if (!telescopeManager->canUseAscom())
#endif
	{
		ui->radioButtonTelescopeLocalAscom->setVisible(false);
		ui->groupBoxAscomSettings->setVisible(false);
	}
}

void TelescopePropertiesWindow::initNewStellariumTelescope(int slot)
{
	configuredSlot = slot;
	initConfigurationDialog();
	ui->stelWindowTitle->setText("New Stellarium Telescope");
	ui->lineEditTelescopeName->setText(QString("New Telescope %1").arg(QString::number(configuredSlot)));
	
	if(deviceModelNames.isEmpty())
	{
		ui->radioButtonTelescopeLocalNative->setEnabled(false);
		ui->radioButtonTelescopeConnection->setChecked(true);
		toggleTypeConnection(true);//Not called if the button is already checked
	}
	else
	{
		ui->radioButtonTelescopeLocalNative->setEnabled(true);
		ui->radioButtonTelescopeLocalNative->setChecked(true);
		toggleTypeLocalNative(true);//Not called if the button is already checked
	}
	
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(DEFAULT_DELAY));
}

void TelescopePropertiesWindow::initNewVirtualTelescope(int slot)
{
	configuredSlot = slot;
	initConfigurationDialog();
	ui->stelWindowTitle->setText("New Simulated Telescope");
	ui->lineEditTelescopeName->setText(QString("New Telescope %1").arg(QString::number(configuredSlot)));

	if (ui->radioButtonTelescopeVirtual->isChecked())
		toggleTypeVirtual(true);
	else
		ui->radioButtonTelescopeVirtual->setChecked(true);
	ui->groupBoxType->setVisible(false);
}

#ifdef Q_OS_WIN32
void TelescopePropertiesWindow::initNewAscomTelescope(int slot)
{
	if (!telescopeManager->canUseAscom())
	{
		emit changesDiscarded();
		return;
	}

	configuredSlot = slot;
	initConfigurationDialog();
	ui->stelWindowTitle->setText("New ASCOM Telescope");
	ui->lineEditTelescopeName->setText(QString("New Telescope %1").arg(QString::number(configuredSlot)));

	if (ui->radioButtonTelescopeLocalAscom->isChecked())
		toggleTypeLocalAscom(true);
	else
		ui->radioButtonTelescopeLocalAscom->setChecked(true);
	ui->groupBoxType->setVisible(false);
}
#endif

void TelescopePropertiesWindow::initExistingTelescopeConfiguration(int slot)
{
	configuredSlot = slot;
	initConfigurationDialog();
	ui->stelWindowTitle->setText("Configure Telescope");
	
	//Read the telescope properties
	QString name;
	ConnectionType connectionType;
	QString equinox;
	QString host;
	int portTCP;
	int delay;
	bool connectAtStartup;
	QList<double> circles;
	QString deviceModelName;
	QString serialPortName;
	if(!telescopeManager->getTelescopeAtSlot(slot, connectionType, name, equinox, host, portTCP, delay, connectAtStartup, circles, deviceModelName, serialPortName))
	{
		//TODO: Add debug
		return;
	}
	
	ui->lineEditTelescopeName->setText(name);
	
	if(!deviceModelName.isEmpty())
	{
		ui->radioButtonTelescopeLocalNative->setChecked(true);
		
		ui->lineEditHostName->setText("localhost");//TODO: Remove magic word!
		
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
		//Initialize the serial port value
		ui->lineEditSerialPort->setText(serialPortName);
	}
	else if (connectionType == ConnectionRemote)
	{
		ui->radioButtonTelescopeConnection->setChecked(true);//Calls toggleTypeConnection(true)
		ui->lineEditHostName->setText(host);
	}
	else
	{
		if (ui->radioButtonTelescopeVirtual->isChecked())
			toggleTypeVirtual(true);
		else
			ui->radioButtonTelescopeVirtual->setChecked(true);
	}

	//Equinox
	if (equinox == "JNow")
		ui->radioButtonJNow->setChecked(true);
	else
		ui->radioButtonJ2000->setChecked(true);
	
	//Circles
	if(!circles.isEmpty())
	{
		ui->checkBoxCircles->setChecked(true);
		
		QStringList circleList;
		for(int i = 0; i < circles.size(); i++)
			circleList.append(QString::number(circles[i]));
		ui->lineEditCircleList->setText(circleList.join(", "));
	}
	
	//TCP port
	ui->spinBoxTCPPort->setValue(portTCP);
	
	//Delay
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(delay));//Microseconds to seconds
	
	//Connect at startup
	ui->checkBoxConnectAtStartup->setChecked(connectAtStartup);
}

void TelescopePropertiesWindow::toggleTypeLocalNative(bool isChecked)
{
	if(isChecked)
	{
		//Re-initialize values that may have been changed
		ui->comboBoxDeviceModel->setCurrentIndex(0);
		ui->lineEditSerialPort->setText(SERIAL_PORT_NAMES.value(0));
		ui->lineEditHostName->setText("localhost");
		ui->spinBoxTCPPort->setValue(DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot));

		//Enable/disable controls
		ui->labelHost->setEnabled(false);
		ui->lineEditHostName->setEnabled(false);

		ui->scrollArea->ensureWidgetVisible(ui->groupBoxTelescopeProperties);
	}
	else
	{
		ui->labelHost->setEnabled(true);
		ui->lineEditHostName->setEnabled(true);
	}
}

void TelescopePropertiesWindow::toggleTypeConnection(bool isChecked)
{
	if(isChecked)
	{
		//Re-initialize values that may have been changed
		ui->lineEditHostName->setText("localhost");
		ui->spinBoxTCPPort->setValue(DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot));

		ui->groupBoxDeviceSettings->setEnabled(false);

		ui->scrollArea->ensureWidgetVisible(ui->groupBoxTelescopeProperties);
	}
	else
	{
		ui->groupBoxDeviceSettings->setEnabled(true);
	}
}

void TelescopePropertiesWindow::toggleTypeVirtual(bool isChecked)
{
	//TODO: This really should be done in the GUI
	ui->groupBoxVirtualTelescope->setVisible(isChecked);
	ui->groupBoxDeviceSettings->setVisible(!isChecked);
	ui->groupBoxConnectionSettings->setVisible(!isChecked);
	ui->labelEquinox->setVisible(!isChecked);
	ui->frameEquinox->setVisible(!isChecked);
	ui->frameDelay->setVisible(!isChecked);

	if (isChecked)
		ui->scrollArea->ensureWidgetVisible(ui->groupBoxVirtualTelescope);
}

#ifdef Q_OS_WIN32
void TelescopePropertiesWindow::toggleTypeLocalAscom(bool isChecked)
{
	if (isChecked && !telescopeManager->canUseAscom())
		return;

	ui->groupBoxAscomSettings->setVisible(isChecked);
	ui->groupBoxDeviceSettings->setVisible(!isChecked);
	ui->groupBoxConnectionSettings->setVisible(!isChecked);
	ui->frameDelay->setVisible(!isChecked);

	if (isChecked)
		ui->scrollArea->ensureWidgetVisible(ui->groupBoxAscomSettings);
}
#endif

void TelescopePropertiesWindow::buttonSavePressed()
{
	//Main telescope properties
	QString name = ui->lineEditTelescopeName->text().trimmed();
	if(name.isEmpty())
		return;
	QString host = ui->lineEditHostName->text();
	if(host.isEmpty())//TODO: Validate host
		return;
	
	int delay = MICROSECONDS_FROM_SECONDS(ui->doubleSpinBoxTelescopeDelay->value());
	int portTCP = ui->spinBoxTCPPort->value();
	bool connectAtStartup = ui->checkBoxConnectAtStartup->isChecked();
	
	//Circles
	//TODO: This will change if there is a validator for that field
	QList<double> circles;
	QString rawCircles = ui->lineEditCircleList->text().trimmed();
	QStringList circleStrings;
	if(ui->checkBoxCircles->isChecked() && !(rawCircles.isEmpty()))
	{
		circleStrings = rawCircles.simplified().remove(' ').split(',', QString::SkipEmptyParts);
		circleStrings.removeDuplicates();
		circleStrings.sort();
		
		for(int i = 0; i < circleStrings.size(); i++)
		{
			if(i >= MAX_CIRCLE_COUNT)
				break;
			double circle = circleStrings.at(i).toDouble();
			if(circle > 0.0)
				circles.append(circle);
		}
	}

	QString equinox("J2000");
	if (ui->radioButtonJNow->isChecked())
		equinox = "JNow";
	
	//Type and server properties
	//TODO: When adding, check for success!
	ConnectionType type = ConnectionNA;
	if(ui->radioButtonTelescopeLocalNative->isChecked())
	{
		//Read the serial port
		QString serialPortName = ui->lineEditSerialPort->text();
		if(!serialPortName.startsWith(SERIAL_PORT_PREFIX))
			return;//TODO: Add more validation!
		
		type = ConnectionInternal;
		telescopeManager->addTelescopeAtSlot(configuredSlot, type, name, equinox, host, portTCP, delay, connectAtStartup, circles, ui->comboBoxDeviceModel->currentText(), serialPortName);
	}
	else if (ui->radioButtonTelescopeConnection->isChecked())
	{
		if(host == "localhost")
			type = ConnectionLocal;
		else
			type = ConnectionRemote;
		telescopeManager->addTelescopeAtSlot(configuredSlot, type, name, equinox, host, portTCP, delay, connectAtStartup, circles);
	}
	else if (ui->radioButtonTelescopeVirtual->isChecked())
	{
		type = ConnectionVirtual;
		telescopeManager->addTelescopeAtSlot(configuredSlot, type, name, equinox, QString(), portTCP, delay, connectAtStartup, circles);
	}
	
	emit changesSaved(name, type);
}

void TelescopePropertiesWindow::buttonDiscardPressed()
{
	emit changesDiscarded();
}

void TelescopePropertiesWindow::deviceModelSelected(const QString& deviceModelName)
{
	ui->labelDeviceModelDescription->setText(telescopeManager->getDeviceModels().value(deviceModelName).description);
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(telescopeManager->getDeviceModels().value(deviceModelName).defaultDelay));
}

#ifdef Q_OS_WIN32
void TelescopePropertiesWindow::showAscomSelector()
{
	if (!telescopeManager->canUseAscom())
		return;

	QAxObject ascomChooser(this);
	if (!ascomChooser.setControl("DriverHelper.Chooser"))
	{
		emit changesDiscarded();
		return;
	}

	//TODO: Switch to windowed mode
	ascomDriverObjectId = ascomChooser.dynamicCall("Choose(const QString&)", ascomDriverObjectId).toString();
	ui->lineEditAscomControlId->setText(ascomDriverObjectId);
}

void TelescopePropertiesWindow::showAscomDeviceSetup()
{
	if (!telescopeManager->canUseAscom() || ascomDriverObjectId.isEmpty())
		return;

	QAxObject ascomDriver(this);
	if (!ascomDriver.setControl(ascomDriverObjectId))
	{
		ascomDriverObjectId.clear();
		return;
	}
	ascomDriver.dynamicCall("SetupDialog()");
}
#endif
