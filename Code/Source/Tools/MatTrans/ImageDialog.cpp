// Adapted from https://wiki.wxwidgets.org/An_image_Dialog

#include "ImageDialog.hpp"
#include "BasicStringAlg.hpp"
#include "Error.hpp"
#include <algorithm>
#include <fstream>
#include <wx/wx.h>
#include <wx/sizer.h>

namespace MatTrans {

BEGIN_EVENT_TABLE(wxImageDialog, wxDialog)
  // some useful events
  /*
   EVT_MOTION(wxImageDialog::mouseMoved)
   EVT_LEFT_DOWN(wxImageDialog::mouseDown)
   EVT_LEFT_UP(wxImageDialog::mouseReleased)
   EVT_RIGHT_DOWN(wxImageDialog::rightClick)
   EVT_LEAVE_WINDOW(wxImageDialog::mouseLeftWindow)
   EVT_KEY_DOWN(wxImageDialog::keyPressed)
   EVT_KEY_UP(wxImageDialog::keyReleased)
   EVT_MOUSEWHEEL(wxImageDialog::mouseWheelMoved)
   EVT_SIZE(wxImageDialog::OnSize)
   */

  EVT_KEY_DOWN(wxImageDialog::keyPressed)
  // catch paint events
  EVT_PAINT(wxImageDialog::paintEvent)
  //Size event
END_EVENT_TABLE()


// some useful events
/*
 void wxImageDialog::mouseMoved(wxMouseEvent& event) {}
 void wxImageDialog::mouseDown(wxMouseEvent& event) {}
 void wxImageDialog::mouseWheelMoved(wxMouseEvent& event) {}
 void wxImageDialog::mouseReleased(wxMouseEvent& event) {}
 void wxImageDialog::rightClick(wxMouseEvent& event) {}
 void wxImageDialog::mouseLeftWindow(wxMouseEvent& event) {}
 void wxImageDialog::keyPressed(wxKeyEvent& event) {}
 void wxImageDialog::keyReleased(wxKeyEvent& event) {}
 */


wxImageDialog::wxImageDialog(wxFrame* parent, TheaArray<std::string> const & image_paths_, wxBitmapType image_format_):
  wxDialog(parent, -1, "Retrieved Image")
{
  image_paths = image_paths_;
  image_format = image_format_;
  if (image_paths.size() == 0)
    throw Error(format("Invalid number of images: %ld", image_paths.size()));

  image_num = 0;
  loadCurrentImage();
  THEA_CONSOLE << "wxImageDialog constructor finished.";
}

void wxImageDialog::keyPressed(wxKeyEvent& event)
{
  int key = event.GetKeyCode();

  if (key == WXK_LEFT) {
    image_num++;
    if (image_num == int(image_paths.size()))
      image_num = 0;
    paintNow();
  }

  // Maybe store parent frame so can update title of window here TODO
  // Maybe important to not allow window to be closed (?)
}

void wxImageDialog::loadCurrentImage()
{
  // Load the file... ideally add a check to see if loading was successful
  image.LoadFile(image_paths[image_num].c_str(), image_format);
  // Convert to bitmap for rendering
  bitmap = wxBitmap(image);
  // Resize dialog to fit the loaded image
  SetSize(wxDefaultCoord, wxDefaultCoord, image.GetWidth(), image.GetHeight());
}

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

void wxImageDialog::paintEvent(wxPaintEvent & evt)
{
  // depending on your system you may need to look at double-buffered dcs
  wxPaintDC dc(this);
  render(dc);
}

/*
 * Alternatively, you can use a clientDC to paint on the panel
 * at any time. Using this generally does not free you from
 * catching paint events, since it is possible that e.g. the window
 * manager throws away your drawing when the window comes to the
 * background, and expects you will redraw it when the window comes
 * back (by sending a paint event).
 */
void wxImageDialog::paintNow()
{
  // depending on your system you may need to look at double-buffered dcs
  wxClientDC dc(this);
  render(dc);
}

/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
void wxImageDialog::render(wxDC&  dc)
{
  //int neww, newh;
  //dc.GetSize(&neww, &newh);

  dc.DrawBitmap(bitmap, 0, 0, false);
}

} // namespace MatTrans

