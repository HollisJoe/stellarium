/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _PLANET_HPP_
#define _PLANET_HPP_

#include "StelObject.hpp"
#include "StelProjector.hpp"
#include "VecMath.hpp"
#include "StelFader.hpp"
#include "StelTextureTypes.hpp"
#include "StelProjectorType.hpp"

#include <QString>

// The callback type for the external position computation function
// The last variable is the userData pointer.
typedef void (*posFuncType)(double, double*, void*);

// GZ2016: new axis functions for external computation of axis orientations for selected objects.
// The last variable is a pointer to the Planet object.
typedef void (*axisFuncType)(double, double*, void*);


typedef void (OsculatingFunctType)(double jde0,double jde,double xyz[3]);

// epoch J2000: 12 UT on 1 Jan 2000
#define J2000 2451545.0
#define ORBIT_SEGMENTS 360

class StelFont;
class StelPainter;
class StelTranslator;
class QOpenGLShaderProgram;

// Class used to store rotational elements, i.e. axis orientation for the planetary body.
// Data are read from ssystem.ini in SolarSystem.cpp::loadPlanets().
//
// GZ2016-01: This seems to be an outdated model.
// TODO: For historical reasons, some of the values are given in hours, others in earth days, making life harder. Most moons have coupled rotation, they should be auto-converted from
//       Finding a consistent scheme would be helpful. Also, IAU has planet axes in ICRF, while Stellarium has them coded in VSOP87.
//       Except if rot_pole... values are given, then they are ICRF and transformed on the fly to VSOP87, stored in here.
//
// IAU standards like the Report of the IAU Working Group on Cartographic Coordinates and Rotational elements 2009 (DOI:10.1007/s10569-010-9320-4)
// has axes given w.r.t. J2000 ICRF.
// Before 0.15, if the planet elements in ssystem.ini had elements "rot_pole_ra" and "rot_pole_de" given, the poles were transformed to
// ecliptically-based directions (VSOP87) obliquity/ascendingNode. But these were only ra/de_J2000 poles. Some axes have precession, which need to be modelled/updated.
// The new way (0.15): We still use the previous elements obliquity and ascendingNode in Planet::computeTransMatrix(.,.)
// and Planet::getSiderealTime(), but can update them periodically from:
// ra=ra0+T*ra1
// de=de0+T*de1         ( --> obliquity, ascendingNode)
// rot_rotation_offset [degrees]     =W0
// rot_periode  [hours] =  computed from rot_pole_W1[deg/day] if that exists.  360 [deg] / _rot_ [hours] --> 360 * _rot_ / 24 [deg/hours]
// In addition, the objects with more complicated element behaviour must be updated with special functions in those two functions.
// New keys in ssystem.ini, their storage in RotationElements, and their equivalents in the IAU report:
// rot_pole_ra  [degrees]     re.ra0      constant term for alpha_0
// rot_pole_de  [degrees]     re.ra1      constant term for delta_0
// rot_pole_ra1 [degrees/cy]  re.de0      T factor for alpha_0
// rot_pole_de1 [degrees/cy]  re.de1      T factor for  delta_0
// rot_pole_W0  [degrees]     re.offset   constant term fo W. Will be stored in re.offset, and is signal for using the IAU model (causes setting re.useICRF=true)
// rot_pole_W1  [degrees/day] re.period   d factor for W      Will be converted to rot_periode=360*24/W1.

class RotationElements
{
public:
	RotationElements(void) : period(1.), offset(0.), epoch(J2000), obliquity(0.), ascendingNode(0.), //precessionRate(0.),
		siderealPeriod(0.),
		useICRF(false), ra0(0.), ra1(0.), de0(0.), de1(0.) {}
	float period;          // (sidereal) rotation period [earth days]     If useICRF, this is from the time term of W.
	float offset;          // rotation at epoch  [degrees]                If useICRF, this is the constant term of W
	double epoch;          // JDE (JD TT) of epoch for these elements
	float obliquity;       // tilt of rotation axis w.r.t. ecliptic [radians]
	float ascendingNode;   // long. of ascending node of equator on the ecliptic [radians]
//	float precessionRate;  // rate of precession of rotation axis in [rads/JulianCentury(36525d)] [ NO LONGER USED WITH 0.14 (was used for Earth only, and that was too simple.) ]
	double siderealPeriod; // sidereal period (Planet year in earth days) [earth days]
	// GZ for 0.15: I propose changes here: The 4 new entries after the switch are enough for many objects. Else, design special_functions.
	bool useICRF;          // Use values w.r.t. ICRF (should ultimately be true for all objects!) This can be set when rot_pole_ra is given. Updating the axis is required if ra1<>0
	double ra0;            // [rad] RA_0 right ascension of north pole. ssystem.ini: rot_pole_ra    /180*M_PI
	double ra1;            // [rad/century] rate of change in axis ra   ssystem.ini: rot_pole_ra1   /180*M_PI
	double de0;            // [rad] DE_0 declination of north pole      ssystem.ini: rot_pole_de    /180*M_PI
	double de1;            // [rad/century] rate of change in axis de   ssystem.ini: rot_pole_de1   /180*M_PI
//	double W0;             // [rad] mean longitude at epoch.
//	double W1;             // [rad/d] mean longitude motion.
};

