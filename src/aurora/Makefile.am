include $(top_srcdir)/Makefile.common

SUBDIRS = nwscript

noinst_LTLIBRARIES = libaurora.la

noinst_HEADERS = types.h \
                 error.h \
                 util.h \
                 archive.h \
                 aurorafile.h \
                 keyfile.h \
                 biffile.h \
                 erffile.h \
                 rimfile.h \
                 ndsrom.h \
                 zipfile.h \
                 resman.h \
                 talktable.h \
                 talkman.h \
                 ssffile.h \
                 2dafile.h \
                 2dareg.h \
                 locstring.h \
                 gfffile.h \
                 gffstructs.h \
                 dlgfile.h \
                 lytfile.h \
                 visfile.h \
                 pefile.h \
                 herffile.h

libaurora_la_SOURCES = error.cpp \
                       util.cpp \
                       archive.cpp \
                       aurorafile.cpp \
                       keyfile.cpp \
                       biffile.cpp \
                       erffile.cpp \
                       rimfile.cpp \
                       ndsrom.cpp \
                       zipfile.cpp \
                       resman.cpp \
                       talktable.cpp \
                       talkman.cpp \
                       ssffile.cpp \
                       2dafile.cpp \
                       2dareg.cpp \
                       locstring.cpp \
                       gfffile.cpp \
                       gffstructs.cpp \
                       dlgfile.cpp \
                       lytfile.cpp \
                       visfile.cpp \
                       pefile.cpp \
                       herffile.cpp

libaurora_la_LIBADD = nwscript/libnwscript.la
