#include "Painter.h"
#include "LefDefParser.h"
#include "CImg.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include <climits>   // For INT_MAX, INT_MIN
#include <cfloat>    // For FLT_MAX
#include <cmath>
#include <random>

// Not a real size
// MAX_W, MAX_H is just a imaginary size
#define MAX_W 6000
#define MAX_H 6000

#define WINDOW_W 2000
#define WINDOW_H 2000

#define ZOOM_SPEED 300
#define MOVE_SPEED 300

#define DIE_OFFSET_X 10
#define DIE_OFFSET_Y 10

#define DIE_OPACITY         1.0
#define MACRO_OPACITY       0.7
#define STD_CELL_OPACITY    0.7

#define DIE_LINE_THICKNESS      1
#define MACRO_LINE_THICKNESS    0
#define STD_CELL_LINE_THICKNESS 0

namespace Graphic
{

using namespace LefDefDB;
using namespace cimg_library;

static const Color DIE_COLOR             = gray;
static const Color MACRO_COLOR           = aqua;
static const Color STD_CELL_COLOR        = red;

static const Color NET_LINE_COLOR        = blue;
static const Color DIE_LINE_COLOR        = black;
static const Color MACRO_LINE_COLOR      = black;
static const Color STD_CELL_LINE_COLOR   = red; // for gif plot mode, red will look better 

inline void printInterfaceMessage()
{
  printf("Graphic Interface Manual\n");
  printf("[Q]: Close the window\n");
  printf("[Z]: Zoom In\n");
  printf("[X]: Zoom Out\n");
  printf("[F]: Zoom to Fit\n");
  printf("[H]: Print Key Map\n");
  printf("[UP DOWN LEFT DOWN]: Move Zoom Box\n");
}

// Painter Interface //
Painter::Painter()
{}

void
Painter::init()
{
  offsetX_ = DIE_OFFSET_X;
  offsetY_ = DIE_OFFSET_Y;

  canvasX_ = MAX_W + 2 * DIE_OFFSET_X;
  canvasY_ = MAX_H + 2 * DIE_OFFSET_Y;

  // canvas is just a background image for placement visualization
  canvas_ = new CImg<unsigned char>(canvasX_, canvasY_, 1, 3, 255);
  canvas_->draw_rectangle(0, 0, canvasX_, canvasY_, white);

  // img_ := Original image which represents the whole placement
  // any 'zoomed' image will use a crop of this img_
  img_ = new CImg<unsigned char>(*canvas_);

  maxWidth_  = db_->die()->ux();
  maxHeight_ = db_->die()->uy();

  double scaleX = double(MAX_W) / double(maxWidth_ );
  double scaleY = double(MAX_H) / double(maxHeight_);

  scale_ = std::min(scaleX, scaleY);
}

int
Painter::getX(int dbX)
{
  double tempX = static_cast<double>(dbX);
  tempX = scale_ * tempX;
  return (static_cast<double>(tempX) + offsetX_);
}

int
Painter::getY(int dbY)
{
  double tempY = static_cast<double>(maxHeight_ - dbY);
  tempY = scale_ * tempY;
  return (static_cast<int>(tempY) + offsetY_);
}

int
Painter::getX(float dbX)
{
  double tempX = static_cast<double>(dbX);
  tempX = scale_ * tempX;
  return (static_cast<double>(tempX) + offsetX_);
}

int
Painter::getY(float dbY)
{
  double tempY = static_cast<double>(maxHeight_ - dbY);
  tempY = scale_ * tempY;
  return (static_cast<int>(tempY) + offsetY_);
}

void 
Painter::drawLine(CImgObj *img, 
                  int x1, 
                  int y1, 
                  int x2, 
                  int y2)
{
  img->draw_line(x1, y1, x2, y2, black);
}

void 
Painter::drawLine(CImgObj *img, int x1, int y1, int x2, int y2, Color color)
{
  img->draw_line(x1, y1, x2, y2, color);
}

void
Painter::drawRect(CImgObj *img, int lx, int ly, int ux, int uy, Color rect_c, int w)
{
  drawRect(img, lx, ly, ux, uy, rect_c, black, w, 1.0);
}

void
Painter::drawRect(CImgObj *img, int lx, int ly, int ux, int uy, Color rect_c,
                  Color line_c, int w, float opacity)
{
  img->draw_rectangle(lx, ly, ux, uy, rect_c, opacity);
  drawLine(img, lx, ly, ux, ly, line_c);
  drawLine(img, ux, ly, ux, uy, line_c);
  drawLine(img, ux, uy, lx, uy, line_c);
  drawLine(img, lx, uy, lx, ly, line_c);

  int xd = (ux > lx) ? 1 : -1; 
  int yd = (uy > ly) ? 1 : -1; 

  if(w > 0)
  {
    for(int i = 1; i < w + 1; i++)
    {
      drawLine(img, 
               lx + xd * i, ly + yd * i, 
               ux - xd * i, ly + yd * i, line_c);

      drawLine(img,
               ux - xd * i, ly + yd * i, 
               ux - xd * i, uy - yd * i , line_c);

      drawLine(img,
               ux - xd * i, uy - yd * i, 
               lx + xd * i, uy - yd * i, line_c);

      drawLine(img,
               lx + xd * i, uy - yd * i, 
               lx + xd * i, ly + yd * i, line_c);

      drawLine(img,
               lx - xd * i, ly - yd * i, 
               ux + xd * i, ly - yd * i, line_c);

      drawLine(img,
               ux + xd * i, ly - yd * i, 
               ux + xd * i, uy + yd * i, line_c);

      drawLine(img,
               ux + xd * i, uy + yd * i, 
               lx - xd * i, uy + yd * i, line_c);

      drawLine(img,
               lx - xd * i, uy + yd * i, 
               lx - xd * i, ly - yd * i, line_c);
    }
  }
}

bool 
Painter::check_inside(int lx, int ly, int w, int h)
{
  if(lx < 0)            return false;
  if(ly < 0)            return false;
  if(lx + w > canvasX_) return false;
  if(ly + h > canvasY_) return false;
  return true;
}

void
Painter::drawDie(CImgObj *img)
{
  drawRect(img, getX(db_->die()->lx()), getY(db_->die()->ly()), 
                getX(db_->die()->ux()), getY(db_->die()->uy()), 
                gray, DIE_LINE_COLOR, DIE_LINE_THICKNESS, DIE_OPACITY);
}

void
Painter::drawCell(CImgObj *img, const dbCell* cell)
{
  int newLx = getX(cell->lx());
  int newLy = getY(cell->ly());
  int newUx = getX(cell->ux());
  int newUy = getY(cell->uy());

  if(cell->isMacro())
  {
    drawRect(img, newLx, newLy, newUx, newUy, MACRO_COLOR, 
                                              MACRO_LINE_COLOR,
                                              MACRO_LINE_THICKNESS, 
                                              MACRO_OPACITY);

		// cell->printLoc();
  }
  else 
  {
    drawRect(img, newLx, newLy, newUx, newUy, STD_CELL_COLOR, 
                                              STD_CELL_LINE_COLOR, 
                                              STD_CELL_LINE_THICKNESS,
                                              STD_CELL_OPACITY);
  }
}

void
Painter::drawFixed(CImgObj *img, const dbCell* cell)
{
  int newLx = getX(cell->lx());
  int newLy = getY(cell->ly());
  int newUx = getX(cell->ux());
  int newUy = getY(cell->uy());

  if(cell->isFixed())
  {
    drawRect(img, newLx, newLy, newUx, newUy, MACRO_COLOR, 
                                              MACRO_LINE_COLOR,
                                              MACRO_LINE_THICKNESS, 
                                              MACRO_OPACITY);
  }
}

void
Painter::drawMovable(CImgObj *img, const dbCell* cell)
{
  int newLx = getX(cell->lx());
  int newLy = getY(cell->ly());
  int newUx = getX(cell->ux());
  int newUy = getY(cell->uy());

  if(!cell->isFixed())
  {
    drawRect(img, newLx, newLy, newUx, newUy, STD_CELL_COLOR, 
                                              STD_CELL_LINE_COLOR, 
                                              STD_CELL_LINE_THICKNESS,
                                              STD_CELL_OPACITY);
  }
}

void
Painter::drawCells(CImgObj *img)
{
	std::cout << "numCells: " << db_->cells().size() << std::endl;

  for(auto &c : db_->cells())
  {
    if( c->isFixed() )
			drawFixed(img, c);
		else
			drawCell(img, c);
  }
}

void
Painter::drawChip()
{
	init();
  drawDie(img_);
  drawCells(img_);
  show();
}

void 
Painter::show()
{
  int viewX = 0;
  int viewY = 0;

  int ZoomBoxW = MAX_H; 
  int ZoomBoxH = MAX_W; 

  bool redraw = false;

  int lx = 0;
  int ly = 0;

  CImgObj ZoomBox 
    = img_->get_crop(lx, ly, lx + canvasX_, ly + canvasY_);

  window_ = new CImgDisplay(WINDOW_W, WINDOW_H, "Placement GUI");

  printInterfaceMessage();

  // Interactive Mode //
  while(!window_->is_closed() && !window_->is_keyESC()
                              && !window_->is_keyQ())
  {
    if(redraw)
    {
      img_ = new CImg<unsigned char>(*canvas_);
      drawDie(img_);
      drawCells(img_);

      ZoomBox 
        = img_->get_crop(lx, ly, lx + ZoomBoxW, ly + ZoomBoxH);
      ZoomBox.resize(*window_);
      redraw = false;
    }

    ZoomBox.display(*window_);

    if(window_->key())
    {
      switch(window_->key())
      {
        case cimg::keyARROWUP:
          if(check_inside(lx, ly - MOVE_SPEED, ZoomBoxW, ZoomBoxH))
          {
            ly -= MOVE_SPEED;
            redraw = true;
          }
          break;
        case cimg::keyARROWDOWN:
          if(check_inside(lx, ly + MOVE_SPEED, ZoomBoxW, ZoomBoxH))
          {
            ly += MOVE_SPEED;
            redraw = true;
          }
          break;
        case cimg::keyARROWLEFT:
          if(check_inside(lx - MOVE_SPEED, ly, ZoomBoxW, ZoomBoxH))
          {
            lx -= MOVE_SPEED;
            redraw = true;
          }
          break;
        case cimg::keyARROWRIGHT:
          if(check_inside(lx + MOVE_SPEED, ly, ZoomBoxW, ZoomBoxH))
          {
            lx += MOVE_SPEED;
            redraw = true;
          }
          break;

        case cimg::keyZ:
          if(ZoomBoxW > ZOOM_SPEED 
          && ZoomBoxH > ZOOM_SPEED)
          {
            redraw = true;
            ZoomBoxW -= ZOOM_SPEED;
            ZoomBoxH -= ZOOM_SPEED;
          }
          break;
        case cimg::keyX:
          if(ZoomBoxW <= canvasX_ - ZOOM_SPEED 
          && ZoomBoxH <= canvasY_ - ZOOM_SPEED)
          {
            redraw = true;
            ZoomBoxW += ZOOM_SPEED;
            ZoomBoxH += ZOOM_SPEED;
          }
          break;

        case cimg::keyF:
          redraw = true;
          lx = 0;
          ly = 0;
          ZoomBoxW = MAX_H; 
          ZoomBoxH = MAX_W; 
          break;

        case cimg::keyH:
          printInterfaceMessage();
          break;
      }
      window_->set_key(); // Flush all key events...
    }
    window_->wait();
  }
  exit(0);
}

} // namespace Graphic
