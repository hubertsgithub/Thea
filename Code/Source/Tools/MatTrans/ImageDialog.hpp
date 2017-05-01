#ifndef __MatTrans_ImageDialog_hpp__
#define __MatTrans_ImageDialog_hpp__

#include "Array.hpp"
#include <wx/wx.h>
#include <wx/sizer.h>
#include <vector>
#include <string>
#include "PythonApi.hpp"

namespace MatTrans {

using namespace Thea;

class wxImageDialog : public wxDialog
{
  enum ShowWhat
  {
    RETRIEVED_PHOTO,
    SHAPE_VIEW
  };

  wxImage shape_view_image;
  wxBitmap shape_view_image_bitmap;
  wxImage retrieved_image;
  wxBitmap retrieved_image_bitmap;
  int w, h;
  TheaArray<PA::PhotoData> photo_list;
  int image_num;
  ShowWhat show_what;

  void resizeKeepAspect(int mindim, wxImage& to_be_resized);

public:
  wxImageDialog(wxFrame* parent, TheaArray<PA::PhotoData> const & photo_list_);

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
  void mouseLeftDown(wxMouseEvent& event);
  void mouseRightDown(wxMouseEvent& event);

  DECLARE_EVENT_TABLE()
};

} // namespace MatTrans

#endif
