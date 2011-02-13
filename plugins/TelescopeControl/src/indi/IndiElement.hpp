/*
 * Qt-based INDI wire protocol client
 * 
 * Copyright (C) 2010-2011 Bogdan Marinov
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ELEMENT_HPP_
#define _ELEMENT_HPP_

#include <QByteArray>
#include <QString>

#include <limits>
const double DOUBLE_MAX = std::numeric_limits<double>::max();
const double DOUBLE_MIN = std::numeric_limits<double>::min();

#include "IndiTypes.hpp"

//In all element classes, put the required elements first.

class Element
{
public:
	Element(const QString& elementName, const QString& elementLabel = QString());

	const QString& getName() const;
	const QString& getLabel() const;

private:
	QString name;
	QString label;
};

//! Sub-property representing a single string.
class TextElement : public Element
{
public:
	TextElement(const QString& elementName,
	            const QString& initialValue,
	            const QString& label = QString());

	QString getValue() const;
	void setValue(const QString& stringValue);

private:
	QString value;
};

//! Sub-property representing a single number.
class NumberElement : public Element
{
public:
	NumberElement(const QString& elementName,
	              const double initialValue = 0.0,
	              const QString& format = "%d",
	              const double minimumValue = DOUBLE_MIN,
	              const double maximumValue = DOUBLE_MAX,
	              const double step = 0,
	              const QString& label = QString());

	NumberElement(const QString& elementName,
	              const QString& initialValue,
	              const QString& format,
	              const QString& minimalValue,
	              const QString& maximalValue,
	              const QString& step,
	              const QString& label = QString());

	double getValue() const;
	QString getFormattedValue() const;
	void setValue(const QString& stringValue);

	QString getFormatString() const;
	double getMinValue() const;
	double getMaxValue() const;
	double getStep() const;

	static double readDoubleFromString(const QString& string);

private:
	// For unit tests
	friend class TestNumberElement;

	bool isSexagesimal;
	double value;
	double maxValue;
	double minValue;
	double step;
	QString formatString;
};

//! Sub-property representing a single switch/button.
//! \todo Is there a point of this existing, or it can be replaced with a hash
//! of booleans?
class SwitchElement : public Element
{
public:
	SwitchElement(const QString& elementName,
				  const QString& initialValue,
				  const QString& label = QString());

	bool isOn();
	void setValue(const QString& stringValue);

private:
	//! State of the switch. True is "on", false is "off". :)
	bool state;
};

//! Sub-property representing a single indicator light.
class LightElement : public Element
{
public:
	LightElement(const QString& elementName,
	             const QString& initialValue,
	             const QString& label = QString());

	State getValue() const;
	//! \todo Decide what to do with the duplication with
	//! IndiClient::readStateFromString().
	//! \todo Decide how to handle wrong values. (At the moment if the parameter
	//! is not recognised it ignores it).
	void setValue(const QString& stringValue);

private:
	State state;
};

//! Sub-property representing a single BLOB (Binary Large OBject).
//! \todo Think what to make of this - it may be massive.
//! \todo Is there a point of it existing? It is not handled as the rest of the
//! elements - it has no initial value.
class BlobElement : public Element
{
public:
	//! \param intialValue is ignored.
	BlobElement(const QString& elementName,
	            const QString& initialValue,
	            const QString& label = QString());

	//! Decodes the string to a QByteArray?
	//! And saves it to disk? Emits a signal to the previewer?
	void setValue(const QString& blobSize,
	              const QString& blobFormat,
	              const QString& blobData);

	//! Example: Returns the decoded data?
	const QByteArray& getValue() const;

	//! Example
	QString getFormat() const;
	//! Example
	//! \todo Isn't int too small?
	int getSize() const;

private:
	QByteArray binaryData;
	QString format;
};

#endif//_ELEMENT_HPP_
