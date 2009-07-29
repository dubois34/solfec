#
# Compilation flags setup
#

ifeq ($(OPENGL),yes)
  OPENGL = -DOPENGL $(GLINC)
else
  OPENGL =
  GLLIB = 
endif

ifeq ($(DEBUG),yes)
  DEBUG =  -W -Wall -Wno-unused-parameter -pedantic -g -DDEBUG
  ifeq ($(PROFILE),yes)
    PROFILE = -p
  else
    PROFILE =
  endif
  ifeq ($(NOTHROW),yes)
    NOTHROW = -DNOTHROW
  else
    NOTHROW =
  endif
  ifeq ($(MEMDEBUG),yes)
    MEMDEBUG = -DMEMDEBUG
  else
    MEMDEBUG =
  endif
  ifeq ($(GEOMDEBUG),yes)
    GEOMDEBUG = -DGEOMDEBUG
  else
    GEOMDEBUG =
  endif
else
  DEBUG =  -W -Wall -Wno-unused-parameter -pedantic -O3 -funroll-loops -ftree-vectorize
  PROFILE =
  MEMDEBUG =
  GEOMDEBUG =
  NOTHROW =
endif

ifeq ($(MPI),yes)
  MPIFLG = -DMPI $(ZOLTANINC)
  MPICC = mpicc
  MPILIBS = $(ZOLTANLIB)
endif
