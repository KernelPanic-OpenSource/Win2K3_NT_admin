
TARGETNAME=MsiZap
!INCLUDE ..\..\..\MsiMake.inc

TARGETTYPE=PROGRAM
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0

UMTYPE=console
UMENTRY=$(MSI_ENTRY)

USE_LIBCMT=1

TARGETLIBS=\
        $(SDK_LIB_PATH)\shell32.lib \
        $(SDK_LIB_PATH)\user32.lib \
        $(SDK_LIB_PATH)\msi.lib

C_DEFINES=$(C_DEFINES) -D_EXE

INCLUDES=$(INC_DIR);$(RES_OBJDIR);$(ADMIN_INC_PATH)

SOURCES=..\msizap.cpp \
	..\msizap.rc