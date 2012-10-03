#include "debug.h"

#include <time.h>

static int debugLevel = DL_ALL;

void limitDebugLevel( int i )
{
 dout() << "Set debug level to: " << (DebugLevel)i;
 debugLevel = i;
}

static int debugClass = DC_ALL;

void limitDebugClass( int i )
{
 dout() << "Set debug class to: " << (DebugClass)i;
 debugClass = i;
}

#ifndef NDEBUG

#include <fstream>
#include <streambuf>

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
static DebugFile debugFile;

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
 if( (lev & debugLevel) && (cl && debugClass) )
 {
  debugFile.file << std::endl;
  debugFile.currentTime() << " ";
  if( lev != debugLevel )
   debugFile.file << lev;
  if( cl != debugClass )
   debugFile.file << cl;
  return debugFile.file << ": ";
 }
 return nullStream;
}

#else
DebugVoid dout(DebugLevel,DebugClass)
{ return DebugVoid(); }
#endif
