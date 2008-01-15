// qtsolarsystembrowser.cpp
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Solar system browser widget for Qt4 front-end
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "qtsolarsystembrowser.h"
#include "qtselectionpopup.h"
#include <QAbstractItemModel>
#include <QTreeView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <celestia/celestiacore.h>
#include <vector>

using namespace std;


class SolarSystemTreeModel : public QAbstractTableModel
{
public:
    SolarSystemTreeModel(const Universe* _universe);
    virtual ~SolarSystemTreeModel();

    Selection objectAtIndex(const QModelIndex& index) const;

    // Methods from QAbstractTableModel
    QModelIndex index(int row, int column, const QModelIndex& parent) const;
    QModelIndex parent(const QModelIndex& index) const;

    Qt::ItemFlags flags(const QModelIndex& index) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex& index) const;
    int columnCount(const QModelIndex& index) const;
    void sort(int column, Qt::SortOrder order);

    enum
    {
        NameColumn        = 0,
        TypeColumn        = 1,
    };

    void buildModel(Star* star, bool _groupByClass);

private:
    class TreeItem
    {
    public:
        TreeItem();
        ~TreeItem();

        Selection obj;
        TreeItem* parent;
        TreeItem** children;
        int nChildren;
        int childIndex;
        int classification;
    };

    TreeItem* createTreeItem(Selection sel, TreeItem* parent, int childIndex);
    void addTreeItemChildren(TreeItem* item,
                             PlanetarySystem* sys,
                             const vector<Star*>* orbitingStars);
    void addTreeItemChildrenGrouped(TreeItem* item,
                                    PlanetarySystem* sys,
                                    const vector<Star*>* orbitingStars,
                                    Selection parent);
    TreeItem* createGroupTreeItem(int classification,
                                  const vector<Body*>& objects,
                                  TreeItem* parent,
                                  int childIndex);

    TreeItem* itemAtIndex(const QModelIndex& index) const;

private:
    const Universe* universe;
    TreeItem* rootItem;
    bool groupByClass;
};


SolarSystemTreeModel::TreeItem::TreeItem() :
    parent(NULL),
    children(NULL),
    nChildren(0),
    classification(0)
{
}


SolarSystemTreeModel::TreeItem::~TreeItem()
{
    if (children != NULL)
    {
        for (int i = 0; i < nChildren; i++)
            delete children[i];
        delete[] children;
    }
}


SolarSystemTreeModel::SolarSystemTreeModel(const Universe* _universe) :
    universe(_universe),
    rootItem(NULL),
    groupByClass(false)
{
    // Initialize an empty model
    buildModel(NULL, false);
}


SolarSystemTreeModel::~SolarSystemTreeModel()
{
}


// Call createTreeItem() to build the parallel tree structure
void SolarSystemTreeModel::buildModel(Star* star, bool _groupByClass)
{
    groupByClass = _groupByClass;

    if (rootItem != NULL)
        delete rootItem;

    rootItem = new TreeItem();
    rootItem->obj = Selection();

    if (star != NULL)
    {
        rootItem->nChildren = 1;
        rootItem->children = new TreeItem*[1];
        rootItem->children[0] = createTreeItem(Selection(star), rootItem, 0);
    }

    reset();
}


// Rather than directly use Celestia's solar system data structure for
// the tree model, we'll build a parallel structure out of TreeItems.
// The additional memory used for this structure is negligible, and
// it gives us some freedom to structure the tree in a different
// way than it's represented internally, e.g. to group objects by
// their classification. It also simplifies the code because stars
// and solar system bodies can be treated almost identically once
// the new tree is built.
SolarSystemTreeModel::TreeItem*
SolarSystemTreeModel::createTreeItem(Selection sel,
                                     TreeItem* parent,
                                     int childIndex)
{
    TreeItem* item = new TreeItem();
    item->parent = parent;
    item->obj = sel;
    item->childIndex = childIndex;

    const vector<Star*>* orbitingStars = NULL;

    PlanetarySystem* sys = NULL;
    if (sel.body() != NULL)
    {
        sys = sel.body()->getSatellites();
    }
    else if (sel.star() != NULL)
    {
        // Stars may have both a solar system and other stars orbiting
        // them.
        SolarSystemCatalog* solarSystems = universe->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter = solarSystems->find(sel.star()->getCatalogNumber());
        if (iter != solarSystems->end())
        {
            sys = iter->second->getPlanets();
        }

        orbitingStars = sel.star()->getOrbitingStars();
    }

    if (groupByClass && sys != NULL)
        addTreeItemChildrenGrouped(item, sys, orbitingStars, sel);
    else
        addTreeItemChildren(item, sys, orbitingStars);

    return item;
}