// Class to manage rings for planets like saturn
class Ring
{
public:
	Ring(float radiusMin, float radiusMax,const QString &texname);
	double getSize(void) const {return radiusMax;}
	const float radiusMin;
	const float radiusMax;
	StelTextureSP tex;
};


class Planet : public StelObject
{
public:
	friend class SolarSystem;

	Q_ENUMS(PlanetType)
	Q_ENUMS(ApparentMagnitudeAlgorithm)
	//! numeric typecodes for the type descriptions in ssystem.ini
	// GZ: Until 0.13 QStrings were used for types.
	// GZ: Enums are slightly faster than string comparisons in time-critical comparisons.
	// GZ: If other types are introduced, add here and the string in init().
	enum PlanetType
	{
		isStar,                   // ssystem.ini: type="star"
		isPlanet,                 // ssystem.ini: type="planet"
		isMoon,                   // ssystem.ini: type="moon"
		isAsteroid,               // ssystem.ini: type="asteroid"
		isPlutino,                // ssystem.ini: type="plutino"
		isComet,                  // ssystem.ini: type="comet"
		isDwarfPlanet,		  // ssystem.ini: type="dwarf planet"
		isCubewano,		  // ssystem.ini: type="cubewano"
		isSDO,			  // ssystem.ini: type="scattered disc object"
		isOCO,			  // ssystem.ini: type="Oort cloud object"
		isUNDEFINED               // ssystem.ini: type=<anything else>
	};

	enum ApparentMagnitudeAlgorithm
	{
		Expl_Sup_1992,	// Algorithm provided by Pere Planesas (Observatorio Astronomico Nacional) (Was called "Planesas")
		Mueller_1893,	// G. Mueller, based on visual observations 1877-91. [Expl.Suppl.1961]
		Astr_Alm_1984,	// Astronomical Almanac 1984 and later. These give V (instrumental) magnitudes (allegedly from D.L. Harris, but this is wrong!)
		UndefinedAlgorithm,
		Generic		// Visual magnitude based on phase angle and albedo. The formula for this is totally unknown!
	};

	Planet(const QString& englishName,
	       int flagLighting,
	       double radius,
	       double oblateness,
	       Vec3f halocolor,
	       float albedo,
	       const QString& texMapName,
	       const QString& normalMapName,
	       posFuncType _coordFunc,
	       void* userDataPtr,
	       OsculatingFunctType *osculatingFunc,
	       bool closeOrbit,
	       bool hidden,
	       bool hasAtmosphere,
	       bool hasHalo,
	       const QString &pTypeStr);

	virtual ~Planet();

	//! Initializes static vars. Must be called before creating first planet.
	// Currently ensured by SolarSystem::init()
	static void init();

