!IF "$(CFG)" == ""
CFG=Debug
!MESSAGE No configuration specified. Defaulting to debug.
!ENDIF

!IF "$(CFG)" == "Release"
OUTDIR=.\Release
INTDIR=.\Release
!ELSE
OUTDIR=.\Debug
INTDIR=.\Debug
!ENDIF

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OBJS=\
	$(INTDIR)\3dsmesh.obj \
	$(INTDIR)\asterism.obj \
	$(INTDIR)\astro.obj \
	$(INTDIR)\body.obj \
	$(INTDIR)\catalogxref.obj \
	$(INTDIR)\cmdparser.obj \
	$(INTDIR)\command.obj \
	$(INTDIR)\constellation.obj \
	$(INTDIR)\customorbit.obj \
	$(INTDIR)\dispmap.obj \
	$(INTDIR)\execution.obj \
	$(INTDIR)\galaxy.obj \
	$(INTDIR)\glext.obj \
	$(INTDIR)\lodspheremesh.obj \
	$(INTDIR)\meshmanager.obj \
	$(INTDIR)\observer.obj \
	$(INTDIR)\octree.obj \
	$(INTDIR)\orbit.obj \
	$(INTDIR)\overlay.obj \
	$(INTDIR)\parser.obj \
	$(INTDIR)\regcombine.obj \
	$(INTDIR)\render.obj \
	$(INTDIR)\selection.obj \
	$(INTDIR)\simulation.obj \
	$(INTDIR)\solarsys.obj \
	$(INTDIR)\spheremesh.obj \
	$(INTDIR)\star.obj \
	$(INTDIR)\stardb.obj \
	$(INTDIR)\starname.obj \
	$(INTDIR)\stellarclass.obj \
	$(INTDIR)\texmanager.obj \
	$(INTDIR)\texture.obj \
	$(INTDIR)\tokenizer.obj \
	$(INTDIR)\univcoord.obj \
	$(INTDIR)\vertexlist.obj \
	$(INTDIR)\vertexprog.obj

TARGETLIB = cel_engine.lib

INCLUDEDIRS=/I .. /I ../../inc/libjpeg /I ../../inc/libpng /I ../../inc/libz

!IF "$(CFG)" == "Release"
CPP=cl.exe
CPPFLAGS=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /Fp"$(INTDIR)\celestia.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c $(INCLUDEDIRS)
!ELSE
CPP=cl.exe
CPPFLAGS=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /Fp"$(INTDIR)\celestia.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c $(INCLUDEDIRS)
!ENDIF

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPPFLAGS) $<
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPPFLAGS) $<
<<

$(OUTDIR)\$(TARGETLIB) : $(OUTDIR) $(OBJS)
	lib @<<
        /out:$(OUTDIR)\$(TARGETLIB) $(OBJS)
<<

"$(OUTDIR)" :
	if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

clean:
	-@del $(OUTDIR)\$(TARGETLIB) $(OBJS)
