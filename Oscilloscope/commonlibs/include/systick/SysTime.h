#ifndef SYSTIME_H
#define SYSTIME_H

#include <inttypes.h>

typedef uint64_t (*MillisGetter)();

class SysTime
{
protected:
	static MillisGetter mgetter;

public:
	static void setMillisGetter(MillisGetter getter) { mgetter = getter; }
	static uint64_t getMillis() {return mgetter();};
};

#endif /* SYSTIME_H */
