// celx.cpp
// 
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// Lua script extensions for Celestia
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <cstring>
#include <celengine/celestia.h>

// Ugh . . . the C++ standard says that stringstream should be in
// sstream, but the GNU C++ compiler uses strstream instead.
#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif // HAVE_SSTREAM

#include "celx.h"
#include "celestiacore.h"
extern "C" {
#include "lua/lualib.h"
}

using namespace std;

static char* ClassNames[] =
{
    "class_celestia",
    "class_observer",
    "class_object",
    "class_vec3",
    "class_matrix",
    "class_rotation"
};

static const int _Celestia = 0;
static const int _Observer = 1;
static const int _Object   = 2;
static const int _Vec3     = 3;
static const int _Matrix   = 4;
static const int _Rotation = 5;

#define CLASS(i) ClassNames[(i)]

static void lua_pushclass(lua_State* l, int id)
{
    
    lua_pushlstring(l, ClassNames[id], strlen(ClassNames[id]) - 1);
}

static void lua_setclass(lua_State* l, int id)
{
    lua_pushclass(l, id);
    lua_rawget(l, LUA_REGISTRYINDEX);
    if (lua_type(l, -1) != LUA_TTABLE)
        cout << "Metatable for " << ClassNames[id] << " not found!\n";
    if (lua_setmetatable(l, -2) == 0)
        cout << "Error setting metatable for " << ClassNames[id] << '\n';
}


LuaState::LuaState() :
    state(NULL),
    costate(NULL),
    alive(false)
{
    state = lua_open();
}

LuaState::~LuaState()
{
    if (state != NULL)
        lua_close(state);
#if 0
    if (costate != NULL)
        lua_close(costate);
#endif
}

lua_State* LuaState::getState() const
{
    return state;
}


bool LuaState::createThread()
{
    // Initialize the coroutine which wraps the script
    if (!(lua_isfunction(state, -1) && !lua_iscfunction(state, -1)))
    {
        // Should never happen; we manually set up the stack in C++
        assert(0);
        return false;
    }
    else
    {
        costate = lua_newthread(state);
        if (costate == NULL)
            return false;
        lua_pushvalue(state, -2);
        lua_xmove(state, costate, 1);  /* move function from L to NL */
        alive = true;
        return true;
    }
}


static int auxresume(lua_State *L, lua_State *co, int narg)
{
    int status;
#if 0
    if (!lua_checkstack(co, narg))
        luaL_error(L, "too many arguments to resume");
#endif
    lua_xmove(L, co, narg);

    status = lua_resume(co, narg);
    if (status == 0)
    {
        int nres = lua_gettop(co);
        //printf("args: %d\n", nres);
#if 0
        if (!lua_checkstack(L, narg))
              luaL_error(L, "too many results to resume");
#endif
        lua_xmove(co, L, nres);  /* move yielded values */
        return nres;
    }
    else
    {
        lua_xmove(co, L, 1);  /* move error message */
        return -1;  /* error flag */
    }
}


int LuaState::resume()
{
    assert(costate != NULL);
    if (costate == NULL)
        return false;

    lua_State* co = lua_tothread(state, -1);
    assert(co == costate);
    if (co != costate)
        return false;

    int nArgs = auxresume(state, co, 0);
    if (nArgs < 0)
    {
        alive = false;
        cout << "Error: " << lua_tostring(state, -1) << '\n';
        return 1; // just the error string
    }
    else
    {
        return nArgs; // arguments from yield
    }
}


bool LuaState::isAlive() const
{
    return alive;
}


struct ReadChunkInfo
{
    char* buf;
    int bufSize;
    istream* in;
};

static const char* readStreamChunk(lua_State* state, void* udata, size_t* size)
{
    assert(udata != NULL);
    if (udata == NULL)
        return NULL;

    ReadChunkInfo* info = reinterpret_cast<ReadChunkInfo*>(udata);
    assert(info->buf != NULL);
    assert(info->in != NULL);

    if (!info->in->good())
    {
        *size = 0;
        return NULL;
    }

    info->in->read(info->buf, info->bufSize);
    streamsize nread = info->in->gcount();
    
    *size = (size_t) nread;
    if (nread == 0)
        return NULL;
    else
        return info->buf;
}