	///////////////////////////////////////////////////////////////////////////
	// Methods inherited from StelObject
	//! Get a string with data about the Planet.
	//! Planets support the following InfoStringGroup flags:
	//! - Name
	//! - Magnitude
	//! - RaDec
	//! - AltAzi
	//! - Distance
	//! - Size
	//! - PlainText
	//! - Extra: Heliocentric Ecliptical Coordinates & Observer-planetocentric Ecliptical Coordinates, Phase, illumination, phase angle & elongation from the Sun
	//! @param core the StelCore object
	//! @param flags a set of InfoStringGroup items to include in the return value.
	//! @return a QString containing an HMTL encoded description of the Planet.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup& flags) const;
	virtual double getCloseViewFov(const StelCore* core) const;
	virtual double getSatellitesFov(const StelCore* core) const;
	virtual double getParentSatellitesFov(const StelCore* core) const;
	virtual float getVMagnitude(const StelCore* core) const;
	virtual float getSelectPriority(const StelCore* core) const;
	virtual Vec3f getInfoColor(void) const;
	virtual QString getType(void) const {return "Planet";}
	virtual Vec3d getJ2000EquatorialPos(const StelCore *core) const;
	virtual QString getEnglishName(void) const;
	virtual QString getNameI18n(void) const;
	virtual double getAngularSize(const StelCore* core) const;
	virtual bool hasAtmosphere(void) {return atmosphere;}
	virtual bool hasHalo(void) {return halo;}

	///////////////////////////////////////////////////////////////////////////
	// Methods of SolarSystem object
	//! Translate planet name using the passed translator
	virtual void translateName(const StelTranslator &trans);

	// Draw the Planet
	// GZ Made that virtual to allow comets having their own draw().
	virtual void draw(StelCore* core, float maxMagLabels, const QFont& planetNameFont);

	///////////////////////////////////////////////////////////////////////////
	// Methods specific to Planet
	//! Get the radius of the planet in AU.
	//! @return the radius of the planet in astronomical units.
	double getRadius(void) const {return radius;}
	//! Get the value (1-f) for oblateness f.
	double getOneMinusOblateness(void) const {return oneMinusOblateness;}
	//! Get duration of sidereal day
	double getSiderealDay(void) const {return re.period;}
	//! Get duration of sidereal year
	// must be virtual for Comets.
	virtual double getSiderealPeriod(void) const { return re.siderealPeriod; }
	//! Get duration of mean solar day
	double getMeanSolarDay(void) const;

	const QString& getTextMapName() const {return texMapName;}	
	const QString getPlanetTypeString() const {return pTypeMap.value(pType);}
	PlanetType getPlanetType() const {return pType;}

	void setNativeName(QString planet) { nativeName = planet; }

	ApparentMagnitudeAlgorithm getApparentMagnitudeAlgorithm() const { return vMagAlgorithm; }
	const QString getApparentMagnitudeAlgorithmString() const { return vMagAlgorithmMap.value(vMagAlgorithm); }
	void setApparentMagnitudeAlgorithm(QString algorithm);

	//! Compute the z rotation to use from equatorial to geographic coordinates. For general applicability we need both time flavours:
	//! @param JD is JD(UT) for Earth
	//! @param JDE is used for other locations
	double getSiderealTime(double JD, double JDE) const;
	Mat4d getRotEquatorialToVsop87(void) const;
	void setRotEquatorialToVsop87(const Mat4d &m);

	const RotationElements &getRotationElements(void) const {return re;}
	// Set the -o-r-b-i-t-a-l- ROTATIONAL elements
	// _period: duration of sidereal rotation [Julian days]
	// _offset: [angle at _epoch. ]
	// _epoch: [JDE]
	// _obliquity [rad]
	// _ascendingNode of equator on ecliptic[rad]
	// ra_pole=_ra0 + T*_ra1. ra_pole and de_pole must be computed more than for initialisation for J2000
	// de_pole=_de0 + T*_de1
	// _precessionRate [rad/JulCt] (was only given for earth, and is no longer used!)
	// _siderealPeriod [earth days] orbital duration. THIS DOES NOT BELONG HERE!
	void setRotationElements(const float _period, const float _offset, const double _epoch,
				 const float _obliquity, const float _ascendingNode,
				 const double _ra0,
				 const double _ra1,
				 const double _de0,
				 const double _de1,
				 //float _precessionRate,
				 const double _siderealPeriod);
	double getRotAscendingnode(void) const {return re.ascendingNode;}
	// return angle between axis and normal of ecliptic plane (or, for a moon, equatorial/reference plane defined by parent).
	// For Earth, this is the angle between axis and normal to current ecliptic of date, i.e. the ecliptic obliquity of date JDE.
	// TODO: decide if this is always angle between axis and J2000 ecliptic, or should be axis//current ecliptic!
	double getRotObliquity(double JDE) const;


	// Compute the position in the parent Planet coordinate system
	void computePositionWithoutOrbits(const double dateJDE);
	void computePosition(const double dateJDE);

