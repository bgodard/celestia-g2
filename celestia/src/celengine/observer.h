// observer.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Because of the vastness of interstellar space, floats and doubles aren't
// sufficient when we need to represent distances to millimeter accuracy.
// BigFix is a high precision (128 bit) fixed point type used to represent
// the position of an observer in space.  However, it's not practical to use
// high-precision numbers for the positions of everything.  To get around
// this problem, object positions are stored at two different scales--light
// years for stars, and kilometers for objects within a star system.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_OBSERVER_H_
#define _CELENGINE_OBSERVER_H_

#include <celengine/frame.h>

class ObserverFrame
{
public:
    enum CoordinateSystem
    {
        Universal       = 0,
        Ecliptical      = 1,
        Equatorial      = 2,
        BodyFixed       = 3,
        PhaseLock       = 5,
        Chase           = 6,
        
        // Previous versions of PhaseLock and Chase used the
        // spin axis of the reference object as a secondary
        // vector for the coordinate system.
        PhaseLock_Old   = 100,
        Chase_Old       = 101,
        
        // ObserverLocal is not a real frame; it's an optional
        // way to specify view vectors. Eventually, there will
        // be some other way to accomplish this and ObserverLocal
        // will go away.
        ObserverLocal   = 200,
        
        Unknown         = 1000,
    };
    
    ObserverFrame();
    ObserverFrame(CoordinateSystem cs,
                  const Selection& _refObject,
                  const Selection& _targetObj = Selection());
    ObserverFrame(const ObserverFrame&);
    
    ~ObserverFrame();
    
    ObserverFrame& operator=(const ObserverFrame& f);
    
    CoordinateSystem getCoordinateSystem() const;
    Selection getRefObject() const;
    Selection getTargetObject() const;
    
    const ReferenceFrame* getFrame() const;
    
    UniversalCoord convertFromUniversal(const UniversalCoord& uc, double tjd) const;
    UniversalCoord convertToUniversal(const UniversalCoord& uc, double tjd) const;
    Quatd convertFromUniversal(const Quatd& q, double tjd) const;
    Quatd convertToUniversal(const Quatd& q, double tjd) const;
    
    static UniversalCoord convert(const ObserverFrame* fromFrame,
                                  const ObserverFrame* toFrame,
                                  const UniversalCoord& uc,
                                  double t);
    static Quatd          convert(const ObserverFrame* fromFrame,
                                  const ObserverFrame* toFrame,
                                  const Quatd& q,
                                  double t);
    
private:
    ReferenceFrame* createFrame(CoordinateSystem _coordSys,
                                const Selection& _refObject,
                                const Selection& _targetObject);
    
private:
    CoordinateSystem coordSys;
    ReferenceFrame* frame;
    Selection targetObject;
};


/*! ObserverFrame is a wrapper class for ReferenceFrame which adds some
 * annotation data. The goal is to place some restrictions on what reference
 * frame can be set for an observer. General reference frames can be
 * arbitrarily complex, with multiple levels of nesting. This makes it
 * difficult to store them in a cel:// URL or display information about
 * them for the user. The restricted set of reference frames wrapped by
 * the ObserverFrame class does not suffer from such problems.
 */
class Observer
{
public:
    Observer();
	Observer(const Observer& o);

	Observer& operator=(const Observer& o);
        
    UniversalCoord getPosition() const;
    void          setPosition(const UniversalCoord&);
    void          setPosition(const Point3d&);    
    
    Quatd         getOrientation() const;
    Quatf         getOrientationf() const;
    void          setOrientation(const Quatf&);
    void          setOrientation(const Quatd&);
    
    Vec3d         getVelocity() const;
    void          setVelocity(const Vec3d&);
    Vec3f         getAngularVelocity() const;
    void          setAngularVelocity(const Vec3f&);
    
    float          getFOV() const;
    void           setFOV(float);

    void           update(double dt, double timeScale);

    Vec3f          getPickRay(float x, float y) const;


    void orbit(const Selection&, Quatf q);
    void rotate(Quatf q);
    void changeOrbitDistance(const Selection&, float d);
    void setTargetSpeed(float s);
    float getTargetSpeed();

    Selection getTrackedObject() const;
    void setTrackedObject(const Selection&);

    const std::string& getDisplayedSurface() const;
    void setDisplayedSurface(const std::string&);
    
    uint32 getLocationFilter() const;
    void setLocationFilter(uint32);

