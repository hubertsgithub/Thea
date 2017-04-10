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

#include "MainWindow.hpp"
#include "App.hpp"
#include "Model.hpp"
#include "ModelDisplay.hpp"
#include "Util.hpp"
#include "../../Application.hpp"
#include "../../FilePath.hpp"
#include "../../FileSystem.hpp"
#include <wx/accel.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/listbox.h>
#include <wx/menu.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace MatTrans {

// Definition of object declared in common header
wxCommandEvent DUMMY_EVENT;

static int const MATERIALS_TAB_INDEX  =     0;
static Real const MIN_SPLIT_SIZE      =   300;

MainWindowUI::MainWindowUI()
: model_display(NULL),
  toolbox(NULL)
{}

MainWindow::MainWindow(wxWindow * parent)
: BaseType(parent, wxID_ANY, "MatTrans", wxDefaultPosition, wxSize(800, 600)),
  model(NULL)
{
  init();
}

void
MainWindow::init()
{
  //==========================================================================================================================
  // Menu
  //==========================================================================================================================

  // Set up the main menu
  wxMenuBar * menubar = new wxMenuBar();

  // File Menu
  wxMenu * file_menu = new wxMenu();
  file_menu->Append(wxID_OPEN,    "&Open");
  file_menu->Append(wxID_SAVEAS,  "&Save");
  file_menu->AppendSeparator();
  file_menu->Append(wxID_EXIT,    "&Quit");
  menubar->Append(file_menu, "&File");

  // View menu
  wxMenu * view_menu = new wxMenu();
  wxMenu * rendering_menu = new wxMenu();
    rendering_menu->AppendRadioItem(ID_VIEW_SHADED,            "&Shaded");
    rendering_menu->AppendRadioItem(ID_VIEW_WIREFRAME,         "&Wireframe");
    rendering_menu->AppendRadioItem(ID_VIEW_SHADED_WIREFRAME,  "S&haded + wireframe");
    rendering_menu->AppendSeparator();
    rendering_menu->AppendCheckItem(ID_VIEW_TWO_SIDED,         "&Two-sided lighting");
    rendering_menu->AppendCheckItem(ID_VIEW_FLAT_SHADING,      "&Flat shading");
  view_menu->AppendSubMenu(rendering_menu,  "&Rendering");
  view_menu->Append(ID_VIEW_FIT,            "&Fit view to model");
  menubar->Append(view_menu, "&View");

  // Go menu
  wxMenu * go_menu = new wxMenu();
  go_menu->Append(ID_GO_PREV,           "&Previous model");
  go_menu->Append(ID_GO_NEXT,           "&Next model");
  go_menu->AppendSeparator();
  go_menu->Append(ID_GO_PREV_FEATURES,  "Previous features");
  go_menu->Append(ID_GO_NEXT_FEATURES,  "Next features");
  menubar->Append(go_menu, "&Go");

  // Tools menu
  wxMenu * tools_menu = new wxMenu();
  tools_menu->Append(ID_TOOLS_SCREENSHOT,        "&Save screenshot");
  tools_menu->AppendCheckItem(ID_TOOLS_TOOLBOX,  "&Toolbox");
  menubar->Append(tools_menu, "&Tools");

  // About menu
  wxMenu * help_menu = new wxMenu();
  help_menu->Append(wxID_ABOUT,  "&About");
  menubar->Append(help_menu, "&Help");

  SetMenuBar(menubar);

  //==========================================================================================================================
  // Toolbar
  //==========================================================================================================================

// #define SHOW_TOOLBAR
#ifdef SHOW_TOOLBAR
  wxToolBar * toolbar = CreateToolBar(wxTB_HORIZONTAL | wxTB_TEXT | wxTB_NOICONS, wxID_ANY, "Main toolbar");
  toolbar->AddTool(wxID_OPEN, "Open", wxNullBitmap, "Open a file");
  toolbar->AddTool(ID_GO_PREV, "Previous", wxNullBitmap, "Go to the previous model");
  toolbar->AddTool(ID_GO_NEXT, "Next", wxNullBitmap, "Go to the next model");
  toolbar->AddSeparator();
  toolbar->AddTool(ID_VIEW_FIT, "Fit", wxNullBitmap, "Fit view to model");
  toolbar->AddSeparator();
  toolbar->AddRadioTool(ID_VIEW_SHADED, "S", wxNullBitmap, wxNullBitmap, "Shaded polygons");
  toolbar->AddRadioTool(ID_VIEW_WIREFRAME, "W", wxNullBitmap, wxNullBitmap, "Wireframe");
  toolbar->AddRadioTool(ID_VIEW_SHADED_WIREFRAME, "SW", wxNullBitmap, wxNullBitmap, "Shading + wireframe");
  toolbar->AddSeparator();
  toolbar->AddTool(ID_TOOLS_TOOLBOX, "Toolbox", wxNullBitmap, "Show/hide toolbox");

  toolbar->Realize();
#endif

  //==========================================================================================================================
  // Main layout
  //==========================================================================================================================

  static Real const SASH_GRAVITY = 0.67;
  ui.main_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                          wxSP_3D | wxSP_BORDER | wxSP_PERMIT_UNSPLIT | wxSP_LIVE_UPDATE);
  ui.main_splitter->SetSashGravity(SASH_GRAVITY);
  ui.main_splitter->SetMinimumPaneSize(MIN_SPLIT_SIZE);
  ui.main_splitter->SetSashInvisible(false);

  // Create the model
  model = new Model;

  // An OpenGL display box for the model
  ui.model_display = new ModelDisplay(ui.main_splitter, model);

  // A tabbed pane for the toolbox
  ui.toolbox = new wxNotebook(ui.main_splitter, wxID_ANY);

  // Material retrieval interface
  wxPanel * materials_panel = new wxPanel(ui.toolbox);
  wxBoxSizer * materials_sizer = new wxBoxSizer(wxVERTICAL);
  materials_panel->SetSizer(materials_sizer);

  ui.toolbox->AddPage(materials_panel, "Materials");

  // The main UI is split into two panes
  ui.main_splitter->SplitVertically(ui.model_display, ui.toolbox, -MIN_SPLIT_SIZE);

  // Do the top-level layout
  wxBoxSizer * main_sizer = new wxBoxSizer(wxVERTICAL);
  SetSizer(main_sizer);
  main_sizer->Add(ui.main_splitter, 1, wxEXPAND);

  //==========================================================================================================================
  // Callbacks
  //==========================================================================================================================

  Bind(wxEVT_MENU, &MainWindow::selectAndLoadModel, this, wxID_OPEN);
  Bind(wxEVT_MENU, &MainWindow::OnExit, this, wxID_EXIT);

  Bind(wxEVT_MENU, &ModelDisplay::renderShaded, ui.model_display, ID_VIEW_SHADED);
  Bind(wxEVT_MENU, &ModelDisplay::renderWireframe, ui.model_display, ID_VIEW_WIREFRAME);
  Bind(wxEVT_MENU, &ModelDisplay::renderShadedWireframe, ui.model_display, ID_VIEW_SHADED_WIREFRAME);
  Bind(wxEVT_MENU, &ModelDisplay::setTwoSided, ui.model_display, ID_VIEW_TWO_SIDED);
  Bind(wxEVT_MENU, &ModelDisplay::setFlatShading, ui.model_display, ID_VIEW_FLAT_SHADING);
  Bind(wxEVT_MENU, &ModelDisplay::fitViewToModel, ui.model_display, ID_VIEW_FIT);

  Bind(wxEVT_MENU, &MainWindow::loadPreviousModel, this, ID_GO_PREV);
  Bind(wxEVT_MENU, &MainWindow::loadNextModel, this, ID_GO_NEXT);
  Bind(wxEVT_MENU, &MainWindow::loadPreviousFeatures, this, ID_GO_PREV_FEATURES);
  Bind(wxEVT_MENU, &MainWindow::loadNextFeatures, this, ID_GO_NEXT_FEATURES);

  Bind(wxEVT_MENU, &ModelDisplay::saveScreenshot, ui.model_display, ID_TOOLS_SCREENSHOT);
  Bind(wxEVT_MENU, &MainWindow::toggleToolboxVisible, this, ID_TOOLS_TOOLBOX);

  ui.toolbox->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &MainWindow::refreshDisplay, this);

  model->Bind(EVT_MODEL_PATH_CHANGED, &MainWindow::setTitle, this);

  Bind(wxEVT_UPDATE_UI, &MainWindow::updateUI, this);  // synchronize menu and toolbar buttons

  //==========================================================================================================================
  // Keyboard shortcuts for menu items
  //==========================================================================================================================

  wxAcceleratorEntry accel[] = {
    wxAcceleratorEntry(wxACCEL_CTRL, (int)'0', ID_VIEW_FIT),
    wxAcceleratorEntry(wxACCEL_CTRL, (int)',', ID_GO_PREV),
    wxAcceleratorEntry(wxACCEL_CTRL, (int)'.', ID_GO_NEXT),
    wxAcceleratorEntry(wxACCEL_CTRL, (int)'G', ID_TOOLS_SCREENSHOT),
    wxAcceleratorEntry(wxACCEL_CTRL, (int)'T', ID_TOOLS_TOOLBOX),
    wxAcceleratorEntry(wxACCEL_CTRL, (int)'[', ID_GO_PREV_FEATURES),
    wxAcceleratorEntry(wxACCEL_CTRL, (int)']', ID_GO_NEXT_FEATURES),
  };
  int num_accel = (int)(sizeof(accel) / sizeof(wxAcceleratorEntry));
  wxAcceleratorTable accel_table(num_accel, accel);
  SetAcceleratorTable(accel_table);

  // Load the initial model, if any
  bool loaded = model->load(app().options().model);
  if (loaded)
  {
    model->setTransform(app().options().model_transform);

    // Load overlays
    overlays.clear();
    for (array_size_t i = 0; i < app().options().overlays.size(); ++i)
    {
      Model * overlay = new Model;
      loaded = overlay->load(app().options().overlays[i]);
      if (loaded)
      {
        overlay->setTransform(app().options().overlay_transforms[i]);
        overlay->setColor(getPaletteColor((long)i));
        overlays.push_back(overlay);
      }
      else
      {
        delete overlay;
        clearOverlays();
        break;
      }
    }
  }

  //==========================================================================================================================
  // Initial view
  //==========================================================================================================================

  // We have to both set the menu item and call the function since wxEVT_MENU is not generated without actually clicking
  tools_menu->FindItem(ID_TOOLS_TOOLBOX)->Check(false);   setToolboxVisible(false);
  rendering_menu->FindItem(ID_VIEW_SHADED)->Check(true);  ui.model_display->renderShaded();

  rendering_menu->FindItem(ID_VIEW_TWO_SIDED)->Check(app().options().two_sided);
  ui.model_display->setTwoSided(app().options().two_sided);

  rendering_menu->FindItem(ID_VIEW_FLAT_SHADING)->Check(app().options().flat);
  ui.model_display->setFlatShading(app().options().flat);

