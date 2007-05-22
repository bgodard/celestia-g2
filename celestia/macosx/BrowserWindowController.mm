//
//  BrowserWindowController.mm
//  celestia
//
//  Created by Hank Ramsey on Tue Dec 28 2004.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//  Modifications Copyright (c) 2004 Hank Ramsey. All rights reserved.
//

#import "BrowserWindowController.h"
#import "NSString_ObjCPlusPlus.h"
#import "CelestiaAppCore.h"
#import "CelestiaStar_PrivateAPI.h"
#import "CelestiaBody_PrivateAPI.h"
#import "CelestiaDSO.h"
#import "CelestiaLocation.h"

#include "celestiacore.h"
#include "celestia.h"
#include "selection.h"
#include "starbrowser.h"

#define BROWSER_MAX_DSO_COUNT   500
#define BROWSER_MAX_STAR_COUNT  100


@interface BrowserItem : NSObject
{
    id data;
    NSMutableDictionary *children;
    NSArray *childNames;
    BOOL childrenChanged;
}
- (id)initWithCelestiaDSO:      (CelestiaDSO *)aDSO;
- (id)initWithCelestiaStar:     (CelestiaStar *)aStar;
- (id)initWithCelestiaBody:     (CelestiaBody *)aBody;
- (id)initWithCelestiaLocation: (CelestiaLocation *)aLocation;
- (id)initWithName:             (NSString *)aName;
- (id)initWithName:             (NSString *)aName
          children:             (NSDictionary *)aChildren;
- (void)dealloc;

- (NSString *)name;
- (id)body;
- (void)addChild: (BrowserItem *)aChild;
- (id)childNamed: (NSString *)aName;
- (NSArray *)allChildNames;
- (unsigned)childCount;
@end

@implementation BrowserItem
- (id)initWithCelestiaDSO: (CelestiaDSO *)aDSO
{
    self = [super init];
    if (self) data = [aDSO retain];
    return self;
}
- (id)initWithCelestiaStar: (CelestiaStar *)aStar
{
    self = [super init];
    if (self) data = [aStar retain];
    return self;
}
- (id)initWithCelestiaBody: (CelestiaBody *)aBody
{
    self = [super init];
    if (self) data = [aBody retain];
    return self;
}
- (id)initWithCelestiaLocation: (CelestiaLocation *)aLocation
{
    self = [super init];
    if (self) data = [aLocation retain];
    return self;
}
- (id)initWithName: (NSString *)aName
{
    self = [super init];
    if (self) data = [aName retain];
    return self;
}
- (id)initWithName: (NSString *)aName
          children: (NSDictionary *)aChildren
{
    self = [super init];
    if (self)
    {
        data = [aName retain];
        if (nil == children)
        {
            children = [[NSMutableDictionary alloc] initWithDictionary: aChildren];
            childrenChanged = YES;
        }
    }
    return self;
}

- (void)dealloc
{
    [childNames release];
    [children release];
    [data release];
    [super dealloc];
}

- (NSString *)name
{
    return ([data respondsToSelector:@selector(name)]) ? [data name] : data;
}

- (id)body
{
    return ([data isKindOfClass: [NSString class]]) ? nil : data;
}

- (void)addChild: (BrowserItem *)aChild
{
    if (nil == children)
        children = [[NSMutableDictionary alloc] init];

    [children setObject: aChild forKey: [aChild name]];
    childrenChanged = YES;
}

- (id)childNamed: (NSString *)aName
{
    return [children objectForKey: aName];
}

- (NSArray *)allChildNames
{
    if (childrenChanged)
    {
        [childNames release]; childNames = nil;
    }

    if (nil == childNames)
    {
        childNames = [[[children allKeys] sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)] retain];
        childrenChanged = NO;
    }

    return childNames;
}

- (unsigned)childCount
{
    return [children count];
}
@end


@implementation BrowserWindowController

static BrowserItem *staticRootItem;
static CelestiaCore *appCore;

- (id) init
{
    self = [super initWithWindowNibName: @"BrowserWindow" ];
    appCore = (CelestiaCore*) [[CelestiaAppCore sharedAppCore] appCore];
    return self;
}

- (void) windowDidLoad
{
    if ([self window]==nil) NSLog(@"loaded browser window is nil");
}

//--------------------------------------------------------------

