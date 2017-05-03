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
#include <numpy/ndarrayobject.h>

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

shared_ptr<PythonApi> getPythonApi(bool verbose /* = false */)
{
  Py_Initialize();
  _import_array();
  bp::numeric::array::set_module_and_type("numpy", "ndarray");
  try {
    bp::object sys_module = bp::import("sys");
    sys_module.attr("path").attr("insert")(0, "python");
    bp::object module = bp::import("python_api");
    bp::object pythonApi = module.attr("PythonApi")();
    THEA_CONSOLE << "PythonApi: Created PythonApi python object.";
    return shared_ptr<PythonApi>(new PythonApi(pythonApi, verbose));
    //PyObject* self = bp::extract<PyObject*>(pythonApi);
    //return shared_ptr<PythonApi>(new PythonApi(self));
  } catch (bp::error_already_set) {
    PyErr_Print();
    throw;
  }
}

// http://stackoverflow.com/questions/10701514/how-to-return-numpy-array-from-boostpython/34023333
bp::object array_to_numpy(TheaArray<Real> const & arr)
{
  npy_intp arr_size = npy_intp(arr.size());
  // const_cast is rather horrible but we need a writable pointer
  // in C++11, vec.data() will do the trick
  // but you will still need to const_cast
  Real* data = arr_size ? const_cast<Real*>(&arr[0]): static_cast<Real*>(NULL);

  PyObject* obj = PyArray_SimpleNewFromData(1, &arr_size, NPY_FLOAT, data);
  bp::handle<> handle(obj);
  bp::numeric::array nparr(handle);
  // The problem of returning nparr is twofold: firstly the user can modify
  // the data which will betray the const-correctness
  // Secondly the lifetime of the data is managed by the C++ API and not the
  // lifetime of the numpy array whatsoever. But we have a simple solution.
  // Copy the object. numpy owns the copy now.
  return nparr.copy();
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

TheaArray<Camera> PythonApi::getCameras()
{
  try {
    if (verbose_)
      THEA_CONSOLE << "PythonApi: calling getCameras...";
    // Convert python list to C++ list
    TheaArray<bp::object> camera_list = toStdVector<bp::object>(self_.attr("get_cameras")());
    // Convert each python dictionary in list to Camera objects
    TheaArray<Camera> ret;
    for (TheaArray<bp::object>::const_iterator it = camera_list.begin(); it != camera_list.end(); ++it) {
      Camera item(*it);
      if (verbose_)
        THEA_CONSOLE << "Camera: camera_id (" << item.camera_id << "), camera_path: (" << item.camera_path << ")";
      ret.push_back(item);
    }
    if (verbose_)
      THEA_CONSOLE << "PythonApi: getCameras finished.";
    return ret;
  }
  PYTHON_API_CATCH()
}

TheaArray<PhotoData> PythonApi::retrieveImages(
    TheaArray<ClickedPoint2D> const & clicked_points,
    TheaArray<Real> const & feat_3D, int feat_idx /* = 0 */,
    int feat_count /* = 0 */, bool do_visualize /* = false */)
{
  try {
    if (verbose_)
      THEA_CONSOLE << "PythonApi: Retrieving images...";
    // Convert C++ list into python list (also convert objects inside)
    bp::list clicked_points_py;
    for (TheaArray<ClickedPoint2D>::const_iterator it = clicked_points.begin(); it != clicked_points.end(); ++it) {
      clicked_points_py.append(it->to_pyobj());
    }

    // Convert feature stored in a vector to a numpy array
    bp::object feat_3D_py = array_to_numpy(feat_3D);
    TheaArray<bp::object> photo_list = toStdVector<bp::object>(
        self_.attr("retrieve_images")(clicked_points_py, feat_3D_py, feat_idx, feat_count, do_visualize));
    // Convert each python dictionary in list to PhotoData objects
    TheaArray<PhotoData> ret;
    for (TheaArray<bp::object>::const_iterator it = photo_list.begin(); it != photo_list.end(); ++it) {
      PhotoData item(*it);
      if (verbose_)
        THEA_CONSOLE << "PhotoData: photo_path (" << item.photo_path << "), rx: (" << item.rx << ") ry: (" << item.ry << ")";
      ret.push_back(item);
    }
    if (verbose_)
      THEA_CONSOLE << "PythonApi: Retrieved images.";
    return ret;
  }
  PYTHON_API_CATCH()
}

} // namespace PA

} // namespace MatTrans
