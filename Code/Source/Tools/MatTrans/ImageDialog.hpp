#ifndef __MatTrans_ImageDialog_hpp__
#define __MatTrans_ImageDialog_hpp__

#include "Array.hpp"
#include <wx/wx.h>
#include <wx/sizer.h>
#include <vector>
#include <string>

namespace MatTrans {

using namespace Thea;

class wxImageDialog : public wxDialog
{
  wxImage image;
  wxBitmap bitmap;
  wxBitmapType image_format;
  int w, h;
  TheaArray<std::string> image_paths;
  int image_num;

public:
  wxImageDialog(wxFrame* parent, TheaArray<std::string> const & image_paths_, wxBitmapType image_format_);

  void loadCurrentImage();
  void paintEvent(wxPaintEvent & evt);
  void paintNow();
  void render(wxDC& dc);

  // some useful events
  /*
   void mouseMoved(wxMouseEvent& event);
   void mouseDown(wxMouseEvent& event);
   void mouseWheelMoved(wxMouseEvent& event);
   void mouseReleased(wxMouseEvent& event);
   void rightClick(wxMouseEvent& event);
   void mouseLeftWindow(wxMouseEvent& event);
   void keyPressed(wxKeyEvent& event);
   void keyReleased(wxKeyEvent& event);
   */
  void keyPressed(wxKeyEvent& event);

  DECLARE_EVENT_TABLE()
};

} // namespace MatTrans

#endif
