/*
 * Copyright (C) 2012 Alexander Wolf
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

#ifndef _EXOPLANET_HPP_
#define _EXOPLANET_HPP_ 1

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>
#include <QDateTime>

#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "StelPainter.hpp"
#include "StelFader.hpp"

class StelPainter;

//! @class Exoplanet
//! A exoplanet object represents one pulsar on the sky.
//! Details about the exoplanets are passed using a QVariant which contains
//! a map of data from the json file.

class Exoplanet : public StelObject
{
	friend class Exoplanets;
public:
	//! @param id The official designation for a exoplanet, e.g. "Kepler-10 b"
	Exoplanet(const QVariantMap& map);
	~Exoplanet();

	//! Get a QVariantMap which describes the exoplanet. Could be used to
	//! create a duplicate.
	QVariantMap getMap(void);

	//! Get the type of object
	virtual QString getType(void) const
	{
		return "Exoplanet";
	}
	virtual float getSelectPriority(const StelCore* core) const;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	virtual QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const;
	virtual Vec3f getInfoColor(void) const;
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const
	{
		return XYZ;
	}
	//! Get the visual magnitude of pulsar
	virtual float getVMagnitude(const StelCore* core, bool withExtinction=false) const;
	//! Get the angular size of pulsar
	virtual double getAngularSize(const StelCore* core) const;
	//! Get the localized name of pulsar
	virtual QString getNameI18n(void) const
	{
		return designation;
	}
	//! Get the english name of pulsar
	virtual QString getEnglishName(void) const
	{
		return designation;
	}

	void update(double deltaTime);

private:
	bool initialized;

	Vec3d XYZ;                         // holds J2000 position	

	static StelTextureSP hintTexture;
	static StelTextureSP markerTexture;

	void draw(StelCore* core, StelPainter& painter);

	//! Variables for description of properties of exoplanets
	QString designation;	//! The designation of the exoplanet
	float starRA;		//! J2000 right ascension of host star
	float starDE;		//! J2000 declination of host star
	float mass;		//! Exoplanet mass
	float radius;		//! Exoplanet radius
	float period;		//! Exoplanet period
	float semiAxis;		//! Exoplanet orbit semi-axis
	float eccentricity;	//! Exoplanet orbit eccentricity
	float inclination;	//! Exoplanet orbit inclination

	LinearFader labelsFader;

};

#endif // _EXOPLANET_HPP_
