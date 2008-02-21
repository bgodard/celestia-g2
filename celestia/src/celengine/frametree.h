// frametree.h
//
// Reference frame tree.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_FRAMETREE_H_
#define _CELENGINE_FRAMETREE_H_

#include <vector>

class Star;
class Body;
class ReferenceFrame;
class TimelinePhase;


class FrameTree
{
public:
    FrameTree(Star*);
    FrameTree(Body*);
    ~FrameTree();

    /*! Return the star that this tree is associated with; it will be
     *  NULL for frame trees associated with solar system bodies.
     */
    Star* getStar() const
    {
        return starParent;
    }

    ReferenceFrame* getDefaultReferenceFrame() const;

    void addChild(TimelinePhase* phase);
    void removeChild(TimelinePhase* phase);
    TimelinePhase* getChild(unsigned int n) const;
    unsigned int childCount() const;

    void markChanged();
    void markUpdated();
    void recomputeBoundingSphere();

    bool isRoot() const
    {
        return bodyParent == NULL;
    }

private:
    Star* starParent;
    Body* bodyParent;
    std::vector<TimelinePhase*> children;

    double boundingSphereRadius;
    bool changed;

    ReferenceFrame* defaultFrame;
};

#endif // _CELENGINE_FRAMETREE_H_