int LuaState::loadScript(istream& in)
{
    char buf[4096];
    ReadChunkInfo info;
    info.buf = buf;
    info.bufSize = sizeof(buf);
    info.in = &in;

    int status = lua_load(state, readStreamChunk, &info, "stream");
    
    return status;
}

int LuaState::loadScript(const string& s)
{
#ifdef HAVE_SSTREAM    
    istringstream in(s);
#else
    istrstream in(s.c_str());
#endif
    return loadScript(in);
}


static int parseRenderFlag(const string& name)
{
    if (compareIgnoringCase(name, "orbits") == 0)
        return Renderer::ShowOrbits;
    else if (compareIgnoringCase(name, "cloudmaps") == 0)
        return Renderer::ShowCloudMaps;
    else if (compareIgnoringCase(name, "constellations") == 0)
        return Renderer::ShowDiagrams;
    else if (compareIgnoringCase(name, "galaxies") == 0)
        return Renderer::ShowGalaxies;
    else if (compareIgnoringCase(name, "planets") == 0)
        return Renderer::ShowPlanets;
    else if (compareIgnoringCase(name, "stars") == 0)
        return Renderer::ShowStars;
    else if (compareIgnoringCase(name, "nightmaps") == 0)
        return Renderer::ShowNightMaps;
    else if (compareIgnoringCase(name, "eclipseshadows") == 0)
        return Renderer::ShowEclipseShadows;
    else if (compareIgnoringCase(name, "ringshadows") == 0)
        return Renderer::ShowRingShadows;
    else if (compareIgnoringCase(name, "pointstars") == 0)
        return Renderer::ShowStarsAsPoints;
    else if (compareIgnoringCase(name, "ringshadows") == 0)
        return Renderer::ShowRingShadows;
    else if (compareIgnoringCase(name, "comettails") == 0)
        return Renderer::ShowCometTails;
    else if (compareIgnoringCase(name, "boundaries") == 0)
        return Renderer::ShowBoundaries;
    else
        return 0;
}


// object - star, planet, or deep-sky object
static int object_new(lua_State* l, const Selection& sel)
{
    Selection* ud = reinterpret_cast<Selection*>(lua_newuserdata(l, sizeof(Selection)));
    *ud = sel;

    lua_setclass(l, _Object);

    return 1;
}

static Selection* to_object(lua_State* l, int index)
{
    // TODO: need to verify that this is actually an object, not some
    // other userdata
    return static_cast<Selection*>(lua_touserdata(l, index));
}

static int object_tostring(lua_State* l)
{
    cout << "object_tostring\n"; cout.flush();
    lua_pushstring(l, "[Object]");

    return 1;
}

static int object_radius(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 1)
    {
        lua_pushstring(l, "No arguments expected to function object:radius");
        lua_error(l);
    }

    Selection* sel = to_object(l, 1);
    if (sel != NULL)
    {
        lua_pushnumber(l, sel->radius());
    }
    else
    {
        lua_pushstring(l, "Bad object!");
        lua_error(l);
    }

    return 1;
}


static void CreateObjectMetaTable(lua_State* l)
{
    lua_newtable(l);
    lua_pushclass(l, _Object);
    lua_pushvalue(l, -2);
    lua_rawset(l, LUA_REGISTRYINDEX);

    lua_pushliteral(l, "__tostring");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, object_tostring, 1);
    lua_rawset(l, -3);

    lua_pushliteral(l, "__index");
    lua_pushvalue(l, -2);
    lua_rawset(l, -3);

    lua_pushstring(l, "radius");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, object_radius, 1);
    lua_settable(l, -3);

    lua_pop(l, 1);
}


// observer object

static int observer_new(lua_State* l, Observer* o)
{
    Observer** ud = static_cast<Observer**>(lua_newuserdata(l, sizeof(Observer*)));
    *ud = o;

    lua_setclass(l, _Observer);

    return 1;
}


