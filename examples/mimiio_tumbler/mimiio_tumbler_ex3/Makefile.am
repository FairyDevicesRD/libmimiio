AUTOMAKE_OPTIONS=subdir-objects
MIMIIODIR = ../../../src
OS_SPECIFIC_LINKS = @OS_SPECIFIC_LINKS@

if MIMIXFE_DEP_BUILD

bin_PROGRAMS = mimiio_tumbler_ex3

if DEBUG

AM_CFLAGS = -g3	-O0 -fno-inline -D_DEBUG $(MIMIXFE_CPPFLAGS) -pthread
AM_CXXFLAGS = -g3 -O0 -fno-inline -D_DEBUG $(MIMIXFE_CPPFLAGS) -std=c++11 -pthread

mimiio_tumbler_ex3_SOURCES = mimiio_tumbler_ex3.cpp class/Stream.cpp class/SpeechEvent.cpp
mimiio_tumbler_ex3_LDADD = $(MIMIIODIR)/.libs/libmimiio.a $(POCO_S_LIBS) $(MIMIXFE_LDFLAGS) $(OS_SPECIFIC_LINKS) -lPocoNetd -lPocoNetSSLd -lPocoFoundationd -lPocoJSONd $(FLAC_LIBS) -lmimixfe -lpthread -ltumbler -lasound

else

AM_CFLAGS = -g -O2 $(MIMIXFE_CPPFLAGS) -DFIO_T01
AM_CXXFLAGS = -g -O2 $(MIMIXFE_CPPFLAGS) -std=c++11 -DFIO_T01

mimiio_tumbler_ex3_SOURCES = mimiio_tumbler_ex3.cpp class/Stream.cpp class/SpeechEvent.cpp
mimiio_tumbler_ex3_LDADD = $(MIMIIODIR)/.libs/libmimiio.a $(OS_SPECIFIC_LINKS) $(MIMIXFE_LDFLAGS) $(FLAC_LIBS) -lPocoNet -lPocoNetSSL -lPocoFoundation -lPocoJSON -lmimixfe -lpthread -ltumbler -lasound

endif
endif