	// Compute the transformation matrix from the local Planet coordinate to the parent Planet coordinate.
	// This requires both flavours of JD in cases involving Earth.
	void computeTransMatrix(double JD, double JDE);

	// Get the phase angle (rad) for an observer at pos obsPos in heliocentric coordinates (in AU)
	double getPhaseAngle(const Vec3d& obsPos) const;
	// Get the elongation angle (rad) for an observer at pos obsPos in heliocentric coordinates (in AU)
	double getElongation(const Vec3d& obsPos) const;
	// Get the angular size of the spheroid of the planet (i.e. without the rings)
	double getSpheroidAngularSize(const StelCore* core) const;
	// Get the planet phase for an observer at pos obsPos in heliocentric coordinates (in AU)
	float getPhase(const Vec3d& obsPos) const;

	//! Get the Planet position in the parent Planet ecliptic coordinate in AU
	Vec3d getEclipticPos() const;

	// Return the heliocentric ecliptical position
	Vec3d getHeliocentricEclipticPos() const;

	// Return the heliocentric transformation for local coordinate
	Vec3d getHeliocentricPos(Vec3d) const;
	void setHeliocentricEclipticPos(const Vec3d &pos);

	// Compute the distance to the given position in heliocentric coordinate (in AU)
	double computeDistance(const Vec3d& obsHelioPos);
	double getDistance(void) const {return distance;}

	void setRings(Ring* r) {rings = r;}

	void setSphereScale(float s) {sphereScale = s;}
	float getSphereScale(void) const {return sphereScale;}

	const QSharedPointer<Planet> getParent(void) const {return parent;}

	static void setLabelColor(const Vec3f& lc) {labelColor = lc;}
	static const Vec3f& getLabelColor(void) {return labelColor;}

	// update displayed elements. @param deltaTime: ms (since last call)
	virtual void update(int deltaTime);

	void setFlagHints(bool b){hintFader = b;}
	bool getFlagHints(void) const {return hintFader;}

	void setFlagLabels(bool b){flagLabels = b;}
	bool getFlagLabels(void) const {return flagLabels;}

	bool flagNativeName;
	void setFlagNativeName(bool b) { flagNativeName = b; }
	bool getFlagNativeName(void) { return flagNativeName; }

	bool flagTranslatedName;
	void setFlagTranslatedName(bool b) { flagTranslatedName = b; }
	bool getFlagTranslatedName(void) { return flagTranslatedName; }

	///////////////////////////////////////////////////////////////////////////
	// DEPRECATED
	///// Orbit related code
	// Should move to an OrbitPath class which works on a SolarSystemObject, not a Planet
	void setFlagOrbits(bool b){orbitFader = b;}
	bool getFlagOrbits(void) const {return orbitFader;}
	LinearFader orbitFader;
	// draw orbital path of Planet
	void drawOrbit(const StelCore*);
	Vec3d orbit[ORBIT_SEGMENTS+1];   // store heliocentric coordinates for drawing the orbit
	Vec3d orbitP[ORBIT_SEGMENTS+1];  // store local coordinate for orbit
	double lastOrbitJDE;
	double deltaJDE;                 // time difference between positional updates.
	double deltaOrbitJDE;
	bool orbitCached;                // whether orbit calculations are cached for drawing orbit yet
	bool closeOrbit;                 // whether to connect the beginning of the orbit line to
					 // the end: good for elliptical orbits, bad for parabolic
					 // and hyperbolic orbits

	static Vec3f orbitColor;
	static void setOrbitColor(const Vec3f& oc) {orbitColor = oc;}
	static const Vec3f& getOrbitColor() {return orbitColor;}

	static bool permanentDrawingOrbits;

	//! Return the list of planets which project some shadow on this planet
	QVector<const Planet*> getCandidatesForShadow() const;
	
protected:
	static StelTextureSP texEarthShadow;     // for lunar eclipses

	void computeModelMatrix(Mat4d &result) const;
	
	// Return the information string "ready to print" :)
	QString getSkyLabel(const StelCore* core) const;

	// Draw the 3d model. Call the proper functions if there are rings etc..
	void draw3dModel(StelCore* core, StelProjector::ModelViewTranformP transfo, float screenSz, bool drawOnlyRing=false);

	// Draw the 3D sphere
	void drawSphere(StelPainter* painter, float screenSz, bool drawOnlyRing=false);

