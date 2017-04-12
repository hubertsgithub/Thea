//============================================================================
//
// This file is part of the MatTrans project.
//
// This software is covered by the following BSD license, except for portions
// derived from other works which are covered by their respective licenses.
// For full licensing information including reproduction of these external
// licenses, see the file LICENSE.txt provided in the documentation.
//
// Copyright (C) 2011, Siddhartha Chaudhuri/Stanford University
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holders nor the names of contributors
// to this software may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//============================================================================

#ifndef __MatTrans_MainWindow_hpp__
#define __MatTrans_MainWindow_hpp__

#include "Common.hpp"
#include <wx/frame.h>

class wxCheckBox;
class wxListBox;
class wxNotebook;
class wxSplitterWindow;
class wxTextCtrl;
class wxUpdateUIEvent;

namespace MatTrans {

class Model;
class ModelDisplay;

/** Holds MainWindow UI elements. */
struct MainWindowUI
{
  wxSplitterWindow * main_splitter;

  ModelDisplay * model_display;
  wxNotebook * toolbox;

  /** Default constructor. Sets all pointers to null. */
  MainWindowUI();

}; // struct MainWindow

/** The main application window. */
class MainWindow : public wxFrame
{
  private:
    typedef wxFrame BaseType;

  public:
    /** Custom item IDs. */
    enum ItemID
    {
      ID_VIEW_SHADED,
      ID_VIEW_WIREFRAME,
      ID_VIEW_SHADED_WIREFRAME,
      ID_VIEW_TWO_SIDED,
      ID_VIEW_FLAT_SHADING,
      ID_VIEW_FIT,
      ID_GO_PREV,
      ID_GO_NEXT,
      ID_GO_PREV_FEATURES,
      ID_GO_NEXT_FEATURES,
      ID_TOOLS_SCREENSHOT,
      ID_TOOLS_TOOLBOX,

    }; // enum EventID

    /** Constructor. */
    explicit MainWindow(wxWindow * parent = NULL);

    /** Destructor. */
    ~MainWindow();

    /** Initialize the main window. Should be called at application startup. */
    void init();

    /** Get the display widget showing the model. */
    ModelDisplay * getRenderDisplay();

    /** Get the active model. */
    Model const * getModel() const { return model; }

    /** Get the active model. */
    Model * getModel() { return model; }

    /** Get the number of overlay models. */
    long numOverlays() const { return (long)overlays.size(); }

    /** Get the set of overlay models. */
    Model const * const * getOverlays() const { return overlays.empty() ? NULL : &overlays[0]; }

    /** [wxWidgets] Set the window title. */
    void SetTitle(wxString const & title);

    //=========================================================================================================================
    // GUI callbacks
    //=========================================================================================================================

    /** Called when program exits. */
    void OnExit(wxEvent & event = DUMMY_EVENT);

    /** Set the window title. */
    void setTitle(wxEvent & event = DUMMY_EVENT);

    /** Select and load a model. */
    void selectAndLoadModel(wxEvent & event = DUMMY_EVENT);

    /** Load the previous model in the directory. */
    void loadPreviousModel(wxEvent & event = DUMMY_EVENT);

    /** Load the next model in the directory. */
    void loadNextModel(wxEvent & event = DUMMY_EVENT);

    /** Load the previous set of features in the features directory. */
    void loadPreviousFeatures(wxEvent & event = DUMMY_EVENT);

    /** Load the next set of features in the features directory. */
    void loadNextFeatures(wxEvent & event = DUMMY_EVENT);

    /** Toggle the visibility state of the toolbox. */
    void toggleToolboxVisible(wxEvent & event = DUMMY_EVENT);

    /** Show/hide the toolbox. */
    void setToolboxVisible(wxCommandEvent & event);

    /** Synchronize states of menu and toolbar buttons etc. */
    void updateUI(wxUpdateUIEvent & event);

    /** Refresh the model display. */
    void refreshDisplay(wxEvent & event = DUMMY_EVENT);

  private:
    /** Get rid of all overlay models. */
    void clearOverlays();

    /** Show or hide the toolbox. */
    void setToolboxVisible(bool value);

    // Models
    Model * model;
    TheaArray<Model *> overlays;

    // Widgets
    MainWindowUI ui;

}; // class MainWindow

} // namespace MatTrans

#endif