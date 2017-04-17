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

#ifndef __MatTrans_PythonApi_hpp__
#define __MatTrans_PythonApi_hpp__

#include "Math.hpp"
#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>

#define PYTHON_API_CATCH() \
  catch (bp::error_already_set) { \
    THEA_CONSOLE << "PythonApi: Caught python exception!"; \
    if (PyErr_Occurred()) { \
      std::string msg = handle_pyerror(); \
      THEA_CONSOLE << msg; \
    } \
    bp::handle_exception(); \
    PyErr_Clear(); \
    throw; \
  }

namespace bp = boost::python;

namespace MatTrans {

namespace PA {
//==================
// Data structures
//==================

struct Camera
{
  int camera_id;
  std::string camera_path;

  Camera(int camera_id, std::string const & camera_path)
    : camera_id(camera_id), camera_path(camera_path) {}

  Camera(bp::object dic)
    : camera_id(bp::extract<int>(dic["camera_id"])), camera_path(bp::extract<std::string>(dic["camera_path"])) {}
};

struct ClickedPoint2D
{
  int camera_id;
  Vector2 pt_2D;

  ClickedPoint2D(int camera_id, Vector2 const & pt_2D)
    : camera_id(camera_id), pt_2D(pt_2D) {}

  /**
   * Create a python dictionary to represent this object.
   */
  bp::dict to_pyobj() const
  {
    bp::dict ret;
    ret["camera_id"] = camera_id;
    bp::dict pt_2D_dic;
    pt_2D_dic["x"] = pt_2D.x();
    pt_2D_dic["y"] = pt_2D.y();
    ret["pt_2D"] = pt_2D_dic;
    return ret;
  }
};

template<typename T>
inline
std::vector<T> toStdVector(bp::object const & iterable)
{
  return std::vector<T>(bp::stl_input_iterator<T>(iterable),
                            bp::stl_input_iterator<T>());
}

// From: http://stackoverflow.com/questions/1418015/how-to-get-python-exception-text
std::string handle_pyerror();

/** Wrapper for the mattrans pipeline functionality in Python. */
class PythonApi
{
  private:
    bp::object self_;
    bool verbose_;

  public:
    PythonApi(bp::object self, bool verbose=true): self_(self), verbose_(verbose) {
      if (verbose_)
        THEA_CONSOLE << "PythonApi: Creating PythonApi C++ object...";
    }

    /**
     * Loads the resources associated to the current shape.
     */
    void loadResources(std::string const & dataset_dir,
      std::string const & experiment_dir, std::string const & shape_data_path);

    /**
     * Returns cameras which were used to render the current shape. We need
     * these to generate 2D points from the clicked 3D point on the shape and
     * each camera pose.
     */
    std::vector<Camera> getCameras();

    /**
     * Retrieves images which have the most relevant materials based on the
     * clicked 2D points for each rendered view of the current shape.
     */
    std::vector<std::string> retrieveImages(std::vector<ClickedPoint2D> const & clicked_points,
        TheaArray<Real> const & feat_3D);

}; // class PythonApi

shared_ptr<PythonApi> getPythonApi();

} // namespace PA

} // namespace MatTrans

#endif
