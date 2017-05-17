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

// clock configuration
static const double radius = 120.0;
static const double ballRadius = 30;
static const int N = 20; // number of trapezoids for each ball

static const XRenderColor colorList[NUMCOLS][2] = {
  [M_DIM] = {   // color of the ball when its corresponding bit = 0
    {0x94ff, 0x94ff, 0x00ff, 0xf0ff},   // inner color
    {0x0000, 0xa0ff, 0x0000, 0xffff}    // outer color: red
  },
  [M_BRG] = {
    {0xffff, 0xffff, 0x0fff, 0xffff},
    {0x0000, 0xafff, 0x0000, 0xffff}    // outer color: red
  },
  [S_DIM] = {
    {0x94ff, 0x94ff, 0x00ff, 0xf0ff},
    {0xffff, 0x00ff, 0x00ff, 0xffff}    // outer color: green
  },
  [S_BRG] = {
    {0xffff, 0xffff, 0x0fff, 0xffff},
    {0xffff, 0x00ff, 0x00ff, 0xffff}    // outer color: green
  }
};

// background color of the locked-screen
static const XRenderColor bg_color = {
  .red=0xffff, .green=0x37ff, .blue=0x00ff, .alpha=0x7fff
};

static const XFixed colorStops[2] = {
  XDoubleToFixed (0.0f), 
  XDoubleToFixed (1.0f) 
};
