// command.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _COMMAND_H_
#define _COMMAND_H_

#include <iostream>
#include <celutil/color.h>
#include <celengine/execenv.h>
#include <celengine/astro.h>


class Command
{
 public:
    Command() {};
    virtual ~Command() {};
    virtual void process(ExecutionEnvironment&, double t, double dt) = 0;
    virtual double getDuration() const = 0;
};

typedef std::vector<Command*> CommandSequence;


class InstantaneousCommand : public Command
{
 public:
    InstantaneousCommand() {};
    virtual ~InstantaneousCommand() {};
    virtual double getDuration() const { return 0.0; };
    virtual void process(ExecutionEnvironment&) = 0;
    void process(ExecutionEnvironment& env, double /*t*/, double /*dt*/)
    {
        process(env);
    };
};


class TimedCommand : public Command
{
 public:
    TimedCommand(double _duration) : duration(_duration) {};
    virtual ~TimedCommand() {};
    double getDuration() const { return duration; };

 private:
    double duration;
};


class CommandWait : public TimedCommand
{
 public:
    CommandWait(double _duration);
    ~CommandWait();
    void process(ExecutionEnvironment&, double t, double dt);
};


class CommandSelect : public InstantaneousCommand
{
 public:
    CommandSelect(std::string _target);
    ~CommandSelect();
    void process(ExecutionEnvironment&);

 private:
    std::string target;
};


class CommandGoto : public InstantaneousCommand
{
 public:
    CommandGoto(double t, double dist,
                Vec3f _up, ObserverFrame::CoordinateSystem _upFrame);
    ~CommandGoto();
    void process(ExecutionEnvironment&);

 private:
    double gotoTime;
    double distance;
    Vec3f up;
    ObserverFrame::CoordinateSystem upFrame;
};


class CommandGotoLongLat : public InstantaneousCommand
{
 public:
    CommandGotoLongLat(double t,
                       double dist,
                       float _longitude, float _latitude,
                       Vec3f _up);
    ~CommandGotoLongLat();
    void process(ExecutionEnvironment&);

 private:
    double gotoTime;
    double distance;
    float longitude;
    float latitude;
    Vec3f up;
};


class CommandGotoLocation : public InstantaneousCommand
{
 public:
    CommandGotoLocation(double t,
                        const Point3d& translation, const Quatf& rotation);
    ~CommandGotoLocation();
    void process(ExecutionEnvironment&);

 private:
    double gotoTime;
    Point3d translation;
    Quatf rotation;
};

class CommandSetUrl : public InstantaneousCommand
{
 public:
    CommandSetUrl(const std::string& _url);
    void process(ExecutionEnvironment&);

 private:
    std::string url;
};


class CommandCenter : public InstantaneousCommand
{
 public:
    CommandCenter(double t);
    ~CommandCenter();
    void process(ExecutionEnvironment&);

 private:
    double centerTime;
};


