# Microsoft Developer Studio Project File - Name="celengine" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=celengine - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "celengine.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "celengine.mak" CFG="celengine - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "celengine - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "celengine - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "celengine - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".." /I "..\..\inc\libjpeg" /I "..\..\inc\libpng" /I "..\..\inc\libz" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\cel_engine.lib"

!ELSEIF  "$(CFG)" == "celengine - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".." /I "..\..\inc\libjpeg" /I "..\..\inc\libpng" /I "..\..\inc\libz" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\cel_engine.lib"

!ENDIF 

# Begin Target

# Name "celengine - Win32 Release"
# Name "celengine - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\3dsmesh.cpp
# End Source File
# Begin Source File

SOURCE=.\asterism.cpp
# End Source File
# Begin Source File

SOURCE=.\astro.cpp
# End Source File
# Begin Source File

SOURCE=.\body.cpp
# End Source File
# Begin Source File

SOURCE=.\catalogxref.cpp
# End Source File
# Begin Source File

SOURCE=.\cmdparser.cpp
# End Source File
# Begin Source File

SOURCE=.\command.cpp
# End Source File
# Begin Source File

SOURCE=.\constellation.cpp
# End Source File
# Begin Source File

SOURCE=.\customorbit.cpp
# End Source File
# Begin Source File

SOURCE=.\dds.cpp
# End Source File
# Begin Source File

SOURCE=.\dispmap.cpp
# End Source File
# Begin Source File

SOURCE=.\execution.cpp
# End Source File
# Begin Source File

SOURCE=.\galaxy.cpp
# End Source File
# Begin Source File

SOURCE=.\glext.cpp
# End Source File
# Begin Source File

SOURCE=.\lodspheremesh.cpp
# End Source File
# Begin Source File

SOURCE=.\meshmanager.cpp
# End Source File
# Begin Source File

SOURCE=.\multitexture.cpp
# End Source File
# Begin Source File

SOURCE=.\observer.cpp
# End Source File
# Begin Source File

SOURCE=.\octree.cpp
# End Source File
# Begin Source File

SOURCE=.\orbit.cpp
# End Source File
# Begin Source File

SOURCE=.\overlay.cpp
# End Source File
# Begin Source File

SOURCE=.\parser.cpp
# End Source File
# Begin Source File

SOURCE=.\regcombine.cpp
# End Source File
# Begin Source File

SOURCE=.\render.cpp
# End Source File
# Begin Source File

SOURCE=.\selection.cpp
# End Source File
# Begin Source File

SOURCE=.\simulation.cpp
# End Source File
# Begin Source File

SOURCE=.\solarsys.cpp
# End Source File
# Begin Source File

SOURCE=.\spheremesh.cpp
# End Source File
# Begin Source File

SOURCE=.\star.cpp
# End Source File
# Begin Source File

SOURCE=.\stardb.cpp
# End Source File
# Begin Source File

SOURCE=.\starname.cpp
# End Source File
# Begin Source File

SOURCE=.\stellarclass.cpp
# End Source File
# Begin Source File

SOURCE=.\texmanager.cpp
# End Source File
# Begin Source File

SOURCE=.\texture.cpp
# End Source File
# Begin Source File

SOURCE=.\tokenizer.cpp
# End Source File
# Begin Source File

SOURCE=.\univcoord.cpp
# End Source File
# Begin Source File

SOURCE=.\universe.cpp
# End Source File
# Begin Source File

SOURCE=.\vertexlist.cpp
# End Source File
# Begin Source File

SOURCE=.\vertexprog.cpp
# End Source File
# Begin Source File

SOURCE=.\vsop87.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\3dsmesh.h
# End Source File
# Begin Source File

SOURCE=.\asterism.h
# End Source File
# Begin Source File

SOURCE=.\astro.h
# End Source File
# Begin Source File

SOURCE=.\atmosphere.h
# End Source File
# Begin Source File

SOURCE=.\body.h
# End Source File
# Begin Source File

SOURCE=.\catalogxref.h
# End Source File
# Begin Source File

SOURCE=.\celestia.h
# End Source File
# Begin Source File

SOURCE=.\cmdparser.h
# End Source File
# Begin Source File

SOURCE=.\command.h
# End Source File
# Begin Source File

SOURCE=.\constellation.h
# End Source File
# Begin Source File

SOURCE=.\customorbit.h
# End Source File
# Begin Source File

SOURCE=.\dispmap.h
# End Source File
# Begin Source File

SOURCE=.\execenv.h
# End Source File
# Begin Source File

SOURCE=.\execution.h
# End Source File
# Begin Source File

SOURCE=.\galaxy.h
# End Source File
# Begin Source File

SOURCE=.\gl.h
# End Source File
# Begin Source File

SOURCE=.\glext.h
# End Source File
# Begin Source File

SOURCE=.\lodspheremesh.h
# End Source File
# Begin Source File

SOURCE=.\mesh.h
# End Source File
# Begin Source File

SOURCE=.\meshmanager.h
# End Source File
# Begin Source File

SOURCE=.\multitexture.h
# End Source File
# Begin Source File

SOURCE=.\observer.h
# End Source File
# Begin Source File

SOURCE=.\octree.h
# End Source File
# Begin Source File

SOURCE=.\orbit.h
# End Source File
# Begin Source File

SOURCE=.\overlay.h
# End Source File
# Begin Source File

SOURCE=.\parser.h
# End Source File
# Begin Source File

SOURCE=.\regcombine.h
# End Source File
# Begin Source File

SOURCE=.\render.h
# End Source File
# Begin Source File

SOURCE=.\selection.h
# End Source File
# Begin Source File

SOURCE=.\simulation.h
# End Source File
# Begin Source File

SOURCE=.\solarsys.h
# End Source File
# Begin Source File

SOURCE=.\spheremesh.h
# End Source File
# Begin Source File

SOURCE=.\star.h
# End Source File
# Begin Source File

SOURCE=.\stardb.h
# End Source File
# Begin Source File

SOURCE=.\starname.h
# End Source File
# Begin Source File

SOURCE=.\stellarclass.h
# End Source File
# Begin Source File

SOURCE=.\surface.h
# End Source File
# Begin Source File

SOURCE=.\texmanager.h
# End Source File
# Begin Source File

SOURCE=.\texture.h
# End Source File
# Begin Source File

SOURCE=.\tokenizer.h
# End Source File
# Begin Source File

SOURCE=.\univcoord.h
# End Source File
# Begin Source File

SOURCE=.\universe.h
# End Source File
# Begin Source File

SOURCE=.\vecgl.h
# End Source File
# Begin Source File

SOURCE=.\vertexbuf.h
# End Source File
# Begin Source File

SOURCE=.\vertexlist.h
# End Source File
# Begin Source File

SOURCE=.\vertexprog.h
# End Source File
# Begin Source File

SOURCE=.\vsop87.h
# End Source File
# End Group
# End Target
# End Project
