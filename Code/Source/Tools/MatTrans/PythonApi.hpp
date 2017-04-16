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

template< typename T >
inline
std::vector< T > toStdVector( bp::object const & iterable )
{
    return std::vector< T >( bp::stl_input_iterator< T >( iterable ),
                             bp::stl_input_iterator< T >( ) );
}

// From: http://stackoverflow.com/questions/1418015/how-to-get-python-exception-text
std::string handle_pyerror();

/** Wrapper for the mattrans pipeline functionality in Python. */
class PythonApi
{
  private:
    bp::object self_;

  public:
    //PythonApi(PyObject* self): self_(bp::handle<>(bp::borrowed(self))) { }
    PythonApi(bp::object self): self_(self) {
      THEA_CONSOLE << "PythonApi: Creating PythonApi C++ object...";
    }

    void loadResources(std::string const & image_dir_path, std::string const & retrieved_images_path)
    {
      try {
        THEA_CONSOLE << "PythonApi: Loading resources...";
        self_.attr("load_resources")(image_dir_path, retrieved_images_path);
        THEA_CONSOLE << "PythonApi: Loaded resources.";
      }
      PYTHON_API_CATCH()
    }

    std::vector<std::string> retrieveImages(Vector3 const & picked_point)
    {
      try {
        THEA_CONSOLE << "PythonApi: Retrieving images...";
        bp::object image_list = self_.attr("retrieve_images")(picked_point.x(), picked_point.y(), picked_point.z());
        std::vector<std::string> ret = toStdVector<std::string>(image_list);
        THEA_CONSOLE << "PythonApi: Retrieved images.";
        return ret;
      }
      PYTHON_API_CATCH()
    }
}; // class Model

shared_ptr<PythonApi> getPythonApi();

} // namespace MatTrans

#endif
