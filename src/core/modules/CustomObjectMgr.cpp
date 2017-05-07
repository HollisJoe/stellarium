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

#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelMainView.hpp"
#include "CustomObjectMgr.hpp"

#include <QSettings>
#include <QKeyEvent>

CustomObjectMgr::CustomObjectMgr()
	: countMarkers(0)
{
	setObjectName("CustomObjectMgr");
	conf = StelApp::getInstance().getSettings();
	font.setPixelSize(StelApp::getInstance().getBaseFontSize());
}

CustomObjectMgr::~CustomObjectMgr()
{
	StelApp::getInstance().getStelObjectMgr().unSelect();
}

double CustomObjectMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+10.;
	if (actionName==StelModule::ActionHandleMouseClicks)
		return -11;	
	return 0;
}

void CustomObjectMgr::handleMouseClicks(class QMouseEvent* e)
{
	// Shift + LeftClick
	if (e->modifiers().testFlag(Qt::ShiftModifier) && e->button()==Qt::LeftButton && e->type()==QEvent::MouseButtonPress)
	{
		// Add custom marker
		const StelProjectorP prj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameJ2000, StelCore::RefractionAuto);

		QPoint p = StelMainView::getInstance().getMousePos(); // get screen coordinates of mouse cursor
		Vec3d mousePosition;
		float wh = prj->getViewportWidth()/2.; // get quarter of width of the screen
		float hh = prj->getViewportHeight()/2.; // get quarter of height of the screen
		float mx = p.x()-wh; // point 0 in center of the screen, axis X directed to right
		float my = p.y()-hh; // point 0 in center of the screen, axis Y directed to bottom
		// calculate position of mouse cursor via position of center of the screen (and invert axis Y)
		// If coordinates are invalid, don't draw them.
		bool coordsValid = prj->unProject(prj->getViewportPosX()+wh+mx, prj->getViewportPosY()+hh+1-my, mousePosition);
		if (coordsValid)
		{ // Nick Fedoseev patch
			Vec3d win;
			prj->project(mousePosition,win);
			float dx = prj->getViewportPosX()+wh+mx - win.v[0];
			float dy = prj->getViewportPosY()+hh+1-my - win.v[1];
			prj->unProject(prj->getViewportPosX()+wh+mx+dx, prj->getViewportPosY()+hh+1-my+dy, mousePosition);
			addCustomObject(QString("%1 %2").arg(N_("Marker")).arg(countMarkers + 1), mousePosition, true);
		}
		e->setAccepted(true);
		return;
	}

	// Shift + Alt + Right click -- Removes all custom markers
	// Changed by snowsailor 5/04/2017
	if(e->modifiers().testFlag(Qt::ShiftModifier) && e->modifiers().testFlag(Qt::AltModifier) && e->button() == Qt::RightButton && e->type() == QEvent::MouseButtonPress) {
		//Delete ALL custom markers
		removeCustomObjects();
		e->setAccepted(true);
		return;
	}
	// Shift + RightClick
	// Added by snowsailor 5/04/2017 -- Removes the closest marker within a radius specified within
	if (e->modifiers().testFlag(Qt::ShiftModifier) && e->button()==Qt::RightButton && e->type()==QEvent::MouseButtonPress) {
		//Limit the click radius to 15px in any direction
		int radiusLimit = 15;

		StelCore *core = StelApp::getInstance().getCore();
		const StelProjectorP prj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameJ2000, StelCore::RefractionAuto);

		QPoint p = StelMainView::getInstance().getMousePos(); // get screen coordinates of mouse cursor
		Vec3d mousePosition;
		float wh = prj->getViewportWidth()/2.; // get half of width of the screen
		float hh = prj->getViewportHeight()/2.; // get half of height of the screen
		float mx = p.x()-wh; // point 0 in center of the screen, axis X directed to right
		float my = p.y()-hh; // point 0 in center of the screen, axis Y directed to bottom
		// calculate position of mouse cursor via position of center of the screen (and invert axis Y)
		prj->unProject(prj->getViewportPosX()+wh+mx, prj->getViewportPosY()+hh+1-my, mousePosition);

		Vec3d winpos;
		prj->project(mousePosition, winpos);
		float xpos = winpos[0];
		float ypos = winpos[1];

		CustomObjectP closest;
		//Smallest valid radius will be at most `radiusLimit`, so radiusLimit + 10 is plenty as the default
		float smallestRad = radiusLimit + 10;
		foreach(CustomObjectP cObj, customObjects) {
			//Get the position of the custom object
			Vec3d a = cObj->getJ2000EquatorialPos(core);
			prj->project(a, winpos);
			//Distance formula to determine how close we clicked to each of the custom objects
			float dist = std::sqrt(((xpos-winpos[0])*(xpos-winpos[0])) + ((ypos-winpos[1])*(ypos-winpos[1])));
			//If the position of the object is within our click radius
			if(dist <= radiusLimit && dist < smallestRad) {
				//Update the closest object and the smallest distance.
				closest = cObj;
				smallestRad = dist;
			}
		}
		//If there was a custom object within `radiusLimit` pixels...
		if(smallestRad <= radiusLimit) {
			//Remove it and return
			removeCustomObject(closest);
			e->setAccepted(true);
			return;
		}
	}
	e->setAccepted(false);
}

