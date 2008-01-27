/*
 * Stellarium
 * Copyright (C) 2008 Matthew Gates
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

#include "StelIniParser.hpp"

#include <QSettings>
#include <QString>
#include <QVariant>
#include <QStringList>
#include <QIODevice>
#include <QRegExp>

bool readStelIniFile(QIODevice &device, QSettings::SettingsMap &map)
{
	// Lazy IO - slurp it all up into memory and process from there.
	// it's a reasonable assumption, after all the map which holds the
	// keys and values exists in memory.
	qint64 s = device.size();
	char buf[s+1];
	if(device.read(buf, s)!=s)
		return false;

	// Is this the right conversion?
	QString data = QString::fromLocal8Bit(buf);

	// Split by a RE which should match any platform's line breaking rules
	QStringList lines = data.split(QRegExp("[\\n\\r]+"), QString::SkipEmptyParts);

	QString currentSection = "";
	QRegExp sectionRe("^\\[(.+)\\]$");
	QRegExp keyRe("^([^=]+)\\s*=\\s*(.+)$");

	for(int i=0; i<lines.size(); i++)
	{
		QString l = lines.at(i);
		l.replace(QRegExp("#.*$"), "");		// clean comments
		l.replace(QRegExp("^\\s+"), "");	// clean whitespace
		l.replace(QRegExp("\\s+$"), "");

		// If it's a section marker set the section variable
		if(sectionRe.exactMatch(l)) 
			currentSection = sectionRe.cap(1);
		// Otherwise only process if it macthes an re which looks like: key = value 
		else if (keyRe.exactMatch(l))
		{
			// Let REs do the work for us exatracting the key and value
			// and cleaning them up by removing whitespace
			QString k = keyRe.cap(1); QString v = keyRe.cap(2);
			v.replace(QRegExp("^\\s+"), ""); k.replace(QRegExp("\\s+$"), "");

			// keys with no section should have no leading /, so only
			// add it when there is a valid section.
			if (currentSection != "") 
				k = currentSection + "/" + k;

			// Set the map item.
			map[k] = QVariant(v);
		}
	}
	
	return true;
}

bool writeStelIniFile(QIODevice &device, const QSettings::SettingsMap &map)
{
	int maxKeyWidth = 30;
	QRegExp reKeyXt("^(.+)/(.+)$");  // for extracting keys/values

	// first go over map and find longest key length
	for(int i=0; i<map.keys().size(); i++)
	{
		QString k = map.keys().at(i);
		QString key = k;
		if (reKeyXt.exactMatch(k))
			key = reKeyXt.cap(2);
		if (key.size() > maxKeyWidth) maxKeyWidth = key.size();
	}

	// OK, this time actually write to the file - first non-section values
	QString outputLine;
	for(int i=0; i<map.keys().size(); i++)
	{
		QString k = map.keys().at(i);
		if (!reKeyXt.exactMatch(k))
		{
			// this is for those keys without a section
			outputLine = QString("%1").arg(k,0-maxKeyWidth) + " = " + map[k].toString() + "\n";
			device.write(outputLine.toLocal8Bit());
		}
	}
	
	// Now those values with sections.
	QString currentSection("");
	for(int i=0; i<map.keys().size(); i++)
	{
		QString k = map.keys().at(i);
		if (reKeyXt.exactMatch(k))
		{
			QString sec = reKeyXt.cap(1); QString key = reKeyXt.cap(2);

			// detect new sections and write section headers in file
			if (sec != currentSection)
			{
				currentSection = sec;
				
				outputLine =  "\n[" + currentSection + "]" + "\n";
				device.write(outputLine.toLocal8Bit());
			}
			outputLine = QString("%1").arg(key,0-maxKeyWidth) + " = " + map[k].toString() + "\n";
			device.write(outputLine.toLocal8Bit());
		}
	}
	return true;
}