- (NSDictionary *) deepSkyObjects
{
    int objCount;
    NSMutableDictionary* newDict;
    int i = 0;
    DSODatabase* catalog = appCore->getSimulation()->getUniverse()->getDSOCatalog();
    DeepSkyObject *obj;
    CelestiaDSO *dsoWrapper;
    NSString *name;

    objCount = catalog->size();
    if (objCount > BROWSER_MAX_DSO_COUNT)
        objCount = BROWSER_MAX_DSO_COUNT;
    newDict = [NSMutableDictionary dictionaryWithCapacity: objCount];

    for (; i < objCount; ++i)
    {
        obj = catalog->getDSO(i);
        if (obj)
        {
            name = [NSString stringWithStdString:
                catalog->getDSOName(obj)];
            dsoWrapper = [[[CelestiaDSO alloc] initWithDSO: obj] autorelease];
            [newDict setObject: [[[BrowserItem alloc] initWithCelestiaDSO: dsoWrapper] autorelease]
                        forKey: [NSString stringWithFormat: @"%@ (%@)", name, [dsoWrapper type]]];
        }
    }

    return newDict;       
}

- (BrowserItem *) sol
{
    Selection sol = appCore->getSimulation()->getUniverse()->find("Sol");
    return [[[BrowserItem alloc] initWithCelestiaStar: [[[CelestiaStar alloc] initWithStar: sol.star()] autorelease]] autorelease];
}

- (NSDictionary *) starsOfKind: (int) kind
{
    std::vector<const Star*>* nearStars;
    int starCount = 0;
    Simulation *sim = appCore->getSimulation();
    StarBrowser* sb = new StarBrowser(sim,kind);
    nearStars = sb->listStars( BROWSER_MAX_STAR_COUNT );
    if (nearStars == nil ) return [NSDictionary dictionary];
    starCount = nearStars->size();            
    NSMutableDictionary* starDict = [NSMutableDictionary dictionaryWithCapacity: starCount+2];
    const Star *aStar;
    NSString *starName;
    int i;
    for (i=0;i<starCount;i++)
    {
        aStar = (*nearStars)[i];
        starName = [NSString  stringWithStdString: sim->getUniverse()->getStarCatalog()->getStarName(*aStar) ];
        [starDict setObject:
            [[[BrowserItem alloc] initWithCelestiaStar: [[[CelestiaStar alloc] initWithStar: aStar] autorelease]] autorelease]
                     forKey: starName];
    }

    delete sb;
    delete nearStars;
    return starDict;
}