/*
  ui->actionToolsToolbox->setChecked(false);
*/
}

MainWindow::~MainWindow()
{
  model->Unbind(EVT_MODEL_PATH_CHANGED, &MainWindow::setTitle, this);

  ui.model_display->setModel(NULL);  // this is necessary else we get a segfault when the base class destructor is called after
                                     // this, and can't find the model to deregister callbacks when destroying the display.
  clearOverlays();
  delete model;
}

ModelDisplay *
MainWindow::getRenderDisplay()
{
  return ui.model_display;
}

void
MainWindow::SetTitle(wxString const & title)
{
  if (title.empty())
    BaseType::SetTitle("MatTrans");
  else
  {
    std::string filename = FilePath::objectName(title.ToStdString());
    BaseType::SetTitle(filename + " - MatTrans (" + title + ")");
  }
}

//=============================================================================================================================
// GUI callbacks
//=============================================================================================================================

void
MainWindow::setTitle(wxEvent & event)
{
  SetTitle(model->getPath());
}

void
MainWindow::selectAndLoadModel(wxEvent & event)
{
  if (model->selectAndLoad())
    clearOverlays();
}

void
getMeshPatterns(TheaArray<std::string> & patterns)
{
  patterns.clear();
  patterns.push_back("*.3ds");
  patterns.push_back("*.obj");
  patterns.push_back("*.off");
  patterns.push_back("*.off.bin");
  patterns.push_back("*.ply");
  patterns.push_back("*.pts");
}