class CommandFollow : public InstantaneousCommand
{
 public:
    CommandFollow();
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandSynchronous : public InstantaneousCommand
{
 public:
    CommandSynchronous();
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandLock : public InstantaneousCommand
{
 public:
    CommandLock();
    void process(ExecutionEnvironment&);

 private:
    int dummy;
};


class CommandChase : public InstantaneousCommand
{
 public:
    CommandChase();
    void process(ExecutionEnvironment&);

 private:
    int dummy;
};


class CommandTrack : public InstantaneousCommand
{
 public:
    CommandTrack();
    void process(ExecutionEnvironment&);

 private:
    int dummy;
};


class CommandSetFrame : public InstantaneousCommand
{
 public:
    CommandSetFrame(ObserverFrame::CoordinateSystem,
                    const std::string&, const std::string&);
    void process(ExecutionEnvironment&);

 private:
    ObserverFrame::CoordinateSystem coordSys;
    std::string refObjectName;
    std::string targetObjectName;
};


class CommandSetSurface : public InstantaneousCommand
{
 public:
    CommandSetSurface(const std::string&);
    void process(ExecutionEnvironment&);

 private:
    std::string surfaceName;
};


class CommandCancel : public InstantaneousCommand
{
 public:
    CommandCancel();
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandExit : public InstantaneousCommand
{
 public:
    CommandExit();
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandPrint : public InstantaneousCommand
{
 public:
    CommandPrint(std::string, int horig, int vorig, int hoff, int voff,
                 double duration);
    void process(ExecutionEnvironment&);

 private:
    std::string text;
    int hOrigin;
    int vOrigin;
    int hOffset;
    int vOffset;
    double duration;
};


class CommandClearScreen : public InstantaneousCommand
{
 public:
    CommandClearScreen();
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandSetTime : public InstantaneousCommand
{
 public:
    CommandSetTime(double _jd);
    void process(ExecutionEnvironment&);

 private:
    double jd;
};


class CommandSetTimeRate : public InstantaneousCommand
{
 public:
    CommandSetTimeRate(double);
    void process(ExecutionEnvironment&);

 private:
    double rate;
};


class CommandChangeDistance : public TimedCommand
{
 public:
    CommandChangeDistance(double duration, double rate);
    void process(ExecutionEnvironment&, double t, double dt);

 private:
    double rate;
};


class CommandOrbit : public TimedCommand
{
 public:
    CommandOrbit(double _duration, const Vec3f& axis, float rate);
    void process(ExecutionEnvironment&, double t, double dt);

 private:
    Vec3f spin;
};


class CommandRotate : public TimedCommand
{
 public:
    CommandRotate(double _duration, const Vec3f& axis, float rate);
    void process(ExecutionEnvironment&, double t, double dt);

 private:
    Vec3f spin;
};


class CommandMove : public TimedCommand
{
 public:
    CommandMove(double _duration, const Vec3d& _velocity);
    void process(ExecutionEnvironment&, double t, double dt);

 private:
    Vec3d velocity;
};


class CommandSetPosition : public InstantaneousCommand
{
 public:
    CommandSetPosition(const UniversalCoord&);
    void process(ExecutionEnvironment&);

 private:
    UniversalCoord pos;
};


class CommandSetOrientation : public InstantaneousCommand
{
 public:
    CommandSetOrientation(const Vec3f&, float);
    void process(ExecutionEnvironment&);

 private:
    Vec3f axis;
    float angle;
};

class CommandLookBack : public InstantaneousCommand
{
 public:
    CommandLookBack();
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandRenderFlags : public InstantaneousCommand
{
 public:
    CommandRenderFlags(int _setFlags, int _clearFlags);
    void process(ExecutionEnvironment&);

 private:
    int setFlags;
    int clearFlags;
};


class CommandLabels : public InstantaneousCommand
{
 public:
    CommandLabels(int _setFlags, int _clearFlags);
    void process(ExecutionEnvironment&);

 private:
    int setFlags;
    int clearFlags;
};


class CommandOrbitFlags : public InstantaneousCommand
{
 public:
    CommandOrbitFlags(int _setFlags, int _clearFlags);
    void process(ExecutionEnvironment&);

 private:
    int setFlags;
    int clearFlags;
};


class CommandSetVisibilityLimit : public InstantaneousCommand
{
 public:
    CommandSetVisibilityLimit(double);
    void process(ExecutionEnvironment&);

 private:
    double magnitude;
};

class CommandSetFaintestAutoMag45deg : public InstantaneousCommand
{
 public:
    CommandSetFaintestAutoMag45deg(double);
    void process(ExecutionEnvironment&);

 private:
    double magnitude;
};

class CommandSetAmbientLight : public InstantaneousCommand
{
 public:
    CommandSetAmbientLight(float);
    void process(ExecutionEnvironment&);

 private:
    float lightLevel;
};


class CommandSet : public InstantaneousCommand
{
 public:
    CommandSet(const std::string&, double);
    void process(ExecutionEnvironment&);

 private:
    std::string name;
    double value;
};


class CommandPreloadTextures : public InstantaneousCommand
{
 public:
    CommandPreloadTextures(const std::string&);
    void process(ExecutionEnvironment&);

 private:
    std::string name;
};


class CommandMark : public InstantaneousCommand
{
 public:
    CommandMark(const std::string&, Color, float, Marker::Symbol, const std::string&);
    void process(ExecutionEnvironment&);

 private:
    std::string target;
    Color color;
    float size;
    Marker::Symbol symbol;
    std::string label;
};


class CommandUnmark : public InstantaneousCommand
{
 public:
    CommandUnmark(const std::string&);
    void process(ExecutionEnvironment&);

 private:
    std::string target;
};


class CommandUnmarkAll : public InstantaneousCommand
{
 public:
    CommandUnmarkAll();
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandCapture : public InstantaneousCommand
{
 public:
    CommandCapture(const std::string&, const std::string&);
    void process(ExecutionEnvironment&);

 private:
    std::string type;
    std::string filename;
};


class CommandRenderPath : public InstantaneousCommand
{
 public:
    CommandRenderPath(GLContext::GLRenderPath);
    void process(ExecutionEnvironment&);

 private:
    GLContext::GLRenderPath path;
};


class Execution;

class RepeatCommand : public Command
{
 public:
    RepeatCommand(CommandSequence* _body, int _repeatCount);
    ~RepeatCommand();
    void process(ExecutionEnvironment&, double t, double dt) = 0;
    double getDuration();

 private:
    CommandSequence* body;
    double bodyDuration;
    int repeatCount;
    Execution* execution;
};



#endif // _COMMAND_H_
