// =====================================================================================
//
//       Filename:  config.h
//
//    Description:  Define smallBreak, largeBreak, and workTime
//
//        Version:  1.0
//        Created:  05/02/2017 08:58:04 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  Hoang-Ngan Nguyen (), zhoangngan@gmail.com
//   Organization:  
//
// =====================================================================================

static const unsigned int smallBreak  = 50; // (seconds)
static const unsigned int largeBreak  = 10; // (minutes)
static const unsigned int workTime    = 19; // (minutes)

static const double radius = 120.0;
static const double ballRadius = 30;
static const int N = 20; // number of trapezoids for each ball

static const XRenderColor colorList[NUMCOLS][2] = {
  [M_DIM] = {   // color of the ball when its corresponding bit = 0
    {0x94ff, 0x94ff, 0x94ff, 0xf0ff},   // inner color: white
    {0x0000, 0xafff, 0x0000, 0xffff}    // outer color: green 
  },
  [M_BRG] = {
    {0xffff, 0xffff, 0x0fff, 0xffff},   // white
    {0x0000, 0xafff, 0x0000, 0xffff}    // outer color: green 
  },
  [S_DIM] = {
    {0x94ff, 0x94ff, 0x94ff, 0xf0ff},   // inner color: white
    {0xffff, 0x2bff, 0x00ff, 0xffff}    // outer color: blue
    //{0x1bff, 0x85ff, 0xedff, 0xffff}    // outer color: blue
  },
  [S_BRG] = {
    {0xffff, 0xffff, 0x0fff, 0xffff}, 
    {0xffff, 0x2bff, 0x00ff, 0xffff}    // outer color: blue
    //{0x1bff, 0x85ff, 0xedff, 0xffff}
  }
};

static const XFixed colorStops[2] = {
  XDoubleToFixed (0.0f), 
  XDoubleToFixed (1.0f) 
};