void
getFeaturePatterns(TheaArray<std::string> & patterns)
{
  patterns.clear();
  patterns.push_back("*.arff");
  patterns.push_back("*.arff.*");
  patterns.push_back("*.features");
  patterns.push_back("*.features.*");
}

long
fileIndex(TheaArray<std::string> const & files, std::string const & file, TheaArray<std::string> const * patterns = NULL)
{
  std::string fname = FilePath::objectName(file);
  for (array_size_t i = 0; i < files.size(); ++i)
    if (fname == FilePath::objectName(files[i]))
      return (long)i;

  return -1;
}

long
fileIndex(std::string const & dir, std::string const & file, TheaArray<std::string> & files,
          TheaArray<std::string> const * patterns = NULL)
{
  files.clear();

  if (FileSystem::fileExists(dir))
  {
    files.push_back(dir);
  }
  else
  {
    std::string pat = (patterns ? stringJoin(*patterns, ' ') : "");
    if (FileSystem::getDirectoryContents(dir, files, FileSystem::ObjectType::FILE, pat,
                                         false /* recursive */,
                                         true /* ignore_case */) <= 0)
      return -1;
  }

  return fileIndex(files, file, patterns);
}

void
MainWindow::loadPreviousModel(wxEvent & event)
{
  TheaArray<std::string> patterns;
  getMeshPatterns(patterns);

  TheaArray<std::string> files;
  long index = fileIndex(FilePath::parent(FileSystem::resolve(model->getPath())), model->getPath(), files, &patterns);
  if (files.empty())
    return;

  if (index < 0 || index >= (long)files.size())  // maybe the file was deleted recently?
    index = 0;
  else if (files.size() == 1)
    return;

  clearOverlays();

  if (index == 0)
    model->load(files[files.size() - 1]);
  else
    model->load(files[index - 1]);
}

