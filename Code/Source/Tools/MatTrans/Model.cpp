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

#include "Model.hpp"
#include "App.hpp"
#include "BinaryInputStream.hpp"
#include "MainWindow.hpp"
#include "Math.hpp"
#include "Mesh.hpp"
#include "ModelDisplay.hpp"
#include "PointCloud.hpp"
#include "Util.hpp"
#include "../../Algorithms/KDTreeN.hpp"
#include "../../Algorithms/MetricL2.hpp"
#include "../../Algorithms/RayIntersectionTester.hpp"
#include "../../Graphics/MeshCodec.hpp"
#include "../../BoundedSortedArrayN.hpp"
#include "../../Colors.hpp"
#include "../../FilePath.hpp"
#include "../../FileSystem.hpp"
#include <wx/filedlg.h>
#include <algorithm>
#include <fstream>

wxDEFINE_EVENT(EVT_MODEL_PATH_CHANGED,         wxCommandEvent);
wxDEFINE_EVENT(EVT_MODEL_GEOMETRY_CHANGED,     wxCommandEvent);
wxDEFINE_EVENT(EVT_MODEL_NEEDS_REDRAW,         wxCommandEvent);

namespace MatTrans {

namespace ModelInternal {

bool first_file_dialog = true;

std::string
getWorkingDir()
{
  if (first_file_dialog)
    if (!app().options().working_dir.empty() && FileSystem::directoryExists(app().options().working_dir))
      return app().options().working_dir;

  return std::string();
}

bool
enableGPURendering(Mesh & mesh)
{
  mesh.setGPUBufferedRendering(true);
  mesh.setGPUBufferedWireframe(true);
  return false;
}

bool
disableGPURendering(Mesh & mesh)
{
  mesh.setGPUBufferedRendering(false);
  mesh.setGPUBufferedWireframe(false);
  return false;
}

void
linkMeshesToParent(MeshGroupPtr mesh_group)
{
  for (MeshGroup::MeshConstIterator mi = mesh_group->meshesBegin(); mi != mesh_group->meshesEnd(); ++mi)
    (*mi)->setParent(mesh_group.get());

  for (MeshGroup::GroupConstIterator ci = mesh_group->childrenBegin(); ci != mesh_group->childrenEnd(); ++ci)
    linkMeshesToParent(*ci);
}

static ColorRGBA const DEFAULT_COLOR(1.0f, 0.9f, 0.8f, 1.0f);
static ColorRGBA const PICKED_SEGMENT_COLOR(0.4f, 0.69f, 0.21f, 1.0f);

} // namespace ModelInternal

Model::Model(std::string const & initial_mesh)
: has_features(false),
  feat_pts_kdtree(NULL),
  has_elem_labels(false),
  color(ModelInternal::DEFAULT_COLOR),
  valid_kdtree(true),
  kdtree(new KDTree),
  valid_vertex_kdtree(true),
  vertex_kdtree(new VertexKDTree)
{
  load(initial_mesh);
}

Model::~Model()
{
  delete feat_pts_kdtree;
  delete vertex_kdtree;
  delete kdtree;
}

std::string
Model::getName() const
{
  if (mesh_group)
    return mesh_group->getName();
  else if (point_cloud)
    return point_cloud->getName();
  else
    return "Untitled";
}

bool
Model::isEmpty() const
{
  return (!mesh_group || mesh_group->isEmpty()) && (!point_cloud || point_cloud->isEmpty());
}

void
Model::clear()
{
  clearMesh();
  clearPoints();
  invalidateAll();
}

void
Model::clearMesh()
{
  if (mesh_group) mesh_group->clear();
  has_features = false;
  has_elem_labels = false;
}

void
Model::clearPoints()
{
  if (point_cloud) point_cloud->clear();
}

bool
Model::load(std::string path_)
{
  if (path_.empty())
    return false;

  path_ = FileSystem::resolve(path_);
  if (!FileSystem::fileExists(path_) || path_ == FileSystem::resolve(path))
    return false;

  if (endsWith(toLower(path_), ".pts"))
  {
    clear();

    point_cloud = PointCloudPtr(new PointCloud);
    if (!point_cloud->load(path_))
      return false;

    bounds = point_cloud->getBounds();
    path = path_;
  }
  else
  {
    MeshGroupPtr new_mesh_group(new MeshGroup("Mesh Group"));

    Mesh::resetVertexIndices();  // reset counting
    Mesh::resetFaceIndices();

    static CodecOBJ<Mesh> const obj_codec(CodecOBJ<Mesh>::ReadOptions().setIgnoreTexCoords(true));
    try
    {
      if (endsWith(toLower(path_), ".obj"))
        new_mesh_group->load(path_, obj_codec);
      else
        new_mesh_group->load(path_);
    }
    THEA_STANDARD_CATCH_BLOCKS(return false;, ERROR, "Couldn't load model '%s'", path_.c_str())

    invalidateAll();

    ModelInternal::linkMeshesToParent(new_mesh_group);

    mesh_group = new_mesh_group;
    clearPoints();

    bounds = new_mesh_group->getBounds();

    THEA_CONSOLE << "Loaded model '" << path_ << "' with bounding box " << mesh_group->getBounds().toString();

    path = path_;

    loadFeatures(getDefaultFeaturesPath());
    loadShapeData(app().options().dataset_dir, app().options().experiment_dir, app().options().shape_data);
  }

  loadElementLabels(getDefaultElementLabelsPath());

  wxPostEvent(this, wxCommandEvent(EVT_MODEL_PATH_CHANGED));
  wxPostEvent(this, wxCommandEvent(EVT_MODEL_GEOMETRY_CHANGED));

  return true;
}

bool
Model::selectAndLoad()
{
  wxFileDialog file_dialog(app().getMainWindow(), "Load model", "", "",
                           "Model files (*.3ds *.obj *.off *.off.bin *.ply *.pts)|"
                               "*.3ds;*.3DS;*.obj;*.OBJ;*.off;*.OFF;*.off.bin;*.OFF.BIN;*.OFF.bin;*.off.BIN;"
                               "*.ply;*.PLY;*.pts;*.PTS",
                           wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  if (file_dialog.ShowModal() == wxID_CANCEL)
      return false;

  bool success = load(file_dialog.GetPath().ToStdString());
  if (success)
    ModelInternal::first_file_dialog = false;

  return success;
}

void
Model::setTransform(AffineTransform3 const & trans_)
{
  TransformableBaseT::setTransform(trans_);

  if (valid_kdtree)
    kdtree->setTransform(trans_);

  if (valid_vertex_kdtree)
    vertex_kdtree->setTransform(trans_);

  if (has_features && feat_pts_kdtree)
    feat_pts_kdtree->setTransform(trans_);
}

void
Model::clearTransform()
{
  TransformableBaseT::clearTransform();

  if (valid_kdtree)
    kdtree->clearTransform();

  if (valid_vertex_kdtree)
    vertex_kdtree->clearTransform();

  if (has_features && feat_pts_kdtree)
    feat_pts_kdtree->clearTransform();
}

void
Model::invalidateAll()
{
  invalidateVertexKDTree();
  invalidateKDTree();
}

void
Model::invalidateKDTree()
{
  valid_kdtree = false;
}

void
Model::updateKDTree() const
{
  if (valid_kdtree) return;

  kdtree->clear(false);

  if (mesh_group)
  {
    kdtree->add(*mesh_group);
    kdtree->init();

    if (hasTransform())
      kdtree->setTransform(getTransform());

    THEA_CONSOLE << getName() << ": Updated kd-tree";
  }

  valid_kdtree = true;
}

void
Model::invalidateVertexKDTree()
{
  valid_vertex_kdtree = false;
}

namespace ModelInternal {

struct CollectVerticesFunctor
{
  CollectVerticesFunctor(TheaArray<MeshVertex *> * verts_) : verts(verts_) {}

  bool operator()(Mesh & mesh)
  {
    for (Mesh::VertexIterator vi = mesh.verticesBegin(); vi != mesh.verticesEnd(); ++vi)
      verts->push_back(&(*vi));

    return false;
  }

  TheaArray<MeshVertex *> * verts;

}; // struct CollectVerticesFunctor

} // namespace ModelInternal

void
Model::updateVertexKDTree() const
{
  if (valid_vertex_kdtree) return;

  vertex_kdtree->clear(false);

  TheaArray<MeshVertex *> verts;
  ModelInternal::CollectVerticesFunctor func(&verts);
  mesh_group->forEachMeshUntil(&func);
  vertex_kdtree->init(verts.begin(), verts.end());

  if (hasTransform())
    vertex_kdtree->setTransform(getTransform());

  valid_vertex_kdtree = true;
}

Model::KDTree const &
Model::getKDTree(bool recompute_if_invalid) const
{
  if (recompute_if_invalid)
    updateKDTree();

  return *kdtree;
}

Model::VertexKDTree const &
Model::getVertexKDTree(bool recompute_if_invalid) const
{
  if (recompute_if_invalid)
    updateVertexKDTree();

  return *vertex_kdtree;
}

bool
Model::rayIntersects(Ray3 const & ray, Real max_time) const
{
  return getKDTree().rayIntersects<Algorithms::RayIntersectionTester>(ray, max_time);
}

Real
Model::rayIntersectionTime(Ray3 const & ray, Real max_time) const
{
  return getKDTree().rayIntersectionTime<Algorithms::RayIntersectionTester>(ray, max_time);
}

Model::RayStructureIntersection3
Model::rayIntersection(Ray3 const & ray, Real max_time) const
{
  return getKDTree().rayStructureIntersection<Algorithms::RayIntersectionTester>(ray, max_time);
}

long
Model::closestPoint(Vector3 const & query, Real distance_bound, Real * min_dist, Vector3 * closest_pt,
                    Vector3 * closest_pt_normal, bool accelerate_with_vertices) const
{
  using namespace Algorithms;

  if (!isEmpty())
  {
    if (accelerate_with_vertices)
    {
      // Tighten the bound as much as we can with a fast initial query on the set of vertices
      updateVertexKDTree();
      double fast_distance_bound = 0;
      long vertex_index = vertex_kdtree->closestElement<MetricL2>(query, distance_bound, &fast_distance_bound);
      if (vertex_index >= 0)
        distance_bound = (Real)fast_distance_bound;
    }

    updateKDTree();
    double d = 0;
    long index = kdtree->closestElement<MetricL2>(query, distance_bound, &d, closest_pt);
    if (index >= 0)
    {
      if (min_dist) *min_dist = (Real)d;
      if (closest_pt_normal) *closest_pt_normal = kdtree->getElements()[index].getNormal();
      return index;
    }
  }

  return -1;
}

Real
Model::pick(Ray3 const & ray)
{
  using namespace Algorithms;

  valid_pick = false;

  if (!has_features || !feat_pts_kdtree)
    return -1;

  Real t = rayIntersectionTime(ray);
  if (t > 0)
  {
    Vector3 p = ray.getPoint(t);
    picked_feat_pt_index = feat_pts_kdtree->closestElement<MetricL2>(p);
    if (picked_feat_pt_index >= 0)
    {
      picked_feat_pt_position = feat_pts_kdtree->getElements()[picked_feat_pt_index];
      valid_pick = true;
    }

    wxPostEvent(this, wxCommandEvent(EVT_MODEL_NEEDS_REDRAW));
  }

  return t;
}

void
Model::invalidatePick()
{
  valid_pick = false;
  wxPostEvent(this, wxCommandEvent(EVT_MODEL_NEEDS_REDRAW));
}

void
Model::processPick()
{
  if (!valid_pick)
    return;

  // Model path: path
  // Position of picked point: picked_feat_pt_position
  // Features of picked point: features[(array_size_t)picked_feat_pt_index]
  THEA_CONSOLE << "Picked feature point " << picked_feat_pt_index << " at " << picked_feat_pt_position;

  std::vector<PA::Camera> cameras = python_api->getCameras();
  std::vector<PA::ClickedPoint2D> clicked_points;
  for (std::vector<PA::Camera>::const_iterator it = cameras.begin(); it != cameras.end(); ++it) {
    // TODO: Load camera parameters from file and compute 2D coordinate of picked point from that camera
    // it->camera_path
    Vector2 pt_2D;
    clicked_points.push_back(PA::ClickedPoint2D(it->camera_id, pt_2D));
  }

  std::vector<std::string> image_paths = python_api->retrieveImages(
      clicked_points, features[(array_size_t)picked_feat_pt_index]);
  THEA_CONSOLE << image_paths[0];
  // TODO: Load images and show in new window
}

void
Model::mousePressEvent(wxMouseEvent & event)
{
  processPick();
  event.StopPropagation();
}

void
Model::mouseMoveEvent(wxMouseEvent & event)
{
  // Currently no-op
}

void
Model::mouseReleaseEvent(wxMouseEvent & event)
{
  // Currently no-op
}

namespace ModelInternal {

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

} // namespace ModelInternal

bool
Model::loadFeatures(std::string const & path_)
{
  using namespace ModelInternal;

  features_path = path_;
  delete feat_pts_kdtree; feat_pts_kdtree = NULL;

  if (point_cloud)
  {
    has_features = point_cloud->loadFeatures(path_);
    return has_features;
  }

  if (!mesh_group)
  {
    has_features = false;
    return has_features;
  }

  has_features = true;
  try
  {
    TheaArray<Vector3> feat_pts;

    if (endsWith(toLower(path_), ".bin"))
    {
      BinaryInputStream in(path_, Endianness::LITTLE);
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
      std::ifstream in(path_.c_str());
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

    if (feat_pts.empty())
    {
      wxPostEvent(this, wxCommandEvent(EVT_MODEL_NEEDS_REDRAW));
      return true;
    }

    if (!feat_pts_kdtree)
      feat_pts_kdtree = new PointKDTree;

    feat_pts_kdtree->init(feat_pts.begin(), feat_pts.end());
    feat_pts_kdtree->enableNearestNeighborAcceleration();
  }
  THEA_STANDARD_CATCH_BLOCKS(has_features = false;, WARNING, "Couldn't load model features from '%s'", path_.c_str())

  if (!features.empty())
    THEA_CONSOLE << "Loaded features for " << features.size() << " point(s) from '" << path_ << '\'';

  wxPostEvent(this, wxCommandEvent(EVT_MODEL_NEEDS_REDRAW));

  return has_features;
}

bool
Model::loadShapeData(std::string const & dataset_dir_,
    std::string const & experiment_dir_, std::string const & shape_data_path_)
{
  THEA_CONSOLE << "Initializing PythonApi...";
  python_api = PA::getPythonApi();
  THEA_CONSOLE << "Initialized PythonApi";

  python_api->loadResources(dataset_dir_, experiment_dir_, shape_data_path_);

  return true;
}

namespace ModelInternal {

class FaceLabeler
{
  public:
    FaceLabeler(TheaArray<ColorRGBA> const & elem_colors_) : elem_colors(elem_colors_) {}

    bool operator()(Mesh & mesh) const
    {
      for (Mesh::FaceIterator fi = mesh.facesBegin(); fi != mesh.facesEnd(); ++fi)
      {
        long index = fi->attr().getIndex();
        if (index < 0 || index >= (long)elem_colors.size())
          throw Error("Face index out of range of face labels array");

        fi->attr().setColor(elem_colors[(array_size_t)index]);
      }

      return false;
    }

  private:
    TheaArray<ColorRGBA> const & elem_colors;
};

} // namespace ModelInternal

bool
Model::loadElementLabels(std::string const & path_)
{
  has_elem_labels = false;

  if (!mesh_group && !point_cloud)
    return has_elem_labels;

  std::ifstream in(path_.c_str());
  if (!in)
  {
    THEA_WARNING << "Couldn't open face labels file '" << path_ << '\'';
    return has_elem_labels;
  }

  TheaArray<ColorRGBA> elem_colors;
  std::string line;
  while (std::getline(in, line))
  {
    line = trimWhitespace(line);
    // An empty line is a null label, don't skip it

    // Check if this is an integer, if so, use as is.
    char * next;
    long n = strtol(line.c_str(), &next, 10);
    if (*next != 0)  // could not parse to end of string, not an integer
      elem_colors.push_back(getLabelColor(line));
    else
      elem_colors.push_back(getPaletteColor(n));
  }

  if (mesh_group)
  {
    try
    {
      ModelInternal::FaceLabeler flab(elem_colors);
      mesh_group->forEachMeshUntil(&flab);
    }
    THEA_STANDARD_CATCH_BLOCKS(return has_elem_labels;, WARNING, "Couldn't load model face labels from '%s'", path_.c_str())
  }
  else
    if (!point_cloud->setPointColors(elem_colors))
      return has_elem_labels;

  has_elem_labels = true;
  wxPostEvent(this, wxCommandEvent(EVT_MODEL_NEEDS_REDRAW));

  return has_elem_labels;
}

namespace ModelInternal {

std::string
getDefaultPath(std::string model_path, std::string const & query_path, TheaArray<std::string> const & query_exts)
{
  if (FileSystem::fileExists(query_path))
    return query_path;

  int iter_begin = FileSystem::directoryExists(query_path) ? 0 : 1;

  for (int i = iter_begin; i < 2; ++i)
  {
    std::string dir = (i == 0 ? query_path : FilePath::parent(model_path));

    for (array_size_t j = 0; j < query_exts.size(); ++j)
    {
      std::string ffn = FilePath::concat(dir, model_path + query_exts[j]);
      if (FileSystem::exists(ffn))
        return ffn;
    }

    for (array_size_t j = 0; j < query_exts.size(); ++j)
    {
      std::string ffn = FilePath::concat(dir, FilePath::completeBaseName(model_path) + query_exts[j]);
      if (FileSystem::exists(ffn))
        return ffn;
    }

    for (array_size_t j = 0; j < query_exts.size(); ++j)
    {
      std::string ffn = FilePath::concat(dir, FilePath::baseName(model_path) + query_exts[j]);
      if (FileSystem::exists(ffn))
        return ffn;
    }
  }

  return "";
}

} // namespace ModelInternal

std::string
Model::getDefaultFeaturesPath() const
{
  TheaArray<std::string> exts;
  exts.push_back(".arff");
  exts.push_back(".features");

  return ModelInternal::getDefaultPath(path, app().options().features, exts);
}

std::string
Model::getDefaultElementLabelsPath() const
{
  TheaArray<std::string> exts;
  exts.push_back(".seg");

  return ModelInternal::getDefaultPath(path, app().options().elem_labels, exts);
}

void
Model::registerDisplay(ModelDisplay * display)
{
  if (!display) return;

  Bind(EVT_MODEL_GEOMETRY_CHANGED, &ModelDisplay::modelGeometryChanged, display);
  Bind(EVT_MODEL_NEEDS_REDRAW, &ModelDisplay::modelNeedsRedraw, display);
}

void
Model::deregisterDisplay(ModelDisplay * display)
{
  if (!display) return;

  Unbind(EVT_MODEL_GEOMETRY_CHANGED, &ModelDisplay::modelGeometryChanged, display);
  Unbind(EVT_MODEL_NEEDS_REDRAW, &ModelDisplay::modelNeedsRedraw, display);
}

AxisAlignedBox3 const &
Model::getBounds() const
{
  return bounds;
}

AxisAlignedBox3
Model::getTransformedBounds() const
{
  return hasTransform() ? bounds.transformAndBound(getTransform()) : bounds;
}

void
Model::updateBounds()
{
  bounds.setNull();

  if (mesh_group)
  {
    mesh_group->updateBounds();
    bounds.merge(mesh_group->getBounds());
  }

  if (point_cloud)
  {
    point_cloud->updateBounds();
    bounds.merge(point_cloud->getBounds());
  }
}

void
Model::uploadToGraphicsSystem(Graphics::RenderSystem & render_system)
{
  if (mesh_group)
    mesh_group->uploadToGraphicsSystem(render_system);

  if (point_cloud)
    point_cloud->uploadToGraphicsSystem(render_system);
}

namespace ModelInternal {

struct DrawFaceNormals
{
  DrawFaceNormals(Graphics::RenderSystem * rs, Real normal_scale_) : render_system(rs), normal_scale(normal_scale_) {}

  bool operator()(Mesh const & mesh)
  {
    render_system->beginPrimitive(Graphics::RenderSystem::Primitive::LINES);

      for (Mesh::FaceConstIterator fi = mesh.facesBegin(); fi != mesh.facesEnd(); ++fi)
      {
        if (fi->numVertices() <= 0)
          continue;

        Vector3 c(0, 0, 0);
        for (Mesh::Face::VertexConstIterator vi = fi->verticesBegin(); vi != fi->verticesEnd(); ++vi)
          c += (*vi)->getPosition();

        c /= fi->numVertices();

        render_system->sendVertex(c);
        render_system->sendVertex(c + normal_scale * fi->getNormal());
      }

    render_system->endPrimitive();

    return false;
  }

  Graphics::RenderSystem * render_system;
  Real normal_scale;
};

struct DrawVertexNormals
{
  DrawVertexNormals(Graphics::RenderSystem * rs, Real normal_scale_) : render_system(rs), normal_scale(normal_scale_) {}

  bool operator()(Mesh const & mesh)
  {
    render_system->beginPrimitive(Graphics::RenderSystem::Primitive::LINES);

      for (Mesh::VertexConstIterator vi = mesh.verticesBegin(); vi != mesh.verticesEnd(); ++vi)
      {
        render_system->sendVertex(vi->getPosition());
        render_system->sendVertex(vi->getPosition() + normal_scale * vi->getNormal());
      }

    render_system->endPrimitive();

    return false;
  }

  Graphics::RenderSystem * render_system;
  Real normal_scale;
};

} // namespace ModelInternal

void
Model::draw(Graphics::RenderSystem & render_system, Graphics::RenderOptions const & options) const
{
  if (isEmpty())
    return;

  const_cast<Model *>(this)->uploadToGraphicsSystem(render_system);

  GraphicsWidget::setLight(Vector3(-1, -1, -2), ColorRGB(1, 1, 1), ColorRGB(1, 0.8f, 0.7f));

  if (hasTransform())
  {
    render_system.setMatrixMode(Graphics::RenderSystem::MatrixMode::MODELVIEW); render_system.pushMatrix();
    render_system.multMatrix(getTransform().toHomMatrix());
  }

  render_system.pushShader();
  render_system.pushTextures();
  render_system.pushColorFlags();

    setPhongShader(render_system);
    render_system.setTexture(0, NULL);

    render_system.setColor(color);

    if (mesh_group)
    {
      Graphics::RenderOptions ro = options;  // make a copy

      if (has_elem_labels)
      {
        ro.sendColors() = true;
        ro.useVertexData() = false;
      }

      bool smooth_shading = (ro.useVertexNormals() && ro.useVertexData());

      if (smooth_shading)
        mesh_group->forEachMeshUntil(&ModelInternal::enableGPURendering);
      else
        mesh_group->forEachMeshUntil(&ModelInternal::disableGPURendering);

      mesh_group->draw(render_system, ro);

      if (app().options().show_normals)
      {
        render_system.setShader(NULL);
        render_system.setColor(ColorRGB(0, 1, 0));

        Real normal_scale = 0.025f * getBounds().getExtent().length();

        if (smooth_shading)
        {
          ModelInternal::DrawVertexNormals drawer(&render_system, normal_scale);
          mesh_group->forEachMeshUntil(&drawer);
        }
        else
        {
          ModelInternal::DrawFaceNormals drawer(&render_system, normal_scale);
          mesh_group->forEachMeshUntil(&drawer);
        }
      }
    }

  render_system.popColorFlags();
  render_system.popTextures();
  render_system.popShader();

  if (point_cloud)
    point_cloud->draw(render_system, options);

  if (hasTransform())
  {
    render_system.setMatrixMode(Graphics::RenderSystem::MatrixMode::MODELVIEW); render_system.popMatrix();
  }
}

} // namespace MatTrans
