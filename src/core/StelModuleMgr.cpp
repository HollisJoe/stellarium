/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#include <config.h>

#include <QDebug>
#include <QPluginLoader>
#include <QSettings>

#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "StelModule.hpp"
#include "StelFileMgr.hpp"
#include "StelPluginInterface.hpp"
#include "StelIniParser.hpp"


#ifdef USE_STATIC_PLUGIN_VIRGO
Q_IMPORT_PLUGIN(VirGO)
#endif

#ifdef USE_STATIC_PLUGIN_HELLOSTELMODULE
Q_IMPORT_PLUGIN(HelloStelModule)
#endif

#ifdef USE_STATIC_PLUGIN_ANGLEMEASURE
Q_IMPORT_PLUGIN(AngleMeasure)
#endif

#ifdef USE_STATIC_PLUGIN_COMPASSMARKS
Q_IMPORT_PLUGIN(CompassMarks)
#endif

#ifdef USE_STATIC_PLUGIN_SATELLITES
Q_IMPORT_PLUGIN(Satellites)
#endif

#ifdef USE_STATIC_PLUGIN_TEXTUSERINTERFACE
Q_IMPORT_PLUGIN(TextUserInterface)
#endif

#ifdef USE_STATIC_PLUGIN_OCULARS
Q_IMPORT_PLUGIN(Oculars)
#endif

StelModuleMgr::StelModuleMgr() : pluginDescriptorListLoaded(false)
{
	// Initialize empty call lists for each possible actions
	callOrders[StelModule::ActionDraw]=QList<StelModule*>();
	callOrders[StelModule::ActionUpdate]=QList<StelModule*>();
	callOrders[StelModule::ActionHandleMouseClicks]=QList<StelModule*>();
	callOrders[StelModule::ActionHandleMouseMoves]=QList<StelModule*>();
	callOrders[StelModule::ActionHandleKeys]=QList<StelModule*>();
	callingListsToRegenerate = false;
}

StelModuleMgr::~StelModuleMgr()
{
}

// Regenerate calling lists if necessary
void StelModuleMgr::update()
{
	if (callingListsToRegenerate)
		generateCallingLists();
	callingListsToRegenerate = false;
}

/*************************************************************************
 Register a new StelModule to the list
*************************************************************************/
void StelModuleMgr::registerModule(StelModule* m, bool fgenerateCallingLists)
{
	QString name = m->objectName();
	if (modules.find(name) != modules.end())
	{
		qWarning() << "Module \"" << name << "\" is already loaded.";
		return;
	}
	modules.insert(name, m);
	m->setParent(this);

	if (fgenerateCallingLists)
		generateCallingLists();
}

/*************************************************************************
 Unregister and delete a StelModule.
*************************************************************************/
void StelModuleMgr::unloadModule(const QString& moduleID, bool alsoDelete)
{
	StelModule* m = getModule(moduleID);
	if (!m)
	{
		qWarning() << "Module \"" << moduleID << "\" is not loaded.";
		return;
	}
	modules.remove(moduleID);
	m->setParent(NULL);
	callingListsToRegenerate = true;
	if (alsoDelete)
	{
		m->deinit();
		m->deleteLater();
	}
}

/*************************************************************************
 Get the corresponding module or NULL if can't find it.
*************************************************************************/
StelModule* StelModuleMgr::getModule(const QString& moduleID)
{
	QMap<QString, StelModule*>::const_iterator iter = modules.find(moduleID);
	if (iter==modules.end())
	{
		qWarning() << "Warning can't find module called " << moduleID << ".";
		return NULL;
	}
	return iter.value();
}


/*************************************************************************
 Load an external plugin
*************************************************************************/
StelModule* StelModuleMgr::loadPlugin(const QString& moduleID)
{
	foreach (const PluginDescriptor& desc, getPluginsList())
	{
		if (desc.info.id==moduleID)
		{
			Q_ASSERT(desc.pluginInterface);
			StelModule* sMod = desc.pluginInterface->getStelModule();
			qDebug() << "Loaded plugin " << moduleID << ".";
			return sMod;
		}
	}
	qWarning() << "Can't find plugin called " << moduleID;
	return NULL;
}

struct StelModuleOrderComparator
{
	StelModuleOrderComparator(StelModule::StelModuleActionName aaction) : action(aaction) {;}
	bool operator()(StelModule* x, StelModule* y) {return x->getCallOrder(action)<y->getCallOrder(action);}
private:
	StelModule::StelModuleActionName action;
};


