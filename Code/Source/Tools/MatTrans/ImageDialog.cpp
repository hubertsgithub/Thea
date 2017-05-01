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
//Size event
  EVT_SIZE(wxImageDialog::OnSize)
  */

// Keyboard events
EVT_KEY_DOWN(wxImageDialog::keyPressed)
EVT_KEY_UP(wxImageDialog::keyPressed)

// Mouse events
EVT_LEFT_DOWN(wxImageDialog::mouseLeftDown)
EVT_RIGHT_DOWN(wxImageDialog::mouseRightDown)
// catch paint events
EVT_PAINT(wxImageDialog::paintEvent)
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


wxImageDialog::wxImageDialog(wxFrame* parent, TheaArray<PA::PhotoData> const & photo_list_):
  wxDialog(parent, -1, "Retrieved Image")
{
  photo_list = photo_list_;
  if (photo_list.size() == 0)
    throw Error(format("Invalid number of images: %ld", photo_list.size()));

  image_num = 0;
  loadCurrentImage();
  show_what = RETRIEVED_PHOTO;
  THEA_CONSOLE << "wxImageDialog constructor finished.";
}

void wxImageDialog::keyPressed(wxKeyEvent& event)
{
  int key = event.GetKeyCode();
  THEA_CONSOLE << "Key pressed in wxImageDialog: " << key;

  // Maybe store parent frame so can update title of window here TODO
  // Maybe important to not allow window to be closed (?)
}

void wxImageDialog::mouseLeftDown(wxMouseEvent& event) {
  // Show other retrievals
  image_num++;
  if (image_num == int(photo_list.size()))
    image_num = 0;
  loadCurrentImage();
  paintNow();
}

void wxImageDialog::mouseRightDown(wxMouseEvent& event) {
  // Switch between shape view and retrieved image
  if (show_what == RETRIEVED_PHOTO) {
    show_what = SHAPE_VIEW;
    SetSize(wxDefaultCoord, wxDefaultCoord, shape_view_image_bitmap.GetWidth(), shape_view_image_bitmap.GetHeight());
  } else if (show_what == SHAPE_VIEW) {
    show_what = RETRIEVED_PHOTO;
    SetSize(wxDefaultCoord, wxDefaultCoord, retrieved_image_bitmap.GetWidth(), retrieved_image_bitmap.GetHeight());
  }
  paintNow();
}

void wxImageDialog::resizeKeepAspect(int mindim, wxImage& to_be_resized)
{
  int new_width, new_height;
  if (to_be_resized.GetWidth() < to_be_resized.GetHeight()) {
    new_width = mindim;
    new_height = int(float(to_be_resized.GetHeight()) / to_be_resized.GetWidth() * mindim);
  } else {
    new_width = int(float(to_be_resized.GetWidth()) / to_be_resized.GetHeight() * mindim);
    new_height = mindim;
  }

  to_be_resized.Rescale(new_width, new_height);
}

void wxImageDialog::loadCurrentImage()
{
  // Load the file... ideally add a check to see if loading was successful
  shape_view_image.LoadFile(photo_list[image_num].shape_view_path.c_str(), wxBITMAP_TYPE_PNG);
  resizeKeepAspect(600, shape_view_image);
  // Convert to bitmap for rendering
  shape_view_image_bitmap = wxBitmap(shape_view_image);
  // Load the file... ideally add a check to see if loading was successful
  retrieved_image.LoadFile(photo_list[image_num].photo_path.c_str(), wxBITMAP_TYPE_JPEG);
  resizeKeepAspect(600, retrieved_image);
  // Convert to bitmap for rendering
  retrieved_image_bitmap = wxBitmap(retrieved_image);

  SetSize(wxDefaultCoord, wxDefaultCoord, retrieved_image_bitmap.GetWidth(), retrieved_image_bitmap.GetHeight());
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
void wxImageDialog::render(wxDC& dc)
{
  const wxBitmap* bitmap_to_show;
  float point_x;
  float point_y;
  if (show_what == RETRIEVED_PHOTO) {
    bitmap_to_show = &retrieved_image_bitmap;
    point_x = photo_list[image_num].rx * bitmap_to_show->GetWidth();
    point_y = photo_list[image_num].ry * bitmap_to_show->GetHeight();
  } else if (show_what == SHAPE_VIEW) {
    bitmap_to_show = &shape_view_image_bitmap;
    point_x = photo_list[image_num].qx * bitmap_to_show->GetWidth();
    point_y = photo_list[image_num].qy * bitmap_to_show->GetHeight();
  } else {
    bitmap_to_show = NULL;
  }
  //int neww, newh;
  //dc.GetSize(&neww, &newh);

  dc.DrawBitmap(*bitmap_to_show, 0, 0, false);

  // draw a circle
  dc.SetBrush(*wxTRANSPARENT_BRUSH); // green filling
  dc.SetPen(wxPen(wxColor(255, 0, 0), 2)); // 2-pixels-thick red outline
  wxPoint center(point_x, point_y);
  dc.DrawCircle(center, 5 /* radius */);
}

} // namespace MatTrans