void
MainWindow::loadNextModel(wxEvent & event)
{
  TheaArray<std::string> patterns;
  getMeshPatterns(patterns);

  TheaArray<std::string> files;
  long index = fileIndex(FilePath::parent(FileSystem::resolve(model->getPath())), model->getPath(), files, &patterns);
  if (files.empty())
    return;

  if (index < 0 || index >= (long)files.size())  // maybe the file was deleted recently?
    index = 0;
  else if (files.size() == 1)
    return;

  clearOverlays();

  if (index == (long)files.size() - 1)
    model->load(files[0]);
  else
    model->load(files[index + 1]);
}

void
MainWindow::loadPreviousFeatures(wxEvent & event)
{
  TheaArray<std::string> patterns;
  getFeaturePatterns(patterns);

  TheaArray<std::string> files;
  long index = fileIndex(app().options().features, model->getFeaturesPath(), files, &patterns);
  if (files.empty())
    return;

  if (index < 0 || index >= (long)files.size())  // maybe the file was deleted recently?
    index = 0;
  else if (files.size() == 1)
    return;

  if (index == 0)
    model->loadFeatures(files[files.size() - 1]);
  else
    model->loadFeatures(files[index - 1]);

  THEA_CONSOLE << "Loaded features " << model->getFeaturesPath();
}

void
MainWindow::loadNextFeatures(wxEvent & event)
{
  TheaArray<std::string> patterns;
  getFeaturePatterns(patterns);

  TheaArray<std::string> files;
  long index = fileIndex(app().options().features, model->getFeaturesPath(), files, &patterns);
  if (files.empty())
    return;

  if (index < 0 || index >= (long)files.size())  // maybe the file was deleted recently?
    index = 0;
  else if (files.size() == 1)
    return;

  if (index == (long)files.size() - 1)
    model->loadFeatures(files[0]);
  else
    model->loadFeatures(files[index + 1]);

  THEA_CONSOLE << "Loaded features " << model->getFeaturesPath();
}

void
MainWindow::clearOverlays()
{
  for (array_size_t i = 0; i < overlays.size(); ++i)
    delete overlays[i];

  overlays.clear();
}

void
MainWindow::toggleToolboxVisible(wxEvent & event)
{
  setToolboxVisible(!ui.toolbox->IsShown());
}

void
MainWindow::setToolboxVisible(wxCommandEvent & event)
{
  setToolboxVisible(event.IsChecked());
}

void
MainWindow::setToolboxVisible(bool value)
{
  if (ui.toolbox->IsShown() == value)
    return;

  ui.toolbox->Show(value);
  if (value)
    ui.main_splitter->SplitVertically(ui.model_display, ui.toolbox, -MIN_SPLIT_SIZE);
  else
    ui.main_splitter->Unsplit(ui.toolbox);
}

void
MainWindow::updateUI(wxUpdateUIEvent & event)
{
  // TODO
}

void
MainWindow::refreshDisplay(wxEvent & event)
{
  ui.model_display->Refresh();
}

void
MainWindow::OnExit(wxEvent & event)
{
  Close(true);
}

} // namespace MatTrans
