#ifndef __Browse3D_ImagePanel_hpp__
#define __Browse3D_ImagePanel_hpp__

#include <wx/wx.h>
#include <wx/sizer.h>
#include <vector>
#include <string>

class wxImagePanel : public wxPanel
{
    wxImage image;
    wxBitmap resized;
    int w, h;
    std::vector<std::string> image_paths;
    int image_num;

public:
    wxImagePanel(wxFrame* parent, wxString file, wxBitmapType format);

    void paintEvent(wxPaintEvent & evt);
    void paintNow();
    void OnSize(wxSizeEvent& event);
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

#endif