    void gotoSelection(const Selection&,
                       double gotoTime,
                       Vec3f up,
                       ObserverFrame::CoordinateSystem upFrame);
    void gotoSelection(const Selection&,
                       double gotoTime,
                       double startInter,
                       double endInter,
                       Vec3f up,
                       ObserverFrame::CoordinateSystem upFrame);
    void gotoSelectionGC(const Selection&,
                         double gotoTime,
                         double startInter,
                         double endInter,
                         Vec3f up,
                         ObserverFrame::CoordinateSystem upFrame);
    void gotoSelection(const Selection&,
                       double gotoTime,
                       double distance,
                       Vec3f up,
                       ObserverFrame::CoordinateSystem upFrame);
    void gotoSelectionLongLat(const Selection&,
                              double gotoTime,
                              double distance,
                              float longitude, float latitude,
                              Vec3f up);
    void gotoLocation(const UniversalCoord& toPosition,
                      const Quatd& toOrientation,
                      double duration);
    void getSelectionLongLat(const Selection&,
                             double& distance,
                             double& longitude,
                             double& latitude);
    void gotoSelectionGC(const Selection& selection,
                         double gotoTime,
                         double distance,
                         Vec3f up,
                         ObserverFrame::CoordinateSystem upFrame);
    void gotoSurface(const Selection&, double duration);
    void centerSelection(const Selection&, double centerTime = 0.5);
    void centerSelectionCO(const Selection&, double centerTime = 0.5);
    void follow(const Selection&);
    void geosynchronousFollow(const Selection&);
    void phaseLock(const Selection&);
    void chase(const Selection&);
    void cancelMotion();

    void reverseOrientation();

    void setFrame(ObserverFrame::CoordinateSystem cs, const Selection& refObj, const Selection& targetObj);
    void setFrame(ObserverFrame::CoordinateSystem cs, const Selection& refObj);
    void setFrame(const ObserverFrame& f);

    const ObserverFrame* getFrame() const;

    double getArrivalTime() const;

    double getTime() const;
    void setTime(double);

    enum ObserverMode
    {
        Free                    = 0,
        Travelling              = 1,
    };
    
    ObserverMode getMode() const;
    void setMode(ObserverMode);

    enum TrajectoryType
    {
        Linear        = 0,
        GreatCircle   = 1,
        CircularOrbit = 2,          
    };

    struct JourneyParams
    {
        double duration;
        double startTime;
        UniversalCoord from;
        UniversalCoord to;
        Quatd initialOrientation;
        Quatd finalOrientation;
        double startInterpolation; // start of orientation interpolation phase [0-1]
        double endInterpolation;   // end of orientation interpolation phase [0-1]
        double expFactor;
        double accelTime;
        Quatd rotation1; // rotation on the CircularOrbit around centerObject

        Selection centerObject;
        
        TrajectoryType traj;
    };

    void gotoJourney(const JourneyParams&);
    // void setSimulation(Simulation* _sim) { sim = _sim; };

 private:
    void computeGotoParameters(const Selection& sel,
                               JourneyParams& jparams,
                               double gotoTime,
                               double startInter,
                               double endInter,
                               Vec3d offset,
                               ObserverFrame::CoordinateSystem offsetFrame,
                               Vec3f up,
                               ObserverFrame::CoordinateSystem upFrame);
    void computeGotoParametersGC(const Selection& sel,
                                 JourneyParams& jparams,
                                 double gotoTime,
                                 double startInter,
                                 double endInter,
                                 Vec3d offset,
                                 ObserverFrame::CoordinateSystem offsetFrame,
                                 Vec3f up,
                                 ObserverFrame::CoordinateSystem upFrame,
                                 const Selection& centerObj);
    void computeCenterParameters(const Selection& sel,
                                 JourneyParams& jparams,
                                 double centerTime);
    void computeCenterCOParameters(const Selection& sel,
                                   JourneyParams& jparams,
                                   double centerTime);

    void updateUniversal();
    void convertFrameCoordinates(const ObserverFrame* newFrame);
    
 private:
    double         simTime;

    // Position, orientation, and velocity in the observer's reference frame
    UniversalCoord position;
    Quatd          orientation;
    Vec3d          velocity;
    Vec3f          angularVelocity;
    
    // Position and orientation in universal coordinates, derived from the
    // equivalent quantities in the observer reference frame.
    UniversalCoord positionUniv;
    Quatd          orientationUniv;
    
    ObserverFrame* frame;
    
    double         realTime;

    double         targetSpeed;
    Vec3d          targetVelocity;
    Vec3d          initialVelocity;
    double         beginAccelTime;

    ObserverMode     observerMode;
    JourneyParams    journey;    
    Selection        trackObject;

    Quatd trackingOrientation;   // orientation prior to selecting tracking

    float fov;
    bool reverseFlag;

    uint32 locationFilter;
    std::string displayedSurface;
};

#endif // _CELENGINE_OBSERVER_H_
