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

#include "Util.hpp"
#include "Math.hpp"
#include "Mesh.hpp"
#include "../../Algorithms/RayIntersectionTester.hpp"
#include "../../Graphics/Camera.hpp"
#include "../../Graphics/RenderSystem.hpp"
#include "../../Ball3.hpp"
#include "../../FileSystem.hpp"
#include "../../Image.hpp"
#include <wx/gdicmn.h>
#include <boost/functional/hash.hpp>
#include <cmath>
#include <limits>
#include <fstream>
#include <iostream>

namespace MatTrans {

// Polar axis (center to pole) is w
void
drawHemisphere(Graphics::RenderSystem & render_system, Vector3 const & center, Vector3 const & u, Vector3 const & v,
               Vector3 const & w, Real radius, int num_longitude_steps, int num_latitude_steps)
{
  using namespace Graphics;

  alwaysAssertM(num_longitude_steps >= 3, "A hemisphere must be drawn with at least 3 longitudinal steps");
  alwaysAssertM(num_latitude_steps >= 1, "A hemisphere must be drawn with at least 3 latitudinal steps");

  float longitude_conv_factor = (float)Math::twoPi() / num_longitude_steps;
  float latitude_conv_factor = (float)Math::halfPi() / num_latitude_steps;

  float prev_s_lat = 0;
  float prev_c_lat = 1;

  Vector3 offset_dir, offset;

  for (int i = 1; i <= num_latitude_steps; ++i)
  {
    float latitude = latitude_conv_factor * i;
    float s_lat = Math::fastSin(latitude);
    float c_lat = Math::fastCos(latitude);

    if (i > 1)
    {
      render_system.beginPrimitive(RenderSystem::Primitive::QUAD_STRIP);

        for (int j = 0; j <= num_longitude_steps; ++j)
        {
          float longitude = longitude_conv_factor * j;
          float s_lng = Math::fastSin(longitude);
          float c_lng = Math::fastCos(longitude);

          offset_dir = (c_lng * prev_s_lat) * u + (s_lng * prev_s_lat) * v + prev_c_lat * w;
          offset = radius * offset_dir;
          render_system.sendNormal(offset_dir);
          render_system.sendVertex(center + offset);

          offset_dir = (c_lng * s_lat) * u + (s_lng * s_lat) * v + c_lat * w;
          offset = radius * offset_dir;
          render_system.sendNormal(offset_dir);
          render_system.sendVertex(center + offset);
        }

      render_system.endPrimitive();
    }
    else
    {
      render_system.beginPrimitive(RenderSystem::Primitive::TRIANGLE_FAN);

        render_system.sendNormal(w);
        render_system.sendVertex(center + radius * w);

        for (int j = 0; j <= num_longitude_steps; ++j)
        {
          float longitude = longitude_conv_factor * j;
          float s_lng = Math::fastSin(longitude);
          float c_lng = Math::fastCos(longitude);

          offset_dir = (c_lng * s_lat) * u + (s_lng * s_lat) * v + c_lat * w;
          offset = radius * offset_dir;
          render_system.sendNormal(offset_dir);
          render_system.sendVertex(center + offset);
        }

      render_system.endPrimitive();
    }

    prev_s_lat = s_lat;
    prev_c_lat = c_lat;
  }
}

void
drawSphere(Graphics::RenderSystem & render_system, Vector3 const & center, Real radius, int num_steps)
{
  int num_latitude_steps = (int)std::ceil(num_steps / 4.0);

  drawHemisphere(render_system, center, Vector3::unitX(), Vector3::unitY(),  Vector3::unitZ(), radius, num_steps,
                 num_latitude_steps);
  drawHemisphere(render_system, center,  Vector3::unitX(), Vector3::unitY(), -Vector3::unitZ(), radius, num_steps,
                 num_latitude_steps);
}

void
drawCylinder(Graphics::RenderSystem & render_system, Vector3 const & base_center, Vector3 const & top_center, Vector3 const & u,
             Vector3 const & v, Real radius, int num_steps)
{
  using namespace Graphics;

  alwaysAssertM(num_steps >= 3, "A cylinder must be drawn with at least 3 steps");

  float conv_factor = (float)Math::twoPi() / num_steps;

  render_system.beginPrimitive(RenderSystem::Primitive::QUAD_STRIP);

    for (int i = 0; i <= num_steps; ++i)
    {
      float angle = conv_factor * i;
      float s = Math::fastSin(angle);
      float c = Math::fastCos(angle);
      Vector3 offset_dir = s * u + c * v;
      Vector3 offset = radius * offset_dir;
      render_system.sendNormal(offset_dir);
      render_system.sendVertex(base_center + offset);
      render_system.sendVertex(top_center + offset);
    }

  render_system.endPrimitive();
}

void
drawCapsule(Graphics::RenderSystem & render_system, Vector3 const & base_center, Vector3 const & top_center, Real radius,
            int num_steps)
{
  Vector3 w = (top_center - base_center).fastUnit();
  Vector3 u, v;
  w.createOrthonormalBasis(u, v);

  // Sides
  drawCylinder(render_system, base_center, top_center, u, v, radius, num_steps);

  // Base and top
  int num_latitude_steps = std::ceil(num_steps / 4.0);
  drawHemisphere(render_system, base_center, u, v, -w, radius, num_steps, num_latitude_steps);
  drawHemisphere(render_system, top_center,  u, v,  w, radius, num_steps, num_latitude_steps);
}

void
drawTorus(Graphics::RenderSystem & render_system, Vector3 const & center, Vector3 const & u, Vector3 const & v, Real radius,
          Real width, int num_major_steps, int num_minor_steps, bool alternate_colors, ColorRGBA const & color1,
          ColorRGBA const & color2)
{
  using namespace Graphics;

  float major_conv_factor = (float)Math::twoPi() / num_major_steps;
  float minor_conv_factor = (float)Math::twoPi() / num_minor_steps;

  Vector3 w = u.cross(v);

  if (alternate_colors)
    render_system.pushColorFlags();

  render_system.beginPrimitive(RenderSystem::Primitive::QUAD_STRIP);

    for (int i = 0; i < num_minor_steps; ++i)
    {
      float minor_angle = minor_conv_factor * i;
      float s_min = Math::fastSin(minor_angle);
      float c_min = Math::fastCos(minor_angle);

      float s_min_offset = Math::fastSin(minor_conv_factor * (i + 1)) - s_min;
      float c_min_offset = Math::fastCos(minor_conv_factor * (i + 1)) - c_min;

      for (int j = 0; j <= num_major_steps; ++j)
      {
        float major_angle = major_conv_factor * j;
        float s_maj = Math::fastSin(major_angle);
        float c_maj = Math::fastCos(major_angle);

        Vector3 p(c_maj * (radius + width * c_min),
                  s_maj * (radius + width * c_min),
                  width * s_min);
        Vector3 n(c_maj * c_min, s_maj * c_min, s_min);

        if (alternate_colors)
          render_system.setColor(j & 0x01 ? color2 : color1);

        render_system.sendNormal(n.x() * u + n.y() * v + n.z() * w);
        render_system.sendVertex(center + p.x() * u + p.y() * v + p.z() * w);

        p += Vector3(c_maj * width * c_min_offset, s_maj * width * c_min_offset, width * s_min_offset);
        n += Vector3(c_maj * c_min_offset, s_maj * c_min_offset, s_min_offset);
        render_system.sendNormal(n.x() * u + n.y() * v + n.z() * w);
        render_system.sendVertex(center + p.x() * u + p.y() * v + p.z() * w);
      }
    }

  render_system.endPrimitive();

  if (alternate_colors)
    render_system.popColorFlags();
}

Ray3
computePickRay(wxRealPoint const & p, Graphics::Camera const & camera, int width, int height)
{
  Vector2 screen_pos = Vector2(2 * p.x / (Real)width - 1, 1 - 2 * p.y / (Real)height);
  return camera.computePickRay(screen_pos);
}

Vector3
dragToTranslation(wxPoint const & start, wxPoint const & end, int width, int height, Graphics::Camera const & camera,
                  Real object_distance)
{
  // Remember pixel coordinates increase top to bottom, so diff.y() is downwards in camera space

  wxPoint diff = end - start;

  Real d_scale = object_distance / camera.getNearDistance();
  Real horz_scale = d_scale * (camera.getRightMargin() - camera.getLeftMargin()) / (Real)width;
  Real vert_scale = d_scale * (camera.getTopMargin() - camera.getBottomMargin()) / (Real)height;

  return diff.x * horz_scale * camera.getRightDirection()
       - diff.y * vert_scale * camera.getUpDirection();
}

Matrix3
dragToRotation(wxPoint const & start, wxPoint const & end, int width, int height, Graphics::Camera const & camera)
{
  // Remember pixel coordinates increase top to bottom, so diff.y() is downwards in camera space

  wxPoint diff = end - start;
  if (diff.x == 0 && diff.y == 0) return Matrix3::identity();

  Vector3 axis = (Real)diff.y * camera.getRightDirection() + (Real)diff.x * camera.getUpDirection();

  static Real const ROT_SPEED = 5;
  int size = width < height ? width : height;
  Real angle = ROT_SPEED * Vector2(diff.x, -diff.y).length() / size;

  return Matrix3::rotationAxisAngle(axis, angle);
}

Matrix3
dragToRotationAroundAxis(wxPoint const & start, Vector3 const & start_pick, wxPoint const & end, Vector3 const & axis,
                         Vector3 const & center, int width, int height, Graphics::Camera const & camera)
{
  // Remember pixel coordinates increase top to bottom, so diff.y() is downwards in camera space

  Vector3 axis_dir = axis.fastUnit();
  Vector3 offset = start_pick - center;
  Vector3 perp_offset = offset - offset.dot(axis_dir) * axis_dir;
  Vector3 tangent = axis.cross(perp_offset).fastUnit();
  Vector2 proj_tangent = (camera.getWorldToCameraTransform().getRotation() * tangent).xy().fastUnit();

  wxPoint diff = end - start;
  if (diff.x == 0 && diff.y == 0) return Matrix3::identity();

  static Real const ROT_SPEED = 0.02f;
  Real angle = ROT_SPEED * (diff.x * proj_tangent.x() - diff.y * proj_tangent.y());

  return Matrix3::rotationAxisAngle(axis, angle);
}

Matrix3
dragToJoystickRotation(wxPoint const & start, wxPoint const & end, Vector3 const & center, Real offset, int width, int height,
                       Graphics::Camera const & camera)
{
  // Remember pixel coordinates increase top to bottom, so diff.y() is downwards in camera space

  Ray3 rays[2] = { computePickRay(start, camera, width, height), computePickRay(end, camera, width, height) };

  Vector3 dirs[2];
  for (int i = 0; i < 2; ++i)
  {
    Ball3 ball(center, offset);
    Real t = ball.rayIntersectionTime(rays[i]);
    Vector3 p = (t >= 0 ? rays[i].getPoint(t) : rays[i].closestPoint(center));

    dirs[i] = (p - center).fastUnit();
  }

  return Matrix3::rotationArc(dirs[0], dirs[1], false);
}

Real
dragToScale(wxPoint const & start, wxPoint const & end, int width, int height, Graphics::Camera const & camera)
{
  // Remember pixel coordinates increase top to bottom, so diff.y() is downwards in camera space

  wxPoint diff = end - start;

  static Real const SCALE_INC_SPEED = 4;
  static Real const SCALE_DEC_SPEED = 2;
  static Real const MIN_SCALE = 0.25;

  if (diff.y < 0)  // drag up => increase
    return std::max(1 - SCALE_INC_SPEED * diff.y / (Real)height, MIN_SCALE);
  else  // drag down => decrease
    return std::max(1 - SCALE_DEC_SPEED * diff.y / (Real)height, MIN_SCALE);
}

int const NUM_PALETTE_COLORS = 24;
ColorRGB COLOR_PALETTE[NUM_PALETTE_COLORS] = {
  ColorRGB::fromARGB(0x298edb),
  ColorRGB::fromARGB(0x982411),
  ColorRGB::fromARGB(0x6d4e25),
  ColorRGB::fromARGB(0x1b5043),
  ColorRGB::fromARGB(0x6e7662),
  ColorRGB::fromARGB(0xa08b00),
  ColorRGB::fromARGB(0x58427b),
  ColorRGB::fromARGB(0x1d2f5b),
  ColorRGB::fromARGB(0xac5e34),
  ColorRGB::fromARGB(0x804055),
  ColorRGB::fromARGB(0x6d7a00),
  ColorRGB::fromARGB(0x572e2c),

  // Invert each color above
  ColorRGB::fromARGB(~0x298edb),
  ColorRGB::fromARGB(~0x982411),
  ColorRGB::fromARGB(~0x6d4e25),
  ColorRGB::fromARGB(~0x1b5043),
  ColorRGB::fromARGB(~0x6e7662),
  ColorRGB::fromARGB(~0xa08b00),
  ColorRGB::fromARGB(~0x58427b),
  ColorRGB::fromARGB(~0x1d2f5b),
  ColorRGB::fromARGB(~0xac5e34),
  ColorRGB::fromARGB(~0x804055),
  ColorRGB::fromARGB(~0x6d7a00),
  ColorRGB::fromARGB(~0x572e2c)
};

int
numPaletteColors()
{
  return NUM_PALETTE_COLORS;
}

ColorRGB const &
getPaletteColor(long i)
{
  long n = numPaletteColors();
  long index;
  if (i < 0)
    index = (n - ((-i) % n)) % n;
  else
    index = i % n;

  return COLOR_PALETTE[index];
}

ColorRGB
getLabelColor(std::string const & label)
{
  boost::hash<std::string> hasher;
  Random rnd((uint32)hasher(label));

  return ColorRGB(rnd.uniform01(), rnd.uniform01(), rnd.uniform01());
}

bool
loadImage(Image & image, std::string const & path)
{
  if (!FileSystem::fileExists(path)) return false;

  image.load(path);
  if (image.isValid())
  {
    // This used to be necessary on Linux. No longer, since we've fixed TextureFormat::fromImageType() to detect and handle BGR
    // images.
    // fixChannelOrdering(image);

    return true;
  }

  return false;
}

void
fixChannelOrdering(Image & image)
{
  // Make sure the red channel is at pixel[0] and the blue channel at pixel[2]
  int bytes_pp = 0;
  switch (image.getType())
  {
    case Image::Type::RGB_8U: bytes_pp = 3; break;
    case Image::Type::RGBA_8U: bytes_pp = 4; break;
    default: break;
  }

  if (bytes_pp > 0)
  {
    for (int r = 0; r < image.getHeight(); ++r)
    {
      uint8 * pixels = (uint8 *)image.getScanLine(r);
      for (int c = 0; c < image.getWidth(); ++c)
      {
        uint8 * pixel = pixels + bytes_pp * c;

        uint8 red    =  pixel[Image::Channel::RED];
        uint8 green  =  pixel[Image::Channel::GREEN];
        uint8 blue   =  pixel[Image::Channel::BLUE];

        pixel[0] = red;
        pixel[1] = green;
        pixel[2] = blue;
      }
    }
  }
}


bool loadCamera(std::string const & camera_filepath, Graphics::Camera & loaded_camera)
{
  if (!FileSystem::fileExists(camera_filepath))
    return false;

  Matrix3 rot;
  Vector3 tr;
  char projtype[20];
  Real left, right, bottom, top, near, far;
  char updir[20];
  std::string camstr;
  std::ifstream camfile(camera_filepath.c_str());
  std::getline(camfile, camstr);
  camfile.close();

  sscanf(camstr.c_str(), "Frame = [R: [%f, %f, %f; %f, %f, %f; %f, %f, %f], T: (%f, %f, %f)], ProjectionType = %s "
      "Left = %f, Right = %f, Bottom = %f, Top = %f, NearDist = %f, FarDist = %f, Projected Y increases %s",
      &rot[0], &rot[1], &rot[2], &rot[3], &rot[4], &rot[5], &rot[6], &rot[7], &rot[8], &tr[0], &tr[1], &tr[2], projtype,
      &left, &right, &bottom, &top, &near, &far, updir);

  CoordinateFrame3 cf;
  cf._setRotation(rot);
  cf.setTranslation(tr);

  Graphics::Camera cam(cf, (std::string(projtype) == "Perspective," ? Graphics::Camera::ProjectionType::PERSPECTIVE : Graphics::Camera::ProjectionType::ORTHOGRAPHIC), left, right, bottom, top, near, far, (std::string(updir) == "upwards" ? Graphics::Camera::ProjectedYDirection::UP: Graphics::Camera::ProjectedYDirection::DOWN) );

  loaded_camera = cam;
  return true;
}


TheaArray<PA::ClickedPoint2D> projectClickedPoint(
    TheaArray<PA::Camera> const & cameras, Vector3 const & picked_pt,
    Thea::Algorithms::MeshKDTree<Mesh> const & kdtree, bool verbose /*= false*/)
{
  TheaArray<PA::ClickedPoint2D> clicked_points;
  for (TheaArray<PA::Camera>::const_iterator it = cameras.begin(); it != cameras.end(); ++it) {
    Graphics::Camera cam;
    if (!loadCamera(it->camera_path, cam)) {
      THEA_WARNING << "Couldn't load camera from path: " << it->camera_path;
      continue;
    }

    if (verbose)
      THEA_CONSOLE << "Loaded camera frame: " << cam.getFrame();
    // Project the 3D point on the screen corresponding to the camera
    Vector2 pt_2D = cam.project(picked_pt).xy();
    if (verbose)
      THEA_CONSOLE << "Clicked point 2D projection: " << pt_2D;
    if (pt_2D.x() < -1.0 || pt_2D.x() > 1.0 || pt_2D.y() < -1.0 || pt_2D.y() > 1.0) {
      if (verbose)
        THEA_CONSOLE << "Clicked point projects outside viewport, skipping!";
      continue;
    }

    // Test if the point was occluded
    // Cast a ray and see if it intersects the shape at the same 3D point
    Ray3 ray = cam.computePickRay(pt_2D);
    // Intersect with the shape in the canonical frame here, not the
    // transformed shape we currently see on the shape
    Real t = kdtree.rayIntersectionTime<Algorithms::RayIntersectionTester>(ray);

    // No intersection, shouldn't happen
    if (t < 0) {
      THEA_WARNING << "Intersection expected, but did not happen! t: " << t;
      continue;
    }
    Vector3 pt_3D_backproj = ray.getPoint(t);
    if (verbose)
      THEA_CONSOLE << "Back projected point: " << pt_3D_backproj;
    Real dist = (pt_3D_backproj - picked_pt).length();
    // If the original point and the backprojected point are far away, the
    // point is occluded in this view.
    if (dist > 0.01f) {
      if (verbose)
        THEA_CONSOLE << "Clicked point occluded in a view!";
      continue;
    }
    clicked_points.push_back(PA::ClickedPoint2D(it->camera_id, pt_2D));
  }

  return clicked_points;
}

std::istream &
getNextNonBlankLine(std::istream & in, std::string & line)
{
  while (std::getline(in, line))
  {
    if (!trimWhitespace(line).empty())
      break;
  }

  return in;
}

bool load3DFeatures(
    std::string const & path, TheaArray<Vector3> & feat_pts,
    TheaArray< TheaArray<Real> > & features)
{
  feat_pts.clear();

  if (endsWith(toLower(path), ".bin"))
  {
    BinaryInputStream in(path, Endianness::LITTLE);
    long num_points = in.readInt64();
    long num_features = in.readInt64();
    if (num_points < 0)
      throw Error(format("Invalid number of points: %ld", num_points));

    if (num_features < 0)
      throw Error(format("Invalid number of features: %ld", num_features));

    feat_pts.resize((array_size_t)num_points);
    features.resize((array_size_t)num_points);
    for (array_size_t i = 0; i < feat_pts.size(); ++i)
    {
      feat_pts[i][0] = in.readFloat32();
      feat_pts[i][1] = in.readFloat32();
      feat_pts[i][2] = in.readFloat32();

      features[i].resize((array_size_t)num_features);
      for (array_size_t j = 0; j < features[i].size(); ++j)
        features[i][j] = in.readFloat32();
    }
  }
  else
  {
    std::ifstream in(path.c_str());
    if (!in)
      throw Error("Couldn't open file");

    std::string line;
    Vector3 p;
    Real f;
    while (getNextNonBlankLine(in, line))
    {
      std::istringstream line_in(line);
      if (!(line_in >> p[0] >> p[1] >> p[2]))
        throw Error(format("Couldn't read position of feature point %ld", (long)feat_pts.size()));

      feat_pts.push_back(p);
      features.push_back(TheaArray<Real>());

      while (line_in >> f)
        features.back().push_back(f);

      if (features.size() > 1 && features.back().size() != features.front().size())
      {
        throw Error(format("Feature point %ld expects %ld feature(s), has %ld",
                          (long)feat_pts.size() - 1, features.front().size(), features.back().size()));
      }
    }
  }
  return true;
}

bool CollectVerticesFunctor::operator()(Mesh & mesh)
{
  for (Mesh::VertexIterator vi = mesh.verticesBegin(); vi != mesh.verticesEnd(); ++vi)
    verts->push_back(&(*vi));

  return false;
}

} // namespace MatTrans