void CustomObjectMgr::init()
{
	texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur2.png");

	customObjects.clear();

	setMarkersColor(StelUtils::strToVec3f(conf->value("color/custom_marker_color", "0.1,1.0,0.1").toString()));
	setMarkersSize(conf->value("gui/custom_marker_size", 5.f).toFloat());

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

void CustomObjectMgr::deinit()
{
	customObjects.clear();	
	texPointer.clear();
}

void CustomObjectMgr::addCustomObject(QString designation, Vec3d coordinates, bool isVisible)
{
	if (!designation.isEmpty())
	{
		CustomObjectP custObj(new CustomObject(designation, coordinates, isVisible));
		if (custObj->initialized)
			customObjects.append(custObj);

		if (isVisible)
			countMarkers++;
	}
}

void CustomObjectMgr::addCustomObject(QString designation, const QString &ra, const QString &dec, bool isVisible)
{
	Vec3d J2000;
	double dRa = StelUtils::getDecAngle(ra);
	double dDec = StelUtils::getDecAngle(dec);
	StelUtils::spheToRect(dRa,dDec,J2000);

	addCustomObject(designation, J2000, isVisible);
}

void CustomObjectMgr::addCustomObjectRaDec(QString designation, const QString &ra, const QString &dec, bool isVisible)
{
	Vec3d aim;
	double dRa = StelUtils::getDecAngle(ra);
	double dDec = StelUtils::getDecAngle(dec);
	StelUtils::spheToRect(dRa, dDec, aim);

	addCustomObject(designation, StelApp::getInstance().getCore()->equinoxEquToJ2000(aim, StelCore::RefractionOff), isVisible);
}

void CustomObjectMgr::addCustomObjectAltAzi(QString designation, const QString &alt, const QString &azi, bool isVisible)
{
	Vec3d aim;
	double dAlt = StelUtils::getDecAngle(alt);
	double dAzi = M_PI - StelUtils::getDecAngle(azi);

	if (StelApp::getInstance().getFlagSouthAzimuthUsage())
		dAzi -= M_PI;

	StelUtils::spheToRect(dAzi, dAlt, aim);

	addCustomObject(designation, StelApp::getInstance().getCore()->altAzToJ2000(aim, StelCore::RefractionAuto), isVisible);
}

void CustomObjectMgr::removeCustomObjects()
{
	setSelected("");
	customObjects.clear();
	//This marker count can be set to 0 because there will be no markers left and a duplicate will be impossible
	countMarkers = 0;
}

void CustomObjectMgr::removeCustomObject(CustomObjectP obj) {
	setSelected("");
	int i = 0;
	foreach(const CustomObjectP& cObj, customObjects) {
		//If we have a match for the thing we want to delete
		if(cObj && cObj == obj && cObj->initialized) {
			//Remove the value at the current index and exit loop
			customObjects.removeAt(i);
			break;
		}
		i++;
	}
	//Don't decrememnt marker count. This will prevent multiple markers from being added with the same name.
	//countMarkers -= 1;
}

void CustomObjectMgr::draw(StelCore* core)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setFont(font);

	foreach (const CustomObjectP& cObj, customObjects)
	{
		if (cObj && cObj->initialized)
			cObj->draw(core, &painter);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);

}