// Unload all plugins
void StelModuleMgr::unloadAllPlugins()
{
	QListIterator<PluginDescriptor> i(getPluginsList());
	i.toBack();
	while (i.hasPrevious())
	{
		const PluginDescriptor& d = i.previous();
		if (d.loadAtStartup==false)
			continue;
		unloadModule(d.info.id, true);
		qDebug() << "Unloaded plugin " << d.info.id << ".";
	}
}

void StelModuleMgr::setPluginLoadAtStartup(const QString& key, bool b)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("plugins_load_at_startup/"+key, b);
	for (QList<StelModuleMgr::PluginDescriptor>::Iterator iter=pluginDescriptorList.begin();iter!=pluginDescriptorList.end();++iter)
	{
		if (iter->info.id==key)
		{
			iter->loadAtStartup=b;
			return;
		}
	}
}

/*************************************************************************
 Generate properly sorted calling lists for each action (e,g, draw, update)
 according to modules orders dependencies
*************************************************************************/
void StelModuleMgr::generateCallingLists()
{
	QMap<StelModule::StelModuleActionName, QList<StelModule*> >::iterator mc;
	// For each actions (e.g. "draw", "update", etc..)
	for(mc=callOrders.begin();mc!=callOrders.end();++mc)
	{
		// Flush previous call orders
		mc.value().clear();
		// and init them with modules in creation order
		foreach (StelModule* m, getAllModules())
		{
			mc.value().push_back(m);
		}
		qSort(mc.value().begin(), mc.value().end(), StelModuleOrderComparator(mc.key()));
	}
}

/*************************************************************************
 Return the list of all the external module found in the modules/ directories
*************************************************************************/
QList<StelModuleMgr::PluginDescriptor> StelModuleMgr::getPluginsList()
{
	if (pluginDescriptorListLoaded	)
	{
		return pluginDescriptorList;
	}
	
	// First list all static plugins
	foreach (QObject *plugin, QPluginLoader::staticInstances())
	{
		StelPluginInterface* pluginInterface = qobject_cast<StelPluginInterface*>(plugin);
		if (pluginInterface)
		{
			StelModuleMgr::PluginDescriptor mDesc;
			mDesc.info = pluginInterface->getPluginInfo();
			mDesc.pluginInterface = pluginInterface;
			pluginDescriptorList.append(mDesc);
		}
	}
	
	// Then list dynamic libraries from the modules/ directory
	QSet<QString> moduleDirs;
	StelFileMgr& fileMan(StelApp::getInstance().getFileMgr());
	try
	{
		moduleDirs = fileMan.listContents("modules",StelFileMgr::Directory);
	}
	catch(std::runtime_error& e)
	{
		qWarning() << "ERROR while trying list list modules:" << e.what();
	}

	foreach (QString dir, moduleDirs)
	{
		QString moduleFullPath = QString("modules/") + dir + "/lib" + dir;
#ifdef WIN32
		moduleFullPath += ".dll";
#else
#ifdef MACOSX
		moduleFullPath += ".dylib";
#else
		moduleFullPath += ".so";
#endif
#endif
		try
		{
			moduleFullPath = StelApp::getInstance().getFileMgr().findFile(moduleFullPath, StelFileMgr::File);
		}
		catch (std::runtime_error& e)
		{
			qWarning() << "ERROR while locating plugin path: " << e.what();
		}

		QPluginLoader loader(moduleFullPath);
		if (!loader.load())
		{
			qWarning() << "Couldn't load the dynamic library: " << moduleFullPath << ": " << loader.errorString();
			qWarning() << "Plugin " << dir << " will not be loaded.";
			continue;
		}

		QObject* obj = loader.instance();
		if (!obj)
		{
			qWarning() << "Couldn't open the dynamic library: " << moduleFullPath << ": " << loader.errorString();
			qWarning() << "Plugin " << dir << " will not be open.";
			continue;
		}
		
		StelPluginInterface* pluginInterface = qobject_cast<StelPluginInterface *>(obj);
		if (pluginInterface)
		{
			StelModuleMgr::PluginDescriptor mDesc;
			mDesc.info = pluginInterface->getPluginInfo();
			mDesc.pluginInterface = pluginInterface;
			pluginDescriptorList.append(mDesc);
		}
	}

	QSettings* conf = StelApp::getInstance().getSettings();
	for (QList<StelModuleMgr::PluginDescriptor>::Iterator iter=pluginDescriptorList.begin();iter!=pluginDescriptorList.end();++iter)
	{
		if (!conf->contains("plugins_load_at_startup/"+iter->info.id))
			conf->setValue("plugins_load_at_startup/"+iter->info.id, false);
		iter->loadAtStartup = conf->value("plugins_load_at_startup/"+iter->info.id).toBool();
	}
	
	pluginDescriptorListLoaded = true;
	return pluginDescriptorList;
}