-(BrowserItem *) root
{
    if (staticRootItem) return staticRootItem;
    BrowserItem *rootItem = [[BrowserItem alloc] initWithName: @""];

    [rootItem addChild: [self sol]];
    [rootItem addChild: [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Nearest Stars",@"") children: [self starsOfKind: StarBrowser::NearestStars]] autorelease]];
    [rootItem addChild: [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Brightest Stars",@"") children: [self starsOfKind: StarBrowser::BrighterStars]] autorelease]];
    [rootItem addChild: [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Stars With Planets",@"") children: [self starsOfKind: StarBrowser::StarsWithPlanets]] autorelease]];
    [rootItem addChild: [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Deep Sky Objects",@"") children: [self deepSkyObjects]] autorelease]];

    staticRootItem = rootItem;
    return staticRootItem;
}

-(void) addChildrenForBody: (BrowserItem *) aBody
{
    PlanetarySystem* sys = [(CelestiaBody *)[aBody body] body]->getSatellites();

    if (sys)
    { 
        int sysSize = sys->getSystemSize();
        BrowserItem *subItem = nil;
        BrowserItem *minorMoons = nil;
        BrowserItem *comets = nil;
        BrowserItem *spacecrafts = nil;
        int i;
        for ( i=0; i<sysSize; i++)
        {
            Body* body = sys->getBody(i);
            BrowserItem *item = [[[BrowserItem alloc] initWithCelestiaBody:
                [[[CelestiaBody alloc] initWithBody: body] autorelease]] autorelease];
            int bodyClass  = body->getClassification();
            if (bodyClass==Body::Asteroid) bodyClass = Body::Moon;

            switch (bodyClass)
            {
                case Body::Invisible:
                    continue;
                case Body::Moon:
                    if (body->getRadius() < 100.0f)
                    {
                        if (!minorMoons)
                            minorMoons = [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Minor Moons",@"")] autorelease];
                        subItem = minorMoons;
                    }
                    else
                    {
                        subItem = aBody;
                    }
                    break;
                case Body::Comet:
                    if (!comets)
                        comets = [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Comets",@"")] autorelease];
                    subItem = comets;
                    break;
                case Body::Spacecraft:
                    if (!spacecrafts)
                        spacecrafts = [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Spacecrafts",@"")] autorelease];
                    subItem = spacecrafts;
                    break;
                default:
                    subItem = aBody;
                    break;
            }
            [subItem addChild: item];
        }
        
        if (minorMoons)  [aBody addChild: minorMoons];
        if (comets)      [aBody addChild: comets];
        if (spacecrafts) [aBody addChild: spacecrafts];
    }
    
    std::vector<Location*>* locations = [(CelestiaBody *)[aBody body] body]->getLocations();
    if (locations != NULL)
    {
        BrowserItem *locationItems = [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Locations",@"")] autorelease];
        for (vector<Location*>::const_iterator iter = locations->begin();
             iter != locations->end(); iter++)
        {
            [locationItems addChild: [[[BrowserItem alloc] initWithCelestiaLocation: [[[CelestiaLocation alloc] initWithLocation: *iter] autorelease]] autorelease]];
        }
        [aBody addChild: locationItems];
    }
}

-(void) addChildrenForStar: (BrowserItem *) aStar
{
    SolarSystem *ss = appCore->getSimulation()->getUniverse()->getSolarSystem([((CelestiaStar *)[aStar body]) star]);
    PlanetarySystem* sys = NULL;
    if (ss) sys = ss->getPlanets();
    
    if (sys)
    {
        int sysSize = sys->getSystemSize();
        BrowserItem *subItem = nil;
        BrowserItem *minorMoons = nil;
        BrowserItem *asteroids = nil;
        BrowserItem *comets = nil;
        BrowserItem *spacecrafts = nil;
        int i;
        for ( i=0; i<sysSize; i++)
        {
            Body* body = sys->getBody(i);
            BrowserItem *item = [[[BrowserItem alloc] initWithCelestiaBody:
                [[[CelestiaBody alloc] initWithBody: body] autorelease]] autorelease];
            int bodyClass  = body->getClassification();
            switch (bodyClass)
            {
                case Body::Invisible:
                    continue;
                case Body::Moon:
                    if (body->getRadius() < 100.0f)
                    {
                        if (!minorMoons)
                            minorMoons = [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Minor Moons",@"")] autorelease];
                        subItem = minorMoons;
                    }
                    else
                    {
                        subItem = aStar;
                    }
                    break;
                case Body::Asteroid:
                    if (!asteroids)
                        asteroids = [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Asteroids",@"")] autorelease];
                    subItem = asteroids;
                    break;
                case Body::Comet:
                    if (!comets)
                        comets = [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Comets",@"")] autorelease];
                    subItem = comets;
                    break;
                case Body::Spacecraft:
                    if (!spacecrafts)
                        spacecrafts = [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Spacecrafts",@"")] autorelease];
                    subItem = spacecrafts;
                    break;
                default:
                    subItem = aStar;
                    break;
            }
            [subItem addChild: item];
        }
        
        if (minorMoons)  [aStar addChild: minorMoons];
        if (asteroids)   [aStar addChild: asteroids];
        if (comets)      [aStar addChild: comets];
        if (spacecrafts) [aStar addChild: spacecrafts];
    }
}

- (BrowserItem *) itemForPathArray: (NSArray*) pathNames
{
    BrowserItem *lastItem = [self root];
    BrowserItem *nextItem = lastItem;
    NSString *lastKey = nil;
    id body;
    unsigned i;
    for (i=1;i<[pathNames count];i++)
    {
        lastKey = [pathNames objectAtIndex: i];
        nextItem = [lastItem childNamed: lastKey];
        if (nil==nextItem) break;
        lastItem = nextItem;
    }

    if (nextItem)
    {
        body = [nextItem body];
        if (body)
        {
            if      ([body respondsToSelector: @selector(star)])
                [self addChildrenForStar: nextItem];
            else if ([body respondsToSelector: @selector(body)])
                [self addChildrenForBody: nextItem];
        }
    }
    
    return lastItem;
}

- (Selection *) selFromPathArray: (NSArray*) pathNames
{
    Selection *sel = NULL;
    BrowserItem *item = [self itemForPathArray: pathNames];
    id body = [item body];
    if (body)
    {
        if      ([body respondsToSelector: @selector(star)])
            sel = new Selection([(CelestiaStar *)body star]);
        else if ([body respondsToSelector: @selector(body)])
            sel = new Selection([(CelestiaBody *)body body]);
        else if ([body respondsToSelector: @selector(DSO)])
            sel = new Selection([(CelestiaDSO *)body DSO]);
        else if ([body respondsToSelector: @selector(location)])
            sel = new Selection([(CelestiaLocation *)body location]);
    }
//    if (sel == nil) NSLog([NSString stringWithFormat: @"obj %@ not found", objPath]);
    return sel;
}


- (int) browser: (NSBrowser*) sender numberOfRowsInColumn: (int) column
{
    if (browser==nil) 
    {
        browser = sender;        
        if ([browser respondsToSelector:@selector(setColumnResizingType:)])
            [browser setColumnResizingType: 2];
        [browser setMinColumnWidth: 80];
        [browser setDoubleAction:@selector(doubleClick:)];
    }

    BrowserItem *itemForColumn = [self itemForPathArray: [[sender pathToColumn: column ] componentsSeparatedByString: [sender pathSeparator] ] ];
    return [itemForColumn childCount];
}

- (BOOL) isLeaf: (BrowserItem *) aItem
{
    if ([aItem childCount] > 0) return NO;
    id body = [aItem body];
    if (body)
    {
        if      ([body respondsToSelector: @selector(star)])
        {
            if (appCore->getSimulation()->getUniverse()->getSolarSystem([(CelestiaStar *)body star]))
                return NO;
        }
        else if ([body respondsToSelector: @selector(body)])
        {
            if ([(CelestiaBody *)body body]->getSatellites() || [(CelestiaBody *)body body]->getLocations())
                return NO;
        }
    }
    return YES;
}

- (void) browser: (NSBrowser*) sender willDisplayCell: (id) cell atRow: (int) row column: (int) column
{
    BrowserItem *itemForColumn = [self itemForPathArray: [[sender pathToColumn: column ] componentsSeparatedByString: [sender pathSeparator] ] ];
    NSArray* colKeys = [itemForColumn allChildNames];
    BOOL isLeaf = YES;
    NSString* itemName = [colKeys objectAtIndex: row];
    if (!itemName)
        itemName = @"????";
    else
        isLeaf = [self isLeaf: [itemForColumn childNamed: itemName ]];

    NSRange rightParenRange = {NSNotFound, 0};
    NSRange leftParenRange  = {NSNotFound, 0};
    NSRange parenRange      = {NSNotFound, 0};

    if (isLeaf)
    {
        rightParenRange = [itemName rangeOfString:@")"
                                          options:NSBackwardsSearch];
        if (rightParenRange.length == 1)
        {
            leftParenRange = NSMakeRange(0, rightParenRange.location - 1);
            parenRange = [itemName rangeOfString:@"("
                                         options:NSBackwardsSearch
                                           range:leftParenRange];
            if (parenRange.length == 1)
            {
                parenRange.length = rightParenRange.location - parenRange.location + 1;
            }
        }
    }

    if (parenRange.location != NSNotFound)
    {
        NSDictionary *parenAttr = [NSDictionary dictionaryWithObjectsAndKeys:
            [NSColor grayColor], NSForegroundColorAttributeName,
            nil];
        NSMutableAttributedString *attrStr = [[[NSMutableAttributedString alloc] initWithString: itemName] autorelease];
        [attrStr setAttributes:parenAttr range:parenRange];

        [cell setAttributedStringValue:attrStr];
    }
    else
    {
        [cell setTitle: itemName ];
    }

    [cell setLeaf: isLeaf];
}

- (IBAction) go: (id) sender
{
    Selection *sel = [self selFromPathArray: [[browser path] componentsSeparatedByString: [browser pathSeparator] ]];
    if (sel)
    {
        appCore->getSimulation()->setSelection(*sel);
        if ([sender tag]!=0) appCore->charEntered([sender tag]);
        delete sel;
    }
}

- (IBAction) doubleClick: (id) sender
{
    Selection *sel = [self selFromPathArray: [[browser path] componentsSeparatedByString: [browser pathSeparator] ]];
    if (sel)
    {
        appCore->getSimulation()->setSelection(*sel);
        appCore->charEntered('g');
    // adjust columns immediately
//    [self adjustColumns: nil];
        delete sel;
    }
}

- (void)windowDidBecomeKey:(NSNotification *)aNotification
{
    if ([aNotification object] == [self window] && staticRootItem)
    {
        [staticRootItem addChild: [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Nearest Stars",@"") children: [self starsOfKind: StarBrowser::NearestStars]] autorelease]];

        // Immediately synch browser display with reloaded star list,
        // but only if star list is showing
        NSArray *selection = [[browser path] componentsSeparatedByString: [browser pathSeparator]];
        if ([selection count]>1 && [[selection objectAtIndex: 1] isEqualToString: NSLocalizedString(@"Nearest Stars",@"")])
        {
            for (int i=1; i<[browser numberOfVisibleColumns]; ++i)
                [browser reloadColumn: i];
        }
    }
}
@end