	// Draw the circle and name of the Planet
	void drawHints(const StelCore* core, const QFont& planetNameFont);

	QString englishName;             // english planet name
	QString nameI18;                 // International translated name
	QString nativeName;              // Can be used in a skyculture
	QString texMapName;              // Texture file path
	QString normalMapName;              // Texture file path
	int flagLighting;                // Set whether light computation has to be proceed
	RotationElements re;             // Rotation param
	double radius;                   // Planet radius in AU
	double oneMinusOblateness;       // (polar radius)/(equatorial radius)
	Vec3d eclipticPos;               // Position in AU in the rectangular ecliptic coordinate system, (GZ2016: presumably equinox J2000)
					 // centered on the parent Planet
	Vec3d screenPos;                 // Used to store temporarily the 2D position on screen
	Vec3d previousScreenPos;         // The position of this planet in the previous frame.
	Vec3f haloColor;                 // exclusively used for drawing the planet halo

	float albedo;                    // Planet albedo. Used for magnitude computation (but formula dubious!)
	Mat4d rotLocalToParent;          // GZ2015: was undocumented.
					 // Apparently this is the axis orientation with respect to the parent body. For planets, this is axis orientation w.r.t. VSOP87A/J2000 ecliptical system.
	float axisRotation;              // Rotation angle of the Planet on its axis, degrees.
					 // For Earth, this should be Greenwich Mean Sidereal Time GMST.
					 // For V0.15+, and for planets computed after the IAU2009 paper this is angle W (rotDeg),
					 // i.e. angle between ascending node of body equator w.r.t. ICRF equator and its prime meridian.
	StelTextureSP texMap;            // Planet map texture
	StelTextureSP normalMap;         // Planet normal map texture

	Ring* rings;                     // Planet rings
	double distance;                 // Temporary variable used to store the distance to a given point
					 // it is used for sorting while drawing
	float sphereScale;               // Artificial scaling for better viewing
	double lastJDE;                  // caches JDE of last positional computation
	// The callback for the calculation of the equatorial rect heliocentric position at time JDE.
	posFuncType coordFunc;
	void* userDataPtr;               // this is always used with an Orbit object.

	OsculatingFunctType *const osculatingFunc;
	QSharedPointer<Planet> parent;           // Planet parent i.e. sun for earth
	QList<QSharedPointer<Planet> > satellites;      // satellites of the Planet
	LinearFader hintFader;
	LinearFader labelsFader;         // Store the current state of the label for this planet
	bool flagLabels;                 // Define whether labels should be displayed
	bool hidden;                     // useful for fake planets used as observation positions - not drawn or labeled
	bool atmosphere;                 // Does the planet have an atmosphere?
	bool halo;                       // Does the planet have a halo?	
	PlanetType pType;                // Type of body

	ApparentMagnitudeAlgorithm vMagAlgorithm;

	static Vec3f labelColor;
	static StelTextureSP hintCircleTex;	
	static QMap<PlanetType, QString> pTypeMap; // Maps fast type to english name.
	static QMap<ApparentMagnitudeAlgorithm, QString> vMagAlgorithmMap;
	
	// Shader-related variables
	struct PlanetShaderVars {
		int projectionMatrix;
		int texCoord;
		int unprojectedVertex;
		int vertex;
		int texture;
		int lightDirection;
		int eyeDirection;
		int diffuseLight;
		int ambientLight;
		int shadowCount;
		int shadowData;
		int sunInfo;
		int skyBrightness;
		
		void initLocations(QOpenGLShaderProgram*);
	};
	static PlanetShaderVars planetShaderVars;
	static QOpenGLShaderProgram* planetShaderProgram;

	// Shader-related variables
	struct RingPlanetShaderVars : public PlanetShaderVars {
		// Rings-specific variables
		int isRing;
		int ring;
		int outerRadius;
		int innerRadius;
		int ringS;
	};
	static RingPlanetShaderVars ringPlanetShaderVars;
	static QOpenGLShaderProgram* ringPlanetShaderProgram;
	
	struct MoonShaderVars : public PlanetShaderVars {
		// Moon-specific variables
		int earthShadow;
		int normalMap;
	};
	static MoonShaderVars moonShaderVars;
	static QOpenGLShaderProgram* moonShaderProgram;
	
	static void initShader();
	static void deinitShader();
};

#endif // _PLANET_HPP_

