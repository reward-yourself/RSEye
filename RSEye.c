/*
 * =====================================================================================
 *
 *       Filename:  RSEye.c
 *        Version:  1.0
 *        Created:  05/02/2017 08:58:04 AM
 *       Revision:  none
 *       Compiler:  g++
 *         Author:  Hoang-Ngan Nguyen (), zhoangngan@gmail.com
 *    Description:  
 *    - To make it simple, the program will work as follows:
 *      + After x minutes turn the locker on, activate small break.
 *      + Small break time is `y` seconds.
 *      + After `n` number of small breaks, activate large break.
 *        * `n` is set so that `[n*(x + y/60)] = 1`
 *      + Large break time is `t` hours.
 *    - How to deal with system suspend because sleep function from
 *      time.h does not respect system suspend?
 *      + Divide work time into smaller chunks
 *    Compilation: compile with gcc
 *    gcc `pkg-config --cflags -libs` xrender` -lm RSEye.c -o rseye
 *
 * =====================================================================================
 */

#include <X11/Xlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <X11/extensions/Xrender.h>

enum {
  M_DIM, 
  M_BRG, 
  S_DIM, 
  S_BRG, 
  NUMCOLS
};

static const double angle    = M_PI/5.0;

#include "config.h"

void make_ball(int cx, int cy, int radius, int N, XPointFixed tris[])
{
  double a = 2*M_PI/N;
  tris[0].x = cx << 16;
  tris[0].y = cy << 16;
  for (int i = 0; i < N+1; ++i) {
    tris[i+1].x = XDoubleToFixed(radius*cos(a*i)) + tris[0].x;
    tris[i+1].y = XDoubleToFixed(radius*sin(a*i)) + tris[0].y;
  }
  tris[N+1].x = tris[1].x;
  tris[N+1].y = tris[1].y;
}

typedef struct {
  int px, py, radius;
} Ball;

unsigned int 
lockscreen(unsigned int lengthOfBreak, int screen) 
{
  Display *dpy = XOpenDisplay(0);
  assert(dpy);

  XRenderPictFormat *fmt=XRenderFindStandardFormat(
      dpy, PictStandardRGB24);

  int win_h, win_w;
  win_h = DisplayHeight(dpy, screen);
  win_w = DisplayWidth(dpy, screen);

  unsigned int i, ptgrab, kbgrab;

  // Draw a window covering the whole screen
  XSetWindowAttributes wa;
  wa.override_redirect = 1;
  Window root = RootWindow(dpy, screen);
  Window w  = XCreateWindow(
      dpy, root, 0, 0, win_w, win_h, 0, 
      DefaultDepth(dpy, screen), CopyFromParent,
      DefaultVisual(dpy, screen),
      CWOverrideRedirect, &wa
      );

  XRenderPictureAttributes pict_attr;
  pict_attr.poly_edge=PolyEdgeSmooth;
  pict_attr.poly_mode=PolyModeImprecise;

  Picture picture = XRenderCreatePicture(
      dpy, w, fmt, CPPolyEdge|CPPolyMode, &pict_attr
      );
  XRadialGradient radialGradient;
  radialGradient.inner.x = XDoubleToFixed (ballRadius);
  radialGradient.inner.y = XDoubleToFixed (ballRadius);
  radialGradient.inner.radius = XDoubleToFixed (0.0f);
  radialGradient.outer.x = XDoubleToFixed (ballRadius);
  radialGradient.outer.y = XDoubleToFixed (ballRadius);
  radialGradient.outer.radius = XDoubleToFixed (1.1*ballRadius);

  Picture gradientPict[NUMCOLS];
  for (i = 0; i < NUMCOLS; ++i) {
    gradientPict[i] = XRenderCreateRadialGradient(
        dpy, &radialGradient, colorStops, colorList[i], 2
        );
  }

  Ball balls[10];

  double tx, ty;
  for(i=0; i<10; i++) {
      tx = cos(angle*(2.5 - i));
      ty = sin(angle*(2.5 - i));
    balls[i].px= (int)(radius*tx) + win_w/2;
    balls[i].py= (int)(radius*ty) + win_h/2;
    balls[i].radius=(int)ballRadius;
  }

  XMapWindow(dpy, w);

  /* Try to grab mouse pointer *and* keyboard for 100ms, else fail the lock */
  /* The following for-loop is 99.99% from slock.c by suckless.org */
  for (i = 0, ptgrab = kbgrab = -1; i < 4; i++) {
    if (ptgrab != GrabSuccess) {
      ptgrab = XGrabPointer(dpy, root, False,
                            ButtonPressMask | ButtonReleaseMask |
                            PointerMotionMask, GrabModeAsync,
                            GrabModeAsync, None, None, CurrentTime);
    }
    if (kbgrab != GrabSuccess) {
      kbgrab = XGrabKeyboard(dpy, root, True,
                             GrabModeAsync, GrabModeAsync, CurrentTime);
    }

    /* input is grabbed: we can lock the screen */
    if (ptgrab == GrabSuccess && kbgrab == GrabSuccess) {
      XMapRaised(dpy, w);
      XSelectInput(dpy, root, SubstructureNotifyMask);
    }

    /* retry on AlreadyGrabbed but fail on other errors */
    if ((ptgrab != AlreadyGrabbed && ptgrab != GrabSuccess) ||
        (kbgrab != AlreadyGrabbed && kbgrab != GrabSuccess))
      break;

    usleep(25000);
  }

  // Wait for the MapNotify event.
  // Mine does not receive any MapNotify event even with
  // StructureNotifyMask.
  /*for(;;) {*/
    /*XEvent ev;*/
    /*XNextEvent(dpy, &ev);*/
    /*if (ev.type == MapNotify) break; */
  /*}*/

  // count down timer

  XRenderColor bg_color = {
    .red=0xffff, .green=0x37ff, .blue=0x00ff, .alpha=0x7fff
  };
  XRenderFillRectangle(
      dpy, PictOpOver, picture, &bg_color, 0, 0, win_w, win_h
      );

  int colorPick[10];
  XPointFixed tris[N+2];

  int min = lengthOfBreak/60;
  int sec = lengthOfBreak%60;
  int set;
  int tmp;
  time_t startTime, endTime;
  startTime = time(NULL);
  unsigned int actualTime;
  while (min | sec) {
    set = 0;
    tmp = sec;
    while (tmp) {
      if (tmp & 1) {
        colorPick[set] = 3;
      } else colorPick[set] = 2;
      set++;
      tmp >>= 1;
    }
    while (set < 6) {
      colorPick[set] = 2;
      set++;
    }
    set = 9;
    tmp = min;
    while (tmp) {
      if (tmp & 1) {
        colorPick[set] = 1;
      } else colorPick[set] = 0;
      set--;
      tmp >>= 1;
    }
    while (set > 5) {
      colorPick[set] = 0;
      set--;
    }
    for(i=0; i<10; i++) {
        make_ball(
            balls[i].px, balls[i].py, balls[i].radius, N, tris
            );
        XRenderCompositeTriFan(
            dpy, PictOpOver, gradientPict[colorPick[i]],
          picture, 0, ballRadius, ballRadius, tris, N+2
          );
    }
    XFlush(dpy);
    sleep(1);
    if (sec==0) {
      min--;
      sec=60;
    }
    sec--;
    endTime = time(NULL);

    // If the system happened to sleep during break time, count that
    // sleeping time as well.
    actualTime = (unsigned int)difftime(endTime, startTime);
    if (actualTime > lengthOfBreak) {
      XRenderFreePicture(dpy, picture);
      XCloseDisplay(dpy);
      return actualTime - lengthOfBreak;
    }
  }

  XRenderFreePicture(dpy, picture);
  XCloseDisplay(dpy);
  return 0;
}

