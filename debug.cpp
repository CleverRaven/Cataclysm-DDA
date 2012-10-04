#include "debug.h"
#include <time.h>


#if !(defined _WIN32 || defined WINDOWS)
#include <execinfo.h>
#include <stdlib.h>
#endif

// Static defines                                                   {{{1
// ---------------------------------------------------------------------

static int debugLevel = DL_ALL;
static int debugClass = DC_ALL;

// Normal functions                                                 {{{1
// ---------------------------------------------------------------------

void setupDebug()
{
 int level = 0;

#ifdef DEBUG_INFO
 level |= D_INFO;
#endif

#ifdef DEBUG_WARNING
 level |= D_WARNING;
#endif

#ifdef DEBUG_ERROR
 level |= D_ERROR;
#endif

#ifdef DEBUG_PEDANTIC_INFO
 level |= D_PEDANTIC_INFO;
#endif

 if( level != 0 )
  limitDebugLevel(level);

 int cl = 0;

#ifdef DEBUG_ENABLE_MAIN
 cl |= D_MAIN;
#endif

#ifdef DEBUG_ENABLE_MAP
 cl |= D_MAP;
#endif

#ifdef DEBUG_ENABLE_MAP_GEN
 cl |= D_MAP_GEN;
#endif

#ifdef DEBUG_ENABLE_GAME
 cl |= D_GAME;
#endif

 if( cl != 0 )
  limitDebugClass(cl);
}

void limitDebugLevel( int i )
{
 dout() << "Set debug level to: " << (DebugLevel)i;
 debugLevel = i;
}

void limitDebugClass( int i )
{
 dout() << "Set debug class to: " << (DebugClass)i;
 debugClass = i;
}

// Debug only                                                       {{{1
// ---------------------------------------------------------------------

#ifdef ENABLE_LOGGING

#define TRACE_SIZE 20

void* tracePtrs[TRACE_SIZE];

// Debug Includes                                                   {{{2
// ---------------------------------------------------------------------

#include <fstream>
#include <streambuf>

// Null OStream                                                     {{{2
// ---------------------------------------------------------------------

struct NullBuf : public std::streambuf
{
 NullBuf() {}
 int overflow(int c) { return c; }
};

struct DebugFile
{
 DebugFile();
 ~DebugFile();

 std::ofstream & currentTime();
 std::ofstream file;
};

static NullBuf nullBuf;
static std::ostream nullStream(&nullBuf);

// DebugFile OStream Wrapper                                        {{{2
// ---------------------------------------------------------------------

static DebugFile debugFile;

DebugFile::DebugFile()
{
 file.open("data/debug.txt", std::ios::out | std::ios::app );
 file << "\n\n-----------------------------------------\n";
 currentTime() << " : Starting log.";
}

DebugFile::~DebugFile()
{
 file << "\n";
 currentTime() << " : Log shutdown.\n";
 file << "-----------------------------------------\n\n";
 file.close();
}

// OStream Operators                                                {{{2
// ---------------------------------------------------------------------

std::ostream & operator<<(std::ostream & out, DebugLevel lev)
{
 if( lev != DL_ALL )
 {
  if( lev & D_INFO )
   out << "INFO ";
  if( lev & D_WARNING )
   out << "WARNING ";
  if( lev & D_ERROR )
   out << "ERROR ";
  if( lev & D_PEDANTIC_INFO )
   out << "PEDANTIC ";
 }
 return out;
}

std::ostream & operator<<(std::ostream & out, DebugClass cl)
{
 if( cl != DC_ALL )
 {
  if( cl & D_MAIN )
   out << "MAIN ";
  if( cl & D_MAP )
   out << "MAP ";
 }
 return out;
}

std::ofstream & DebugFile::currentTime()
{
 struct tm *current;
 time_t now;

 time(&now);
 current = localtime(&now);

 file << current->tm_hour << ":" << current->tm_min << ":" <<
  current->tm_sec;
 return file;
}

std::ostream & dout(DebugLevel lev,DebugClass cl)
{
 if( (lev & debugLevel) && (cl & debugClass) )
 {
  debugFile.file << std::endl;
  debugFile.currentTime() << " ";
  if( lev != debugLevel )
   debugFile.file << lev;
  if( cl != debugClass )
   debugFile.file << cl;
  debugFile.file << ": ";

  // Backtrace on error.
#if !(defined _WIN32 || defined WINDOWS)
  if( lev == D_ERROR )
  {
   int count = backtrace( tracePtrs, TRACE_SIZE );
   char** funcNames = backtrace_symbols( tracePtrs, count );
   for(int i = 0; i < count; ++i)
    debugFile.file << "\n\t(" << funcNames[i] << "), ";
   debugFile.file << "\n\t";
   free(funcNames);
  }
#endif

  return debugFile.file;
 }
 return nullStream;
}

// Non Debug Only                                                   {{{1
// ---------------------------------------------------------------------

#else // If NOT defined ENABLE_LOGGING


DebugVoid dout(DebugLevel,DebugClass)
{ return DebugVoid(); }

#endif // END If NOT defined ENABLE_LOGGING

// vim:tw=72:sw=1:fdm=marker:fdl=0:
