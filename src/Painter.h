#pragma once

#include <memory>
#include "CImg.h"
#include "LefDefParser.h"

namespace Graphic
{

static const unsigned char white[]  = {255, 255, 255},
                           black[]  = {  0,   0,   0},
                           red[]    = {255,   0,   0},
                           blue[]   = {120, 200, 255},
                           green[]  = {0,   255,   0},
                           purple[] = {255, 100, 255},
                           orange[] = {255, 165,   0},
                           yellow[] = {255, 255,   0},
                           gray[]   = {204, 204, 204},
                           aqua[]   = {204, 204, 255};

using namespace LefDefDB;
using namespace cimg_library;

typedef const unsigned char* Color;
typedef CImg<unsigned char> CImgObj;

class Painter 
{
  public:

    Painter();
		Painter(std::shared_ptr<LefDefParser> db) : Painter()
		{ db_ = db; }

    void drawChip();

  private:

		void init();

    // LefDef Parser
    std::shared_ptr<LefDefParser> db_;

    int maxWidth_;
    int maxHeight_;

    int offsetX_;
    int offsetY_;

    int canvasX_;
    int canvasY_;

    double scale_; 
    // Scaling Factor to fit the window size

    //CImg library
    CImgObj*       canvas_;
    CImgObj*          img_;
    CImgDisplay*   window_;

    // Draw Objects
    int getX(int dbX);
    int getY(int dbY);

    int getX(float dbX);
    int getY(float dbY);

    void drawLine(CImgObj *img, int x1, int y1, int x2, int y2);
    void drawLine(CImgObj *img, int x1, int y1, int x2, int y2, Color c);
    void drawRect(CImgObj *img, int lx, int ly, int ux, int uy, Color rect_c, int w);
    void drawRect(CImgObj *img, int lx, int ly, int ux, int uy, Color rect_c, 
                  Color line_c = black, int w = 0, float opacity = 1.0);
                  // line_c: border-line color
                  // w : Thicnkness of border-line

    // Draw Cell
    void drawCell     (CImgObj *img, const dbCell* cell);
    void drawFixed    (CImgObj *img, const dbCell* cell);
    void drawMovable  (CImgObj *img, const dbCell* cell);
    void drawCells    (CImgObj *img);
    void drawDie      (CImgObj *img);

    void show();

    bool check_inside(int lx, int ly, int w, int h);
};

} // namespace Graphic