static Observer* to_observer(lua_State* l, int index)
{
    // TODO: need to verify that this is actually an object, not some
    // other userdata
    Observer** o = static_cast<Observer**>(lua_touserdata(l, index));
    if (o == NULL)
        return NULL;
    else
        return *o;
}


static int observer_goto(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 2)
    {
        lua_pushstring(l, "Two arguments expected to function observer:goto");
        lua_error(l);
    }

    Observer* o = to_observer(l, 1);
    if (o != NULL)
    {
        Selection* sel = to_object(l, 2);
        if (sel != NULL)
        {
            o->gotoSelection(*sel, 5.0, Vec3f(0, 1, 0), astro::ObserverLocal);
        }
    }

    return 0;
}


static int observer_center(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc > 1)
    {
        lua_pushstring(l, "Too many argument to function observer:center");
        lua_error(l);
    }

    Observer* o = to_observer(l, 1);
    if (o != NULL)
    {
    }

    return 0;
}


static int observer_tostring(lua_State* l)
{
    cout << "observer_tostring\n"; cout.flush();
    lua_pushstring(l, "[Observer]");

    return 1;
}

static void CreateObserverMetaTable(lua_State* l)
{
    lua_pushclass(l, _Observer);
    lua_newtable(l);

    lua_pushliteral(l, "__tostring");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, observer_tostring, 1);
    lua_rawset(l, -3);

    lua_pushliteral(l, "__index");
    lua_pushvalue(l, -2);
    lua_rawset(l, -3);
    lua_pushvalue(l, -1);

    lua_pushstring(l, "goto");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, observer_goto, 1);
    lua_settable(l, -4);
    lua_pushstring(l, "center");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, observer_center, 1);
    lua_settable(l, -4);
    lua_pop(l, 1);

    lua_rawset(l, LUA_REGISTRYINDEX);
}


// Celestia objects

static int celestia_new(lua_State* l, CelestiaCore* appCore)
{
    CelestiaCore** ud = reinterpret_cast<CelestiaCore**>(lua_newuserdata(l, sizeof(CelestiaCore*)));
    *ud = appCore;

    lua_setclass(l, _Celestia);

    return 1;
}


static CelestiaCore* to_celestia(lua_State* l, int index)
{
    CelestiaCore** c = reinterpret_cast<CelestiaCore**>(lua_touserdata(l, index));
    if (c == NULL)
        return NULL;
    else
        return *c;
}


static int celestia_flash(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 2)
    {
        lua_pushstring(l, "One argument expected to function celestia:flash");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);
    const char* s = lua_tostring(l, 2);

    if (appCore != NULL && s != NULL)
        appCore->flash(s, 1.5);

    return 0;
}


static int celestia_show(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc < 1)
    {
        lua_pushstring(l, "One argument expected to function celestia:show");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);

    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        const char* s = lua_tostring(l, i);
        if (s != NULL)
            flags |= parseRenderFlag(s);
    }

    if (appCore != NULL)
    {
        Renderer* r = appCore->getRenderer();
        r->setRenderFlags(r->getRenderFlags() | flags);
    }

    return 0;
}


static int celestia_hide(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc < 1)
    {
        lua_pushstring(l, "One argument expected to function celestia:hide");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);

    int flags = 0;
    for (int i = 2; i <= argc; i++)
    {
        const char* s = lua_tostring(l, i);
        if (s != NULL)
            flags |= parseRenderFlag(s);
    }

    if (appCore != NULL)
    {
        Renderer* r = appCore->getRenderer();
        r->setRenderFlags(r->getRenderFlags() & ~flags);
    }

    return 0;
}


static int celestia_getobserver(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc < 1 || argc > 2)
    {
        lua_pushstring(l, "Wrong number of arguments for function celestia:getobserver()");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);

    if (appCore != NULL)
    {
        cout << "celestia_getobserver\n"; cout.flush();
        Observer* o = appCore->getSimulation()->getActiveObserver();
        if (o == NULL)
            lua_pushnil(l);
        else
            observer_new(l, o);
    }
    else
    {
        lua_pushstring(l, "Bad celestia object!\n");
        lua_error(l);
    }

    return 1;
}