void
SolarSystemTreeModel::addTreeItemChildren(TreeItem* item,
                                          PlanetarySystem* sys,
                                          const vector<Star*>* orbitingStars)
{
    // Calculate the number of children: the number of orbiting stars plus
    // the number of orbiting solar system bodies.
    item->nChildren = 0;
    if (orbitingStars != NULL)
        item->nChildren += orbitingStars->size();
    if (sys != NULL)
        item->nChildren += sys->getSystemSize();
    
    if (item->nChildren != 0)
    {
        int childIndex = 0;
        item->children = new TreeItem*[item->nChildren];

        // Add the stars
        if (orbitingStars != NULL)
        {
            for (unsigned int i = 0; i < orbitingStars->size(); i++)
            {
                Selection child(orbitingStars->at(i));
                item->children[i] = createTreeItem(child, item, childIndex);
                childIndex++;
            }
        }

        // Add the solar system bodies
        if (sys != NULL)
        {
            for (int i = 0; i < sys->getSystemSize(); i++)
            {
                Selection child(sys->getBody(i));
                item->children[i] = createTreeItem(child, item, childIndex);
                childIndex++;
            }
        }
    }
}


// Add children to item, but group objects of certain classes
// into subtrees to avoid clutter. Stars, planets, and moons
// are shown as direct children of the parent. Small moons,
// asteroids, and spacecraft a grouped together, as there tend to be
// large collections of such objects.
void
SolarSystemTreeModel::addTreeItemChildrenGrouped(TreeItem* item,
                                                 PlanetarySystem* sys,
                                                 const vector<Star*>* orbitingStars,
                                                 Selection parent)
{
    vector<Body*> asteroids;
    vector<Body*> spacecraft;
    vector<Body*> minorMoons;
    vector<Body*> other;
    vector<Body*> normal;

    float minorMoonCutoffRadius = 0.0f;
    bool groupAsteroids = true;
    bool groupSpacecraft = true;
    if (parent.body())
    {
        // TODO: we should define a class for minor moons. Until then,
        // we'll call a moon 'minor' if its radius is less than 1/1000
        // times the radius of the planet it orbits.
        minorMoonCutoffRadius = parent.body()->getRadius() / 1000.0f;

        // Don't put asteroid moons in the asteroid group; make them
        // immediate children of the parent.
        if (parent.body()->getClassification() == Body::Asteroid)
            groupAsteroids = false;

        if (parent.body()->getClassification() == Body::Spacecraft)
            groupSpacecraft = false;
    }

    for (int i = 0; i < sys->getSystemSize(); i++)
    {
        Body* body = sys->getBody(i);
        switch (body->getClassification())
        {
        case Body::Planet:
        case Body::Invisible:
            normal.push_back(body);
            break;
        case Body::Moon:
            if (body->getRadius() < minorMoonCutoffRadius)
                minorMoons.push_back(body);
            else
                normal.push_back(body);
            break;
        case Body::Asteroid:
        case Body::Comet:
            if (groupAsteroids)
                asteroids.push_back(body);
            else
                normal.push_back(body);
            break;
        case Body::Spacecraft:
            if (groupSpacecraft)
                spacecraft.push_back(body);
            else
                normal.push_back(body);
            break;
        default:
            other.push_back(body);
            break;
        }
    }

    // Calculate the total number of children
    item->nChildren = 0;
    if (orbitingStars != NULL)
        item->nChildren += orbitingStars->size();

    item->nChildren += normal.size();
    if (!asteroids.empty())
        item->nChildren++;
    if (!spacecraft.empty())
        item->nChildren++;
    if (!minorMoons.empty())
        item->nChildren++;
    if (!other.empty())
        item->nChildren++;

    if (item->nChildren != 0)
    {
        int childIndex = 0;
        item->children = new TreeItem*[item->nChildren];
        {
            // Add the stars
            if (orbitingStars != NULL)
            {
                for (unsigned int i = 0; i < orbitingStars->size(); i++)
                {
                    Selection child(orbitingStars->at(i));
                    item->children[i] = createTreeItem(child, item, childIndex);
                    childIndex++;
                }
            }

            // Add the direct children
            for (int i = 0; i < (int) normal.size(); i++)
            {
                item->children[childIndex] = createTreeItem(normal[i], item, childIndex);
                childIndex++;
            }

            // Add the groups
            if (!minorMoons.empty())
            {
                item->children[childIndex] = createGroupTreeItem(Body::SmallBody,
                                                                 minorMoons,
                                                                 item, childIndex);
                childIndex++;
            }

            if (!asteroids.empty())
            {
                item->children[childIndex] = createGroupTreeItem(Body::Asteroid,
                                                                 asteroids,
                                                                 item, childIndex);
                childIndex++;
            }

            if (!spacecraft.empty())
            {
                item->children[childIndex] = createGroupTreeItem(Body::Spacecraft,
                                                                 spacecraft,
                                                                 item, childIndex);
                childIndex++;
            }

            if (!other.empty())
            {
                item->children[childIndex] = createGroupTreeItem(Body::Unknown,
                                                                 other,
                                                                 item, childIndex);
                childIndex++;
            }
        }
    }
}


