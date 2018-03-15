AUTOMAKE_OPTIONS=subdir-objects
MIMIIODIR = ../../src
OS_SPECIFIC_LINKS = @OS_SPECIFIC_LINKS@

if PA_DEP_BUILD

bin_PROGRAMS = mimiio_pa

if DEBUG

AM_CFLAGS = -g3	-O0 -fno-inline -D_DEBUG $(PORTAUDIO_CFLAGS) 
AM_CXXFLAGS = -g3 -O0 -fno-inline -D_DEBUG $(PORTAUDIO_CFLAGS) -std=c++11

mimiio_pa_SOURCES = mimiio_pa.cpp
mimiio_pa_LDADD = $(MIMIIODIR)/.libs/libmimiio.a $(POCO_S_LIBS) $(PORTAUDIO_LIBS) $(OS_SPECIFIC_LINKS) @POCO_LDFLAGS@ -lPocoNetd -lPocoNetSSLd -lPocoFoundationd -lPocoJSONd $(FLAC_LIBS)

else

AM_CFLAGS = -g -O2 $(PORTAUDIO_CFLAGS)
AM_CXXFLAGS = -g -O2 $(PORTAUDIO_CFLAGS) -std=c++11

mimiio_pa_SOURCES = mimiio_pa.cpp
mimiio_pa_LDADD = $(MIMIIODIR)/libmimiio.la $(OS_SPECIFIC_LINKS) $(PORTAUDIO_LIBS) $(FLAC_LIBS) @POCO_LDFLAGS@ -lPocoNet -lPocotNetSSL -lPocoFoundation -lPocoJSON -lPocoCrypto -lPocoXML -lPocoUtil

endif

endif
