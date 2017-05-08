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
 *    gcc `pkg-config --cflags -libs` xrender` RSEye.c -o rseye
 *
 * =====================================================================================
 */

#include <X11/Xlib.h>
#include <stdio.h>
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
  radialGradient.outer.radius = XDoubleToFixed (1.3*ballRadius);

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
  unsigned int suspendTime;
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
    suspendTime = (unsigned int)difftime(endTime, startTime);
    if (suspendTime > lengthOfBreak) {
      fprintf(stderr, "The system has slept for longer than the requested break time of %d seconds.\n", lengthOfBreak);
      XRenderFreePicture(dpy, picture);
      XCloseDisplay(dpy);
      return suspendTime - lengthOfBreak;
    }
  }

  XRenderFreePicture(dpy, picture);
  XCloseDisplay(dpy);
  return 0;
}

int
main ( int argc, char *argv[] )
{
  time_t startTime = time(NULL);
  struct tm tm = *localtime(&startTime);
  fprintf(stderr, "Program starts on %s\n",  asctime(&tm));
  unsigned int smallBreakCounter = 0;
  const unsigned int numberSubWorkTime = 4;
  const unsigned int subWorkTime = workTime*(60 / numberSubWorkTime);
  const unsigned int maxSuspendTime = 60;
  unsigned int i = 0;
  unsigned int suspendTime = 0;

  int run = 0;
  while (run < 25) {
    run++;

    suspendTime = 0;

    // Working time: program sleeps for workTime minutes.
    // workTime = subWorkTime * numberSubWorkTime
    i = 0;
    while (i < numberSubWorkTime) {
      startTime = time(NULL);
      sleep(subWorkTime);
      suspendTime = (unsigned int)difftime(time(NULL), startTime) - subWorkTime;

      // If the system has been suspended for more than
      // maxSuspendTime seconds, end the current break time.
      if (suspendTime > maxSuspendTime)
        break;
      else
        i++;
    }

    // If the system has been suspended for longer than largeBreak, we
    // assume that largeBreak is done.
    if (suspendTime > largeBreak*60) {
      fprintf(stderr, "Round %d, the system has slept for more than largeBreak which is %d minutes.\n", i, largeBreak);
      smallBreakCounter = 0;
      continue;
    }

    // If the system has not been suspended for two long, continue as if
    // the system has never been suspended.
    if (i == numberSubWorkTime) {
      smallBreakCounter++;
    }

    if (smallBreakCounter == 3) {
      smallBreakCounter = 0;
      fprintf(stderr, "Starting large break and reset smallBreakCounter!\n");

      // Take a large break
      suspendTime = lockscreen(largeBreak*60, 0);
      if (suspendTime > 0) {
        fprintf(stderr, "During large break, the system has slept for %d seconds.\n", suspendTime);
        smallBreakCounter = 0;
      }
    } else {
      fprintf(stderr, "Starting small break number %d!\n", smallBreakCounter);
      // Take a small break
      suspendTime = lockscreen(smallBreak, 0);
      if (suspendTime > largeBreak*60 - smallBreak) {
        fprintf(stderr, "During small break, the system has slept longer than largeBreak, %d seconds.\n", suspendTime);
        smallBreakCounter = 0;
      } else if (suspendTime > 0) {
        fprintf(stderr, "During small break, the system has slept for %d seconds.\n", suspendTime);
        continue;
      }
    }
  }
  return 0;
}        /* ----------  end of function main  ---------- */