/* The following function is 99.99% from slock.c on suckless.org */
static void 
die(const char *errmsg, ...) 
{
  va_list ap;
  va_start(ap, errmsg);
  vfprintf(stderr, errmsg, ap);
  va_end(ap);
  fprintf(stderr, "\tUsage: \n\t\trseye -k -w worktime -s smallbreak -l largebreak -o logfile\n\n");
  exit(1);
}

int
main ( int argc, char *argv[] )
{
  FILE *fid = NULL;

  // Check if kill all
  if (argc == 2) {
    argv++;
    if (argv[0][1] == '\0') die("\n\tError: Invalid arguments!\n");
    if ((argv[0][0] != '-') || (argv[0][1] != 'k') || (argv[0][2] != '\0'))   // if not '-k' abort
      die("\n\tError: Invalid arguments!\n");
  }

  // Check if it is already running.
  fid = popen("ps ax | awk '$5 ~ /[r]seye/'", "r");
  if (fid != NULL) {
    FILE *out = stderr;
    char str[100];
    int count = 0;
    if (argc == 2) out = stdout;
    fprintf(out, "The following instances of rseye are running:\n");
    while (fgets(str, 100, fid)) {
      fprintf(out, "%s", str);
      count++;
    }
    pclose(fid);
    fid = NULL;
    if (argc == 2) {
      printf("Do you want to kill all instances?[yn]: ");
      char c = getchar();
      if (c == 'n') die("\n\tAborting current process!\n");
      system("ps ax | awk '$5 ~ /[r]seye/ { system(\"kill -9 \"$1) }'");
    }
    if (count > 1) {
      die("\n\tError: Another instance of rseye is already running! Abort now!\n");
    }
  }

  // check arguments
  if (~argc & 1) die("\n\tError: Invalid number of arguments!\n");
  if (argc > 9) {
    die("\n\tError: Too many arguments!\n");
  }
  while (argc > 2) {
    argc--; argv++;
    if (argv[0][0] != '-') die("\n\tError: Something is wrong with the arguments!\n");
    if (argv[0][1] == '\0') die("\n\tError: Something is wrong with the arguments!\n");
    if (argv[0][2] != '\0') {
      if ((fid != NULL) && (fid != stderr)) fclose(fid);
      die("\n\tError: Invalid arguments!\n");
    }

    char argv_ = argv[0][1];
    argv++, argc--;

    int val = atoi(argv[0]);
    if ((val <= 0) && argv_ != 'o') {
      if ((fid != NULL) && (fid != stderr)) fclose(fid);
      die("\n\tError: Time values must be positive integers!\n");
    }

    switch (argv_) {
      case 'w':
        workTime = val;
        break;
      case 's':
        smallBreak = val;
        break;
      case 'l':
        largeBreak = val;
        break;
      case 'o':
        fid = fopen(argv[0], "a");
        if (fid == NULL) die("\n\tError: Cannot open file %s!\n", argv[0]);
        setbuf(fid, NULL);
        break;
      default:
        if ((fid != NULL) && (fid != stderr)) fclose(fid);
        die("\n\tError: Invalid arguments!\n");
    }
  }
  if (fid == NULL) {
    fid = stderr;
  }
  time_t endTime;
  time_t startTime = time(NULL);
  struct tm tm = *localtime(&startTime);
  fprintf(fid, "\nProgram starts on %s",  asctime(&tm));
  fprintf(fid, "  WorkTime   = %d (minutes).\n", workTime);
  fprintf(fid, "  SmallBreak = %d (seconds).\n", smallBreak);
  fprintf(fid, "  LargeBreak = %d (minutes).\n", largeBreak);
  fprintf(fid, "\n**** Start Logging! ****\n");

  unsigned int smallBreakCounter = 0;
  unsigned int largeBreakCounter = 0;
  const unsigned int numberSubWorkTime = 4;
  const unsigned int subWorkTime = workTime*(60 / numberSubWorkTime);
  unsigned int i = 0;
  unsigned int suspendTime = 0;

  // Predefined sleep function does account for system sleep which is
  // undesired. So, we need to use CLOCK_REALTIME.
  struct timespec tp;

  unsigned int workTimeNumber = 0;
  while (True) {
    workTimeNumber++;
    suspendTime = 0;

    // Working time: program sleeps for workTime minutes.
    // workTime = subWorkTime * numberSubWorkTime
    startTime = time(NULL);
    tm = *localtime(&startTime);
    fprintf(fid, "\nWorkTimeNumber %d  started on %s", workTimeNumber, asctime(&tm));
    i = 0;
    while (i < numberSubWorkTime) {
      startTime = time(NULL);
      tm = *localtime(&startTime);
      fprintf(fid, "   SubWorkTime %d  started on %s", i+1, asctime(&tm));
      /*sleep(subWorkTime);*/
      clock_gettime(CLOCK_REALTIME, &tp);
      tp.tv_sec += subWorkTime;
      clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &tp, NULL);
      endTime = time(NULL);
      tm = *localtime(&endTime);
      fprintf(fid, "   SubWorkTime %d finished on %s", i+1, asctime(&tm));
      suspendTime = (unsigned int)difftime(endTime, startTime) - subWorkTime;

      // If the system has been suspended for more than
      // smallBreak seconds, end the current work time.
      if (suspendTime > smallBreak)
        break;
      else
        i++;
    }

    // If the system has been suspended for longer than largeBreak, we
    // assume that largeBreak is done.
    if (suspendTime > largeBreak*60) {
      fprintf(fid, "   SubWorkTime %d: the system has slept for %d hours %d minutes %d seconds which is more than largeBreak of %d minutes.\n", i+1, suspendTime/3600, suspendTime/60, suspendTime%60, largeBreak);
      fprintf(fid, "WorkTimeNumber %d finished on %s", workTimeNumber, asctime(&tm));
      smallBreakCounter = 0;
      continue;
    }

    fprintf(fid, "WorkTimeNumber %d finished on %s", workTimeNumber, asctime(&tm));

    // If the system has not been suspended for two long, continue as if
    // the system has never been suspended.
    smallBreakCounter++;

    if (smallBreakCounter == 3) {
      largeBreakCounter++;
      smallBreakCounter = 0;
      fprintf(fid, "\nLarge break number %d: let's take a break for %d minutes!\n", largeBreakCounter, largeBreak);

      // Take a large break
      suspendTime = lockscreen(largeBreak*60, 0);
      if (suspendTime > 0) {
        fprintf(fid, "During large break, the system has slept for %d hours %d minutes %d seconds.\n", suspendTime/3600, suspendTime/60, suspendTime%60);
        smallBreakCounter = 0;
      }
    } else {
      fprintf(fid, "\nSmall break number %d(%d): let's take a break for %d seconds!\n", smallBreakCounter, largeBreakCounter, smallBreak);
      // Take a small break
      suspendTime = lockscreen(smallBreak, 0);
      if (suspendTime > 0) {
        fprintf(fid, "During small break, the system has slept for %d hours %d minutes %d seconds.\n", suspendTime/3600, suspendTime/60, suspendTime%60);
        if (suspendTime > largeBreak*60 - smallBreak) {
          smallBreakCounter = 0;
        }
      }
    }
  }
  if ((fid != NULL) && (fid != stderr)) {
    fclose(fid);
  }
  return 0;
}        /* ----------  end of function main  ---------- */
