/*
 * =====================================================================================
 *
 *       Filename:  RSEye.c
 *        Version:  0.1
 *        Created:  05/02/2017 08:58:04 AM
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

#include "global.h"
#include "config.h"

/* forward declarations */
int kbhit();
void make_ball(int cx, int cy, int radius, int N, XPointFixed tris[]);
mytime_t lockscreen(mytime_t lengthOfBreak, int screen);
static void die(const char *errmsg, ...);
void signal_handler(int sig);
void reate_pid();
void change_logfile(const char * path);
void load_config();
void check_arguments(int argc, char **argv);
int str_equal(const char * str1, const char * str2);

// non-blocking check if keyboard is hit
// https://linux.die.net/man/2/select
int kbhit() 
{
  fd_set fds;
  struct timeval tv = {0, 0};
  FD_ZERO(&fds);
  FD_SET(0, &fds);
  select(1, &fds, NULL, NULL, &tv);
  return FD_ISSET(0, &fds);
}

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

mytime_t
lockscreen(mytime_t lengthOfBreak, int screen)
{
  Display *dpy = XOpenDisplay(NULL);
  time_t startTime, endTime;
  mytime_t actualTime;
  if (dpy == NULL) {
    fprintf(logFile, "Cannot connect to X server! Sleep for %d seconds now!\n", lengthOfBreak);
    startTime = time(NULL);
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    tp.tv_sec += lengthOfBreak;
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &tp, NULL);
    endTime = time(NULL);

    // If the system happened to sleep during break time, count that
    // sleeping time as well.
    actualTime = (mytime_t)difftime(endTime, startTime);
    return actualTime - lengthOfBreak;
  }

  XRenderPictFormat *fmt=XRenderFindStandardFormat(
      dpy, PictStandardRGB24);

  int win_h, win_w;
  win_h = DisplayHeight(dpy, screen);
  win_w = DisplayWidth(dpy, screen);

  mytime_t i, ptgrab, kbgrab;

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

  // count down timer
  XRenderFillRectangle(
      dpy, PictOpOver, picture, &bg_color, 0, 0, win_w, win_h
      );

  int colorPick[10];
  XPointFixed tris[N+2];

  int min = lengthOfBreak/60;
  int sec = lengthOfBreak%60;
  int set;
  int tmp;
  startTime = time(NULL);
  XEvent ev;
  while (min | sec) {
    set = 0;
    tmp = sec;
    while (tmp) {
      if (tmp & 1) {
        colorPick[set] = 3;
      } else 
        colorPick[set] = 2;
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
    actualTime = (mytime_t)difftime(endTime, startTime);
    if (actualTime >= lengthOfBreak) {
      XRenderFreePicture(dpy, picture);
      XCloseDisplay(dpy);
      return actualTime - lengthOfBreak;
    }
    if (XCheckWindowEvent(dpy, root, KeyPressMask, &ev)) {
      KeySym key = XLookupKeysym(&ev.xkey, 0);
      if (actualTime > minBreak)
        if ((key == XK_q) || (key == XK_Escape)) {
          endTime = time(NULL);
          actualTime = (mytime_t)difftime(endTime, startTime);
          XRenderFreePicture(dpy, picture);
          XCloseDisplay(dpy);
          return actualTime - lengthOfBreak;
        }
      if (key == XK_r) reloadRequested = 1;
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
  vfprintf(logFile, errmsg, ap);
  va_end(ap);
  fprintf(logFile, "Usage: \n\t\trseye -w worktime -s smallbreak -l largebreak -o logfile\n\n");
  fprintf(logFile, "\t-w  worktime   \tThe time in minutes between consecutive breaks. Default value is 20 minutes.\n");
  fprintf(logFile, "\t-s  smallbreak \tThe length in seconds of a small break. Default value is 60 seconds.\n");
  fprintf(logFile, "\t-l  largebreak \tThe length in minutes of a large break. Default value is 8 minutes.\n");
  fprintf(logFile, "\t-o  logfile    \tRecord starting and ending time for working times and breaks. Default value is /tmp/rseye.log.\n");
  if (logFile != stderr) fclose(logFile);
  exit(1);
}

void
signal_handler(int sig)
{
  signal(sig, SIG_IGN);
  FILE * fid = NULL;
  char pidstr[10] = "SIG";
  switch (sig) {
    case SIGINT:
      pidstr[3] = 'I';
      pidstr[4] = 'N';
      pidstr[5] = 'T';
      pidstr[6] = '\0';
      break;
    case SIGTERM:
      pidstr[3] = 'T';
      pidstr[4] = 'E';
      pidstr[5] = 'R';
      pidstr[6] = 'M';
      pidstr[7] = '\0';
      break;
    case SIGQUIT:
      pidstr[3] = 'Q';
      pidstr[4] = 'U';
      pidstr[5] = 'I';
      pidstr[6] = 'T';
      pidstr[7] = '\0';
      break;
  }
  switch (sig) {
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
      fprintf(logFile, "\nAborting rseye (pid = %u) due to %s signal!\n", getpid(), pidstr);
      if (logFile != stderr) fclose(logFile);
      fid = NULL;
      fid = fopen("/tmp/rseye.pid", "r");
      if (fid != NULL) {
        fgets(pidstr, 10, fid);
        fclose(fid);
        if (getpid() == atoi(pidstr)) unlink("/tmp/rseye.pid");
      }
      signal(sig, signal_handler);
      exit(0);
      break;
  }
  signal(sig, signal_handler);
}

void
create_pid()
{
  FILE * fid = NULL;

  // Check if it is already running.
  char pidstr[10];
  fid = fopen("/tmp/rseye.pid", "r");
  if (fid != NULL) {
    fgets(pidstr, 10, fid);
    fclose(fid);
    fid = NULL;
    printf("\nAnother instance of rseye (pid = %s) has already been running!\n", pidstr);
    printf("Choose the following options (timeout is 4 seconds):\n");
    printf("k      \tkill the current process only\n");
    printf("a      \tkill it and the current process\n");
    printf("default\tkill it and continue with the current process\n");

    // non-blocking input
    char c = 'y';
    /* This will suspend the program if put into background from a terminal */
    /*fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);*/ 
    sleep(4);
    /* This will not. */
    if (kbhit()) read(0, &c, sizeof(char));

    if (c == 'k') die("\nAborting current process!\n");
    pid_t pid = getpid();
    fprintf(logFile, "\nrseye (pid = %u): Sending SIGTERM to rseye (pid = %s)\n", pid, pidstr);
    pid = 0;
    int i = 0;
    while (pidstr[i] != '\0') {
      pid = pid*10 + (pidstr[i] - '0');
      i++;
    }
    kill(pid, SIGTERM);
    unlink("/tmp/rseye.pid");   /* delete pid file */
    if (c == 'a') die("\nAborting current process!\n");
  }

  // write pid to pid file
  fid = fopen("/tmp/rseye.pid", "w");
  if (fid == NULL) die("Cannot create pid file /tmp/rseye.pid! Abort now!\n");
  sprintf(pidstr, "%u", getpid());
  fputs(pidstr, fid);
  fclose(fid);
}

void
change_logfile(const char * path)
{
  FILE * flog = NULL;
  if (str_equal(path, "stderr")) 
    flog = stderr;
  else 
    flog = fopen(path, "a");
  if (flog == NULL) {
    fprintf(logFile, "Error: Cannot open log file %s!\nFall back to %s!\n", path, logFileName);
    return;
  }
  if (flog != stderr) setbuf(flog, NULL);
  time_t startTime = time(NULL);
  struct tm tm = *localtime(&startTime);
  fprintf(logFile, "%slog file is changed to: \n%s\n", asctime(&tm), path);
  if (logFile != stderr) fclose(logFile);
  logFile = flog;
  fprintf(logFile, "%slog file is changed from: \n%s\n", asctime(&tm), logFileName);
  int i = 0;
  while (path[i] != '\0') {
    logFileName[i] = path[i];
    i++;
  }
  logFileName[i] = '\0';
}

void
check_arguments(int argc, char **argv)
{
  if (~argc & 1) die("\nError: Invalid number of arguments!\n");
  if (argc > MAX_CML_ARGS) {
    die("\nError: Too many arguments!\n");
  }
  while (argc > 2) {
    argc--; argv++;
    if (argv[0][0] != '-') die("\nError: Something is wrong with the arguments!\n");
    if (argv[0][1] == '\0') die("\nError: Something is wrong with the arguments!\n");
    if (argv[0][2] != '\0') die("\nError: Invalid arguments!\n");

    char argv_ = argv[0][1];
    argv++, argc--;

    int val = atoi(argv[0]);
    if ((val <= 0) && argv_ != 'o') {
      die("\nError: Time values must be positive integers!\n");
    }

    switch (argv_) {
      case 'w':
        workTime = val;
        if (workTime > 30) workTime = 30;
        workTime = ((workTime + 4)/5)*5;
        break;
      case 's':
        smallBreak = val;
        if (smallBreak > 120) smallBreak = 120;
        if (smallBreak < minBreak) smallBreak = minBreak;
        break;
      case 'l':
        largeBreak = val;
        if (largeBreak < 5) largeBreak = 5;
        break;
      case 'm':
        maxWorkTime = val;
        if (maxWorkTime > 180) maxWorkTime = 180;
        if (maxWorkTime <  60) maxWorkTime =  60;
        if (maxWorkTime < workTime) maxWorkTime = workTime;
        break;
      case 'o':
        if(!str_equal(argv[0], logFileName)) change_logfile(argv[0]);
        break;
      default:
        die("\nError: Invalid arguments!\n");
    }
  }
}

void
load_config() 
{
  static unsigned int reloaded = 0;

  char str[F_NAME_MAX] = "/.rseyerc";
  char * home = getenv("HOME"); /* cannot free home because it points to environment variable */
  int i = 0, len = 0, strlen = 0;
  while (home[len] != '\0') len++;
  if (len + 10 > F_NAME_MAX) {
    fprintf(logFile, "Error: Cannot load config -- $HOME/.rseyerc path is too long (len($HOME/.rseyerc) = %d).\n", len + 10);
    fprintf(logFile, "Please increase F_NAME_MAX in global.h and recompile!\n");
    return;
  }
  for (i = 0; i < 10; ++i) {
    str[i + len] = str[i];
  }
  for (i = 0; i < len; ++i) {
    str[i] = home[i];
  }

  char * path = str;
  FILE * fid = fopen(path, "r");
  if (fid == NULL) {
    perror("Cannot load config file");
    return;
  }
  mytime_t tmp;
  char c;
  printf("Home is %s\n", home);
  while (fgets(str, F_NAME_MAX, fid) != NULL) {
    if (str[0] == 'o') { // ^o filename
      strlen = 0;
      while (str[strlen] != '\0') strlen++;
      if (str[strlen - 1] == '\n') str[--strlen] = '\0';
      strlen -= 2;
      path = str + 2;
      printf("path is %s\n", path);
      if (str[2] == '~') {
        strlen--;
        if (len + strlen + 1 > F_NAME_MAX) {
          fprintf(logFile, "Error: Path is too long (len(Path) = %d).\n Please increase F_NAME_MAX and recompile!\n", len + strlen);
          fprintf(logFile, "Path: %s/%s\n", home, str);
          return;
        }
        // now append $HOME to the beginning of str
        for (int j = 0; j < strlen+1; ++j) {
          str[len + strlen - j] = str[strlen + 3 - j];
        }
        for (int j = 0; j < len; ++j) {
          str[j] = home[j];
        }
        path = str;
      }

      if (!str_equal(path, logFileName)) change_logfile(path);

      if (reloaded) {
        fprintf(logFile, "\nrseye (pid = %u): reloaded values are as follows.\n", getpid());
        fprintf(logFile, "  WorkTime    = %d (minutes).\n", workTime);
        fprintf(logFile, "  SmallBreak  = %d (seconds).\n", smallBreak);
        fprintf(logFile, "  LargeBreak  = %d (minutes).\n", largeBreak);
        fprintf(logFile, "  maxWorkTime = %d (minutes).\n", maxWorkTime);
        fprintf(logFile, "\n**** Start Logging in a new log file! ****\n");
      }

      continue;
    }
    strlen = 0;
    while (str[strlen] != '\0') strlen++;
    if (str[strlen - 1] == '\n') str[--strlen] = '\0';
    while (strlen < 5) {
      str[strlen] = '\0';
      strlen++;
    }
    tmp = 0;
    c = str[2];
    if ((c >= '0') && (c <= '9')) tmp = tmp*10 + (c - '0');
    c = str[3];
    if ((c >= '0') && (c <= '9')) tmp = tmp*10 + (c - '0');
    c = str[4];
    if ((c >= '0') && (c <= '9')) tmp = tmp*10 + (c - '0');
    if (tmp == 0) continue;
    switch (str[0]) {
      case 'w':
        workTime = tmp;
        if (workTime > 30) workTime = 30;
        workTime = ((workTime + 4)/5)*5;
        break;
      case 's':
        smallBreak = tmp;
        if (smallBreak > 120) smallBreak = 120;
        if (smallBreak < minBreak) smallBreak = minBreak;
        break;
      case 'l':
        largeBreak = tmp;
        if (largeBreak < 5) largeBreak = 5;
        break;
      case 'm':
        maxWorkTime = tmp;
        if (maxWorkTime > 180) maxWorkTime = 180;
        if (maxWorkTime <  60) maxWorkTime =  60;
        if (maxWorkTime < workTime) maxWorkTime = workTime;
        break;
    }
  }
  fclose(fid);
  reloaded = 1;
}

int
str_equal(const char * str1, const char * str2)
{
  int i = 0;
  while (str1[i] == str2[i]) {
    if (str1[i] == '\0') break;
    if (str2[i] == '\0') break;
    i++; /* By moving i++ to the end, empty strings are equal. */
  }
  return (str1[i] == str2[i]);
}

int
main ( int argc, char *argv[] )
{
  /* install signal handlers */
  signal(SIGINT , signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGQUIT, signal_handler);

  // initialize logFile
  logFile = fopen(logFileName, "a");
  if (logFile == NULL) {
    logFile = stderr;
    sprintf(logFileName, "stderr");
    fprintf(logFile, "Error: Cannot open log file /tmp/rseye.log!\nFall back to stderr!\n");
  } else {
    setbuf(logFile, NULL);
  }
  fprintf(stderr, "Log file is %s\n", logFileName);

  // load values from $HOME/.rseyerc if any
  load_config();

  // check arguments
  check_arguments(argc, argv);

  // check if another instance has already been running and create pid file
  create_pid();

  time_t endTime;
  time_t startTime = time(NULL);
  struct tm tm = *localtime(&startTime);
  fprintf(logFile, "\nrseye (pid = %u) starts on %s", getpid(), asctime(&tm));
  fprintf(logFile, "  WorkTime    = %d (minutes).\n", workTime);
  fprintf(logFile, "  SmallBreak  = %d (seconds).\n", smallBreak);
  fprintf(logFile, "  LargeBreak  = %d (minutes).\n", largeBreak);
  fprintf(logFile, "  maxWorkTime = %d (minutes).\n", maxWorkTime);
  fprintf(logFile, "\n**** Start Logging! ****\n");

  const mytime_t subWorkTime = 300;
  mytime_t suspendTime = 0;

  unsigned int numberSubWorkTime = workTime*60/subWorkTime;
  unsigned int maxWorkTimeNum = maxWorkTime/workTime;
  unsigned int smallBreakCounter = 0;
  unsigned int largeBreakCounter = 0;
  unsigned int i = 0;

  // Predefined sleep function does account for system sleep which is
  // undesired. So, we need to use CLOCK_REALTIME.
  struct timespec tp;

  mytime_t workTimeNumber = 0;
  while (True) {
    workTimeNumber++;
    suspendTime = 0;

    // Working time: program sleeps for workTime minutes.
    // workTime = subWorkTime * numberSubWorkTime
    startTime = time(NULL);
    tm = *localtime(&startTime);
    fprintf(logFile, "\nWorkTimeNumber %d  started on %s", workTimeNumber, asctime(&tm));
    i = 0;
    while (i < numberSubWorkTime) {
      startTime = time(NULL);
      tm = *localtime(&startTime);
      fprintf(logFile, "   SubWorkTime %d  started on %s", i+1, asctime(&tm));
      /*sleep(subWorkTime);*/
      clock_gettime(CLOCK_REALTIME, &tp);
      tp.tv_sec += subWorkTime;
      clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &tp, NULL);
      endTime = time(NULL);
      tm = *localtime(&endTime);
      fprintf(logFile, "   SubWorkTime %d finished on %s", i+1, asctime(&tm));
      suspendTime = (mytime_t)difftime(endTime, startTime) - subWorkTime;

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
      fprintf(logFile, "   SubWorkTime %d: the system has slept for %d hours %d minutes %d seconds which is more than largeBreak of %d minutes.\n", i+1, suspendTime/3600, (suspendTime/60)%60, suspendTime%60, largeBreak);
      fprintf(logFile, "WorkTimeNumber %d finished on %s", workTimeNumber, asctime(&tm));
      smallBreakCounter = 0;
      continue;
    }

    fprintf(logFile, "WorkTimeNumber %d finished on %s", workTimeNumber, asctime(&tm));

    // If the system has not been suspended for two long, continue as if
    // the system has never been suspended.
    smallBreakCounter++;
    fprintf(logFile, "smallBreakCounter = %d\n", smallBreakCounter);

    if (smallBreakCounter < maxWorkTimeNum) {
      fprintf(logFile, "\nSmall break number %d(%d): let's take a break for %d seconds!\n", smallBreakCounter, largeBreakCounter, smallBreak);
      // Take a small break
      suspendTime = lockscreen(smallBreak, 0);
      if (suspendTime > 0) {
        fprintf(logFile, "During small break, the system has slept for %d hours %d minutes %d seconds.\n", suspendTime/3600, (suspendTime/60)%60, suspendTime%60);
        if (suspendTime > largeBreak*60 - smallBreak) {
          smallBreakCounter = 0;
        }
      } else {
        if (suspendTime < 0) {
          postponeNumSmall++;
          fprintf(logFile, "You forcefully end a small break the %u-th times. Remaining time is %d seconds.\n", postponeNumSmall, -suspendTime);
        }
      }
    } else {
      largeBreakCounter++;
      fprintf(logFile, "largeBreakCounter = %d\n", largeBreakCounter);
      smallBreakCounter = 0;
      fprintf(logFile, "\nLarge break number %d: let's take a break for %d minutes!\n", largeBreakCounter, largeBreak);

      // Take a large break
      suspendTime = lockscreen(largeBreak*60, 0);
      if (suspendTime > 0) {
        fprintf(logFile, "During large break, the system has slept for %d hours %d minutes %d seconds.\n", suspendTime/3600, (suspendTime/60)%60, suspendTime%60);
        smallBreakCounter = 0;
      } else {
        if (suspendTime < 0) {
          postponeNumLarge++;
          fprintf(logFile, "You forcefully end a large break the %u-th times. Remaining time is %d minutes %d seconds.\n", postponeNumLarge, -suspendTime/60, -suspendTime%60);
        }
      }
    }
    if (reloadRequested) {
      load_config();
      maxWorkTimeNum = maxWorkTime/workTime;
      numberSubWorkTime = workTime*60/subWorkTime;
      reloadRequested = 0;
    }
  }
  if (logFile != stderr) fclose(logFile);
  return 0;
}        /* ----------  end of function main  ---------- */
