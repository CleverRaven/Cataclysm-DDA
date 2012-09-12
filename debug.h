#ifndef DEBUGLOG_H
#define DEBUGLOG_H
#include <fstream>

struct DebugLog
{
	DebugLog() {
		fout.open("logg.txt", std::ios_base::app | std::ios_base::out);
	}
	~DebugLog() {
		fout.close();
	}

	template <class T>
	DebugLog& operator<<(T& t) {
		fout << t;
		return *this;
	}
private:
	std::ofstream fout;
};

#endif