SolarSystemTreeModel::TreeItem*
SolarSystemTreeModel::createGroupTreeItem(int classification,
                                          const vector<Body*>& objects,
                                          TreeItem* parent,
                                          int childIndex)
{
    TreeItem* item = new TreeItem();
    item->parent = parent;
    item->childIndex = childIndex;
    item->classification = classification;

    if (!objects.empty())
    {
        item->nChildren = (int) objects.size();
        item->children = new TreeItem*[item->nChildren];
        for (int i = 0; i < item->nChildren; i++)
        {
            item->children[i] = createTreeItem(Selection(objects[i]), item, i);
        }
    }

    return item;
}


SolarSystemTreeModel::TreeItem*
SolarSystemTreeModel::itemAtIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return rootItem;
    else
        return static_cast<TreeItem*>(index.internalPointer());
}


Selection
SolarSystemTreeModel::objectAtIndex(const QModelIndex& index) const
{
    return itemAtIndex(index)->obj;
}


/****** Virtual methods from QAbstractTableModel *******/

// Override QAbstractTableModel::index()
QModelIndex SolarSystemTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem* parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    if (row < parentItem->nChildren)
        return createIndex(row, column, parentItem->children[row]);
    else
        return QModelIndex();
}


// Override QAbstractTableModel::parent()
QModelIndex SolarSystemTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem* child = static_cast<TreeItem*>(index.internalPointer());

    if (child->parent == rootItem)
        return QModelIndex();
    else
        return createIndex(child->parent->childIndex, 0, child->parent);
}


// Override QAbstractTableModel::flags()
Qt::ItemFlags SolarSystemTreeModel::flags(const QModelIndex&) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}


static QString objectTypeName(const Selection& sel)
{
    if (sel.star() != NULL)
    {
        if (!sel.star()->getVisibility())
            return QObject::tr("Barycenter");
        else
            return QObject::tr("Star");
    }
    else if (sel.body() != NULL)
    {
        int classification = sel.body()->getClassification();
        if (classification == Body::Planet)
            return QObject::tr("Planet");
        else if (classification == Body::Moon)
            return QObject::tr("Moon");
        else if (classification == Body::Asteroid)
            return QObject::tr("Asteroid");
        else if (classification == Body::Comet)
            return QObject::tr("Comet");
        else if (classification == Body::Spacecraft)
            return QObject::tr("Spacecraft");
        else if (classification == Body::Invisible)
            return QObject::tr("Reference point");
    }

    return QObject::tr("Unknown");
}


static QString classificationName(int classification)
{
    if (classification == Body::Planet)
        return QObject::tr("Planets");
    else if (classification == Body::Moon)
        return QObject::tr("Moons");
    else if (classification == Body::Spacecraft)
        return QObject::tr("Spacecraft");
    else if (classification == Body::Asteroid)
        return QObject::tr("Asteroids & comets");
    else if (classification == Body::Invisible)
        return QObject::tr("Reference points");
    else if (classification == Body::SmallBody) // TODO: should have a separate
        return QObject::tr("Minor moons");      // category for this.
    else
        return QObject::tr("Other objects");
}


// Override QAbstractTableModel::data()
QVariant SolarSystemTreeModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    TreeItem* item = itemAtIndex(index);

    // See if the tree item is a group
    if (item->classification != 0)
    {
        if (index.column() == NameColumn)
            return classificationName(item->classification);
        else
            return QVariant();
    }

    // Tree item is an object, not a group

    Selection sel = item->obj;

    switch (index.column())
    {
    case NameColumn:
        if (sel.star() != NULL)
        {
            string starNameString = ReplaceGreekLetterAbbr(universe->getStarCatalog()->getStarName(*sel.star()));
            return QString::fromUtf8(starNameString.c_str());
        }
        else if (sel.body() != NULL)
        {
            return QVariant(sel.body()->getName().c_str());
        }
        else
        {
            return QVariant();
        }

    case TypeColumn:
        return objectTypeName(sel);

    default:
        return QVariant();
    }
}


