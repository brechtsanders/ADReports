ifeq ($(OS),)
OS = $(shell uname -s)
endif
PREFIX = /usr/local
CC   = gcc
CXX  = g++
AR   = ar
LIBPREFIX = lib
LIBEXT = .a
ifeq ($(OS),Windows_NT)
BINEXT = .exe
SOEXT = .dll
else ifeq ($(OS),Darwin)
BINEXT =
SOEXT = .dylib
else
BINEXT =
SOEXT = .so
endif
INCS = -I.
CFLAGS = $(INCS) -Os
CXXFLAGS = $(INCS) -Os
LIBS =
LDFLAGS =
ifeq ($(OS),Darwin)
STRIPFLAG =
else
STRIPFLAG = -s
endif
MKDIR = mkdir -p
RM = rm -f
RMDIR = rm -rf
CP = cp -f
CPDIR = cp -rf

CFLAGS += -DUSE_XLSXIO
CXXFLAGS += -DUSE_XLSXIO
LDFLAGS += -lxlsxio_write
ifdef STATIC
CFLAGS += -DSTATIC
CXXFLAGS += -DSTATIC
LDFLAGS += -static -lzip -lzip -lbz2 -lz
endif

LDAP_FLAGS := 
LDAP_LIBS := -lldap -llber
ifeq ($(OS),Windows_NT)
ifdef USE_WINLDAP
LDAP_FLAGS := -DUNICODE -DUSE_WINLDAP
LDAP_LIBS := -lwldap32
endif
endif

#ifneq ($(OS),Windows_NT)
#SHARED_CFLAGS += -fPIC
#endif
#ifeq ($(OS),Windows_NT)
#CFLAGS += -DUNICODE -DUSE_WINLDAP
#CXXFLAGS += -DUNICODE -DUSE_WINLDAP
#ADREPORTUSERS_SHARED_LDFLAGS += -Wl,--out-implib,$@$(LIBEXT)
#else ifeq ($(OS),Darwin)
#else
#endif
#ifeq ($(OS),Darwin)
#OS_LINK_FLAGS = -dynamiclib -o $@
#else
#OS_LINK_FLAGS = -shared -Wl,-soname,$@ $(STRIPFLAG)
#endif

ADREPORTUSERS_OBJ = ADReportUsers.o ldapconnection.o adformats.o dataoutput.o
ADREPORTUSERS_LDFLAGS = $(LDAP_LIBS)
ADREPORTGROUPS_OBJ = ADReportGroups.o ldapconnection.o adformats.o dataoutput.o
ADREPORTGROUPS_LDFLAGS = $(LDAP_LIBS)
ADREPORTCOMPUTERS_OBJ = ADReportComputers.o ldapconnection.o adformats.o dataoutput.o
ADREPORTCOMPUTERS_LDFLAGS = $(LDAP_LIBS)
CHECKUSERFOLDERS_OBJ = checkuserfolders.o ldapconnection.o adformats.o dataoutput.o
CHECKUSERFOLDERS_LDFLAGS = $(LDAP_LIBS)
ADGETPHOTO_OBJ = ADGetPhoto.o ldapconnection.o adformats.o dataoutput.o
ADGETPHOTO_LDFLAGS = $(LDAP_LIBS)

#ifdef STATICDLL
#ifeq ($(OS),Windows_NT)
## lines below to compile Windows DLLs with no dependancies
#CFLAGS += -DUNICODE -DUSE_WINLDAP
#ADREPORTUSERS_LDFLAGS += -static -lz -lbz2
#endif
#endif

TOOLS_BIN = ADReportUsers$(BINEXT) ADReportGroups$(BINEXT) ADReportComputers$(BINEXT) CheckUserFolders$(BINEXT) ADGetPhoto$(BINEXT)

COMMON_PACKAGE_FILES = README.md COPYING.txt Changelog.txt
SOURCE_PACKAGE_FILES = $(COMMON_PACKAGE_FILES) Makefile *.c *.cpp *.h

default: all

all: tools

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

%.static.o: %.c
	$(CC) -c -o $@ $< $(STATIC_CFLAGS) $(CFLAGS)

%.shared.o: %.c
	$(CC) -c -o $@ $< $(SHARED_CFLAGS) $(CFLAGS)

tools: $(TOOLS_BIN)

ADReportUsers$(BINEXT): $(ADREPORTUSERS_OBJ)
	$(CXX) $(STRIPFLAG) -o $@ $(ADREPORTUSERS_OBJ) $(ADREPORTUSERS_LDFLAGS) $(LDFLAGS)

ADReportGroups$(BINEXT): $(ADREPORTGROUPS_OBJ)
	$(CXX) $(STRIPFLAG) -o $@ $(ADREPORTGROUPS_OBJ) $(ADREPORTGROUPS_LDFLAGS) $(LDFLAGS)

ADReportComputers$(BINEXT): $(ADREPORTCOMPUTERS_OBJ)
	$(CXX) $(STRIPFLAG) -o $@ $(ADREPORTCOMPUTERS_OBJ) $(ADREPORTCOMPUTERS_LDFLAGS) $(LDFLAGS)

CheckUserFolders$(BINEXT): $(CHECKUSERFOLDERS_OBJ)
	$(CXX) $(STRIPFLAG) -o $@ $(CHECKUSERFOLDERS_OBJ) $(CHECKUSERFOLDERS_LDFLAGS) $(LDFLAGS)

ADGetPhoto$(BINEXT): $(ADGETPHOTO_OBJ)
	$(CXX) $(STRIPFLAG) -o $@ $(ADGETPHOTO_OBJ) $(ADGETPHOTO_LDFLAGS) $(LDFLAGS)

install: all
	$(MKDIR) $(PREFIX)/bin
	$(CP) $(TOOLS_BIN) $(PREFIX)/bin/
#	$(MKDIR) $(PREFIX)/include $(PREFIX)/lib $(PREFIX)/bin
#	$(CP) include/*.h $(PREFIX)/include/
#	$(CP) *$(LIBEXT) $(PREFIX)/lib/
#ifeq ($(OS),Windows_NT)
#	$(CP) *$(SOEXT) $(PREFIX)/bin/
#else
#	$(CP) *$(SOEXT) $(PREFIX)/lib/
#endif
#	$(CP) $(TOOLS_BIN) $(PREFIX)/bin/
#ifdef DOXYGEN
#	$(CPDIR) doc/man $(PREFIX)/
#endif

.PHONY: version
version:
	sed -ne "s/^#define\s*ADREPORTS_VERSION_[A-Z]*\s*\([0-9]*\)\s*$$/\1./p" adreports_version.h | tr -d "\n" | sed -e "s/\.$$//" > version

.PHONY: package
package: version
	tar cfJ adreports-$(shell cat version).tar.xz --transform="s?^?adreports-$(shell cat version)/?" $(SOURCE_PACKAGE_FILES)

.PHONY: package
binarypackage: version
	#$(MAKE) PREFIX=binarypackage_temp install USE_WINLDAP=1 STATIC=1
	#tar cfJ "adreports-$(shell cat version)-$(OS).tar.xz" --transform="s?^binarypackage_temp/??" $(COMMON_PACKAGE_FILES) binarypackage_temp/*
	$(MAKE) PREFIX=binarypackage_temp install USE_WINLDAP=1 STATIC=1
	zip -9 -r -j "adreports-$(shell cat version)-$(OS).zip" $(COMMON_PACKAGE_FILES) binarypackage_temp/*
	rm -rf binarypackage_temp

.PHONY: clean
clean:
	$(RM) version *.o $(TOOLS_BIN)

