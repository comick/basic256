// Constants.h - Constants used throughout the code

// EPSILON represents how far off float comparison may be to be the same
#ifndef EPSILON
    #define EPSILON 0.00000001
#endif

#ifndef M_PI
    #define M_PI 3.14159265
#endif

#ifndef CALLSIG_SUBROUTINE
    #define CALLSIG_SUBROUTINE 0xffff
    #define CALLSIG_FUNCTION 0xfffe
#endif

#ifndef EDITOR_TAB_WIDTH
	#define EDITOR_TAB_WIDTH 4
#endif

// time between sleep interrupt checks
#ifndef SLEEP_GRANULE
	#define SLEEP_GRANULE 2000L
#endif

// command line states that define how the GUI is laid-out and reacts
#ifndef GUISTATENORMAL
	#define GUISTATENORMAL 0
	#define GUISTATERUN 1
	#define GUISTATEAPP 2
#endif

// states of a program
#ifndef RUNSTATESTOP
	#define RUNSTATESTOP 0
	#define RUNSTATERUN 1
	#define RUNSTATEDEBUG 2
	#define RUNSTATESTOPING 3
	#define RUNSTATERUNDEBUG 4
#endif