// Override QAbstractDataModel::headerData()
QVariant SolarSystemTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
    case 0:
        return tr("Name");
    case 1:
        return tr("Type");
    default:
        return QVariant();
    }
}


// Override QAbstractDataModel::rowCount()
int SolarSystemTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        return rootItem->nChildren;
    else
        return static_cast<TreeItem*>(parent.internalPointer())->nChildren;
}


// Override QAbstractDataModel::columnCount()
int SolarSystemTreeModel::columnCount(const QModelIndex&) const
{
    return 2;
}


void SolarSystemTreeModel::sort(int column, Qt::SortOrder order)
{
}


SolarSystemBrowser::SolarSystemBrowser(CelestiaCore* _appCore, QWidget* parent) :
    QWidget(parent),
    appCore(_appCore),
    solarSystemModel(NULL),
    treeView(NULL)
{
    treeView = new QTreeView();
    treeView->setRootIsDecorated(true);
    treeView->setAlternatingRowColors(true);
    treeView->setItemsExpandable(true);
    treeView->setUniformRowHeights(true);
    treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    solarSystemModel = new SolarSystemTreeModel(appCore->getSimulation()->getUniverse());
    treeView->setModel(solarSystemModel);

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(slotContextMenu(const QPoint&)));

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(treeView);

    QPushButton* refreshButton = new QPushButton(tr("Refresh"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(slotRefreshTree()));
    layout->addWidget(refreshButton);

    groupCheckBox = new QCheckBox(tr("Group objects by class"));
    connect(groupCheckBox, SIGNAL(clicked()), this, SLOT(slotRefreshTree()));
    layout->addWidget(groupCheckBox);

    slotRefreshTree();

    setLayout(layout);
}


SolarSystemBrowser::~SolarSystemBrowser()
{
}


/******* Slots ********/

void SolarSystemBrowser::slotRefreshTree()
{
    Simulation* sim = appCore->getSimulation();
    
    // Update the browser with the solar system closest to the active observer
    SolarSystem* solarSys = sim->getUniverse()->getNearestSolarSystem(sim->getActiveObserver()->getPosition());

    // Don't update the solar system browser if no solar system is nearby
    if (!solarSys)
        return;

    Star* rootStar = solarSys->getStar();

    // We want to show all gravitationally associated stars in the
    // browser; follow the chain up the parent star or barycenter.
    while (rootStar->getOrbitBarycenter() != NULL)
    {
        rootStar = rootStar->getOrbitBarycenter();
    }

    bool groupByClass = groupCheckBox->checkState() == Qt::Checked;

    solarSystemModel->buildModel(rootStar, groupByClass);

    treeView->resizeColumnToContents(SolarSystemTreeModel::NameColumn);

    treeView->clearSelection();
}


void SolarSystemBrowser::slotContextMenu(const QPoint& pos)
{
    QModelIndex index = treeView->indexAt(pos);
    Selection sel = solarSystemModel->objectAtIndex(index);

    if (!sel.empty())
    {
        SelectionPopup* menu = new SelectionPopup(sel, appCore, this);
        menu->popupAtGoto(treeView->mapToGlobal(pos));
    }
}


void SolarSystemBrowser::slotMarkSelected()
{
#if 0
    QItemSelectionModel* sm = treeView->selectionModel();
    QModelIndexList rows = sm->selectedRows();

    bool labelMarker = labelMarkerBox->checkState() == Qt::Checked;
    bool convertOK = false;
    QVariant markerData = markerSymbolBox->itemData(markerSymbolBox->currentIndex());
    Marker::Symbol markerSymbol = (Marker::Symbol) markerData.toInt(&convertOK);
    QColor markerColor = colorSwatch->color();
    Color color((float) markerColor.redF(),
                (float) markerColor.greenF(),
                (float) markerColor.blueF());
    
    Universe* universe = appCore->getSimulation()->getUniverse();
    string label;

    for (QModelIndexList::const_iterator iter = rows.begin();
         iter != rows.end(); iter++)
    {
        Selection sel = solarSystemModel->objectAtIndex(*iter);
        if (!sel.empty())
        {
            if (convertOK)
            {
#if 0
                if (labelMarker)
                {
                    label = universe->getDSOCatalog()->getDSOName(dso);
                    label = ReplaceGreekLetterAbbr(label);
                }
#endif

                universe->markObject(sel, 10.0f,
                                     color,
                                     markerSymbol, 1, label);
            }
            else
            {
                universe->unmarkObject(sel, 1);
            }
        }
    }
#endif
}


#if 0
void SolarSystemBrowser::slotClearMarkers()
{
    appCore->getSimulation()->getUniverse()->unmarkAll();
}
#endif
