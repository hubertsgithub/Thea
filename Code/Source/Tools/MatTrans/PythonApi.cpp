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
// // INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//============================================================================

#include "PythonApi.hpp"

namespace MatTrans {

namespace PA {

// From: http://stackoverflow.com/questions/1418015/how-to-get-python-exception-text
std::string handle_pyerror()
{
    PyObject *exc, *val, *tb;
    bp::object formatted_list;
    PyErr_Fetch(&exc, &val, &tb);
    bp::handle<> hexc(exc), hval(bp::allow_null(val)), htb(bp::allow_null(tb));
    bp::object traceback(bp::import("traceback"));
    if (!tb) {
      bp::object format_exception_only(traceback.attr("format_exception_only"));
      formatted_list = format_exception_only(hexc, hval);
    } else {
      bp::object format_exception(traceback.attr("format_exception"));
      formatted_list = format_exception(hexc, hval, htb);
    }
    bp::object formatted = bp::str("\n").join(formatted_list);
    return bp::extract<std::string>(formatted);
}

shared_ptr<PythonApi> getPythonApi()
{
  Py_Initialize();
  try {
    bp::object sys_module = bp::import("sys");
    sys_module.attr("path").attr("insert")(0, ".");
    bp::object module = bp::import("python_api");
    bp::object pythonApi = module.attr("PythonApi")();
    THEA_CONSOLE << "PythonApi: Created PythonApi python object.";
    return shared_ptr<PythonApi>(new PythonApi(pythonApi));
    //PyObject* self = bp::extract<PyObject*>(pythonApi);
    //return shared_ptr<PythonApi>(new PythonApi(self));
  } catch (bp::error_already_set) {
    PyErr_Print();
    throw;
  }
}


//==================
// PythonApi
//==================

void PythonApi::loadResources(std::string const & dataset_dir,
  std::string const & experiment_dir, std::string const & shape_data_path)
{
  try {
    if (verbose_)
      THEA_CONSOLE << "PythonApi: Loading resources...";
    self_.attr("load_resources")(dataset_dir, experiment_dir, shape_data_path);
    if (verbose_)
      THEA_CONSOLE << "PythonApi: Loaded resources.";
  }
  PYTHON_API_CATCH()
}

std::vector<Camera> PythonApi::getCameras()
{
  try {
    if (verbose_)
      THEA_CONSOLE << "PythonApi: calling getCameras...";
    // Convert python list to C++ list
    std::vector<bp::object> camera_list = toStdVector<bp::object>(self_.attr("get_cameras")());
    // Convert each python dictionary in list to Camera objects
    std::vector<Camera> ret;
    for (std::vector<bp::object>::const_iterator it = camera_list.begin(); it != camera_list.end(); ++it) {
      Camera c(*it);
      if (verbose_)
        THEA_CONSOLE << "Camera: camera_id (" << c.camera_id << "), camera_path: (" << c.camera_path << ")";
      ret.push_back(c);
    }
    if (verbose_)
      THEA_CONSOLE << "PythonApi: getCameras finished.";
    return ret;
  }
  PYTHON_API_CATCH()
}

std::vector<std::string> PythonApi::retrieveImages(std::vector<ClickedPoint2D> const & clicked_points)
{
  try {
    if (verbose_)
      THEA_CONSOLE << "PythonApi: Retrieving images...";
    // Convert C++ list into python list (also convert objects inside)
    bp::list clicked_points_py;
    for (std::vector<ClickedPoint2D>::const_iterator it = clicked_points.begin(); it != clicked_points.end(); ++it) {
      clicked_points_py.append(it->to_pyobj());
    }

    bp::object image_list = self_.attr("retrieve_images")(clicked_points_py);
    std::vector<std::string> ret = toStdVector<std::string>(image_list);
    if (verbose_)
      THEA_CONSOLE << "PythonApi: Retrieved images.";
    return ret;
  }
  PYTHON_API_CATCH()
}

} // namespace PA

} // namespace MatTrans