void CustomObjectMgr::drawPointer(StelCore* core, StelPainter& painter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("CustomObject");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!painter.getProjector()->project(pos, screenpos))
			return;

		const Vec3f& c(obj->getInfoColor());
		painter.setColor(c[0],c[1],c[2]);
		texPointer->bind();
		painter.setBlending(true);
		painter.drawSprite2dMode(screenpos[0], screenpos[1], 13.f, StelApp::getInstance().getTotalRunTime()*40.);
	}
}

QList<StelObjectP> CustomObjectMgr::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	foreach(const CustomObjectP& cObj, customObjects)
	{
		if (cObj->initialized)
		{
			equPos = cObj->XYZ;
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(cObj));
			}
		}
	}

	return result;
}

StelObjectP CustomObjectMgr::searchByName(const QString& englishName) const
{
	foreach(const CustomObjectP& cObj, customObjects)
	{
		if (cObj->getEnglishName().toUpper() == englishName.toUpper())
			return qSharedPointerCast<StelObject>(cObj);
	}

	return NULL;
}

StelObjectP CustomObjectMgr::searchByNameI18n(const QString& nameI18n) const
{
	foreach(const CustomObjectP& cObj, customObjects)
	{
		if (cObj->getNameI18n().toUpper() == nameI18n.toUpper())
			return qSharedPointerCast<StelObject>(cObj);
	}

	return NULL;
}

QStringList CustomObjectMgr::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords, bool inEnglish) const
{
	return StelObjectModule::listMatchingObjects(objPrefix, maxNbItem, useStartOfWords, inEnglish);
}

QStringList CustomObjectMgr::listAllObjects(bool inEnglish) const
{
	QStringList result;

	if (inEnglish)
	{
		foreach(const CustomObjectP& cObj, customObjects)
		{
			result << cObj->getEnglishName();
		}
	}
	else
	{
		foreach(const CustomObjectP& cObj, customObjects)
		{
			result << cObj->getNameI18n();
		}
	}
	return result;
}

void CustomObjectMgr::selectedObjectChange(StelModule::StelModuleSelectAction)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("CustomObject");
	if (!newSelected.empty())
	{
		setSelected(qSharedPointerCast<CustomObject>(newSelected[0]));
	}
	else
		setSelected("");
}

// Set selected planets by englishName
void CustomObjectMgr::setSelected(const QString& englishName)
{
	setSelected(searchByEnglishName(englishName));
}

void CustomObjectMgr::setSelected(CustomObjectP obj)
{
	if (obj && obj->getType() == "CustomObject")
		selected = obj;
	else
		selected.clear();
}

CustomObjectP CustomObjectMgr::searchByEnglishName(QString customObjectEnglishName) const
{
	foreach (const CustomObjectP& p, customObjects)
	{
		if (p->getEnglishName() == customObjectEnglishName)
			return p;
	}
	return CustomObjectP();
}

// Set/Get planets names color
void CustomObjectMgr::setMarkersColor(const Vec3f& c)
{
	CustomObject::markerColor = c;
}

const Vec3f& CustomObjectMgr::getMarkersColor(void) const
{
	return CustomObject::markerColor;
}

void CustomObjectMgr::setMarkersSize(const float size)
{
	CustomObject::markerSize = size;
}

float CustomObjectMgr::getMarkersSize() const
{
	return CustomObject::markerSize;
}
