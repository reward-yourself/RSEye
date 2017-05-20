// =====================================================================================
//       Filename:  global.h
//    Description:  define global variables for rseye.c
//        Created:  05/18/2017 02:08:50 PM
//         Author:  Hoang-Ngan Nguyen (), zhoangngan@gmail.com
// =====================================================================================

#include <X11/Xlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <X11/extensions/Xrender.h>

#define MAX_CML_ARGS  11
#define F_NAME_MAX   120

enum {
  M_DIM,
  M_BRG,
  S_DIM,
  S_BRG,
  NUMCOLS
};

typedef int mytime_t;

typedef struct {
  int px, py, radius;
} Ball;

unsigned int postponeNumSmall = 0;
unsigned int postponeNumLarge = 0;

unsigned int reloadRequested = 0;

static mytime_t workTime    = 20; // (minutes)
static mytime_t maxWorkTime = 60; // (minutes)
static mytime_t smallBreak  = 60; // (seconds)
static mytime_t largeBreak  = 8;  // (minutes)

char logFileName[F_NAME_MAX] = "/tmp/rseye.log";
FILE * logFile = NULL;

static const double angle    = M_PI/5.0;