static int celestia_find(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 2 || !lua_isstring(l, 2))
    {
        lua_pushstring(l, "One string argument expected for function celestia:find()");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore != NULL)
    {
        Simulation* sim = appCore->getSimulation();
        // Should use universe not simulation for finding objects
        Selection sel = sim->findObjectFromPath(lua_tostring(l, 2));
        object_new(l, sel);
    }
    else
    {
        lua_pushstring(l, "Bad celestia object!\n");
        lua_error(l);
    }

    return 1;
}


static int celestia_select(lua_State* l)
{
    int argc = lua_gettop(l);
    if (argc != 2)
    {
        lua_pushstring(l, "One argument expected to function observer:select");
        lua_error(l);
    }

    CelestiaCore* appCore = to_celestia(l, 1);
    if (appCore != NULL)
    {
        Simulation* sim = appCore->getSimulation();
        Selection* sel = to_object(l, 2);

        // If the argument is an object, set the selection; if it's anything else
        // clear the selection.
        if (sel != NULL)
            sim->setSelection(*sel);
        else
            sim->setSelection(Selection());
    }

    return 0;
}


static int celestia_tostring(lua_State* l)
{
    lua_pushstring(l, "[Celestia]");

    return 1;
}


static void CreateCelestiaMetaTable(lua_State* l)
{
    lua_newtable(l);
    lua_pushclass(l, _Celestia);
    lua_pushvalue(l, -2);
    lua_rawset(l, LUA_REGISTRYINDEX);

    lua_pushliteral(l, "__tostring");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_tostring, 1);
    lua_rawset(l, -3);

    lua_pushliteral(l, "__index");
    lua_pushvalue(l, -2);
    lua_rawset(l, -3);

    lua_pushstring(l, "flash");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_flash, 1);
    lua_settable(l, -3);

    lua_pushstring(l, "show");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_show, 1);
    lua_settable(l, -3);

    lua_pushstring(l, "hide");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_hide, 1);
    lua_settable(l, -3);

    lua_pushstring(l, "getobserver");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_getobserver, 1);
    lua_settable(l, -3);

    lua_pushstring(l, "find");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_find, 1);
    lua_settable(l, -3);

    lua_pushstring(l, "select");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_select, 1);
    lua_settable(l, -3);

    lua_pop(l, 1);
#if 0
    lua_pushclass(l, _Celestia);
    lua_newtable(l);

    lua_pushliteral(l, "__tostring");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_tostring, 1);
    lua_rawset(l, -3);

    lua_pushliteral(l, "__index");
    lua_pushvalue(l, -2);
    lua_rawset(l, -3);
    lua_pushvalue(l, -1);

    lua_pushstring(l, "flash");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_flash, 1);
    lua_settable(l, -4);
    lua_pushstring(l, "show");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_show, 1);
    lua_settable(l, -4);
    lua_pushstring(l, "hide");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_hide, 1);
    lua_settable(l, -4);
    lua_pushstring(l, "getobserver");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_getobserver, 1);
    lua_settable(l, -4);
    lua_pushstring(l, "find");
    lua_pushvalue(l, -2);
    lua_pushcclosure(l, celestia_find, 1);
    lua_settable(l, -4);
    lua_pop(l, 1);

    lua_rawset(l, LUA_REGISTRYINDEX);
#endif
}


bool LuaState::init(CelestiaCore* appCore)
{
    // Import the base library
    lua_baselibopen(state);

    // Add an easy to use wait function, so that script writers can
    // live in ignorance of coroutines.  There will probably be a significant
    // library of useful functions that can be defined purely in Lua.
    // At that point, we'll want something a bit more robust than just
    // parsing the whole text of the library every time a script is launched
    if (loadScript("wait = function(x) coroutine.yield(x) end") != 0)
        return false;
    lua_pcall(state, 0, 0, 0); // execute it

    CreateObjectMetaTable(state);
    CreateObserverMetaTable(state);
    CreateCelestiaMetaTable(state);

    // Create the celestia object
    lua_pushstring(state, "celestia");
    celestia_new(state, appCore);
    lua_settable(state, LUA_GLOBALSINDEX);

    return true;
}
