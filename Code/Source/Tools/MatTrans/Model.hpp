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

#ifndef __MatTrans_Model_hpp__
#define __MatTrans_Model_hpp__

#include "Common.hpp"
#include "GraphicsWidget.hpp"
#include "MeshFwd.hpp"
#include "PythonApi.hpp"
#include "../../Algorithms/KDTreeN.hpp"
#include "../../Algorithms/MeshKDTree.hpp"
#include "../../Algorithms/PointTraitsN.hpp"
#include "../../Algorithms/RayQueryStructureN.hpp"
#include "../../AffineTransform3.hpp"
#include "../../Transformable.hpp"
#include <wx/event.h>

class wxMouseEvent;

namespace MatTrans {

class ModelDisplay;
class PointCloud;
typedef shared_ptr<PointCloud> PointCloudPtr;

} // namespace MatTrans

wxDECLARE_EVENT(EVT_MODEL_PATH_CHANGED,         wxCommandEvent);
wxDECLARE_EVENT(EVT_MODEL_GEOMETRY_CHANGED,     wxCommandEvent);
wxDECLARE_EVENT(EVT_MODEL_NEEDS_REDRAW,         wxCommandEvent);

namespace MatTrans {

/** The model manipulated by the user. */
class Model : public GraphicsWidget, public Transformable<AffineTransform3>, public wxEvtHandler
{
  private:
    typedef Transformable<AffineTransform3> TransformableBaseT;
    typedef Algorithms::KDTreeN<Vector3, 3> PointKDTree;

  public:
    typedef Thea::Algorithms::MeshKDTree<Mesh> KDTree;  ///< A kd-tree on mesh triangles.
    typedef Algorithms::RayStructureIntersection3 RayStructureIntersection3;  /**< Intersection of a ray with an acceleration
                                                                                   structure. */
    typedef Thea::Algorithms::KDTreeN<MeshVertex const *, 3> VertexKDTree;  ///< A kd-tree on mesh vertices.

    //========================================================================================================================
    // Basic functions
    //========================================================================================================================

    /** Constructor. */
    Model(std::string const & initial_mesh = "");

    /** Destructor. */
    ~Model();

    /** Get the name of the model. */
    std::string getName() const;

    /** Get the path of the currently loaded model. */
    std::string const & getPath() const { return path; }

    /** Is the model empty? */
    bool isEmpty() const;

    /** Clear the model and invalidate all associated structures. */
    void clear();

    /** Invalidate all associated structures. */
    void invalidateAll();

    void setTransform(AffineTransform3 const & trans_);

    void clearTransform();

    //========================================================================================================================
    // KD-trees on mesh triangles and vertices
    //========================================================================================================================

    /** Invalidate the kd-tree of the model. The kd-tree will be lazily recomputed. */
    void invalidateKDTree();

    /** Invalidate the kd-tree on the vertices of the model. The kd-tree will be lazily recomputed. */
    void invalidateVertexKDTree();

    /** Compute the kd-tree of the model, if the current kd-tree is invalid. */
    void updateKDTree() const;

    /** Compute the kd-tree on the vertices of the model, if the current kd-tree is invalid. */
    void updateVertexKDTree() const;

    /** Get the kd-tree for the model. By default, it will be recomputed if it is currently invalid. */
    KDTree const & getKDTree(bool recompute_if_invalid = true) const;

    /** Get the kd-tree on the vertices of the model. By default, it will be recomputed if it is currently invalid. */
    VertexKDTree const & getVertexKDTree(bool recompute_if_invalid = true) const;

    //========================================================================================================================
    // Ray-shooting and nearest neighbor queries
    //========================================================================================================================

    /** Check if a ray intersects the model, optionally restricting the search to a maximum hit time. */
    bool rayIntersects(Ray3 const & ray, Real max_time = -1) const;

    /**
     * Compute the time after which a ray will intersect the model, optionally restricting the search to a maximum hit time. If
     * the ray does not intersect the model, a negative value is returned.
     */
    Real rayIntersectionTime(Ray3 const & ray, Real max_time = -1) const;

    /**
     * Compute the intersection of a ray with the model, optionally restricting the search to a maximum hit time. If the ray
     * does not intersect the model, an invalid intersection is returned.
     */
    RayStructureIntersection3 rayIntersection(Ray3 const & ray, Real max_time = -1) const;

    /**
     * Get the point on the model closest to a query point.
     *
     * @param query The query point.
     * @param distance_bound Maximum allowed distance to closest point. Pass a negative value to disable this limit.
     * @param min_dist Used to return distance to closest point.
     * @param closest_pt Used to return the closest point itself.
     * @param closest_pt_normal Used to return the surface normal at the closest point.
     * @param accelerate_with_vertices If true, the function first computes an approximate nearest neighbor using the set of
     *   mesh vertices, to make the exact computation faster. This parameter should normally always be true.
     *
     * @return The index of the kd-tree triangle containing the closest point, if one is found within the distance bound, else
     *   a negative number.
     */
    long closestPoint(Vector3 const & query, Real distance_bound = -1, Real * min_dist = NULL, Vector3 * closest_pt = NULL,
                      Vector3 * closest_pt_normal = NULL, bool accelerate_with_vertices = true) const;

    /**
     * Select the nearest point on the model along a ray.
     *
     * @return The distance, in multiples of ray length, to the picked point.
     */
    Real pick(Ray3 const & ray);

    /** Invalidate the last picked point. */
    void invalidatePick();

    /** Process the picked point. */
    void processPick();

    //========================================================================================================================
    // Interaction
    //========================================================================================================================

    /** [wxWidgets] Called when a mouse button is pressed. */
    void mousePressEvent(wxMouseEvent & event);

    /** [wxWidgets] Called when the mouse is moved. */
    void mouseMoveEvent(wxMouseEvent & event);

    /** [wxWidgets] Called when a mouse button is released. */
    void mouseReleaseEvent(wxMouseEvent & event);

    //========================================================================================================================
    // Features
    //========================================================================================================================

    /** Load features from a file. */
    bool loadFeatures(std::string const & path_);

    /** Get the path of the currently loaded features. */
    std::string const & getFeaturesPath() const { return features_path; }

    /** Check if the model has currently loaded features. */
    bool hasFeatures() const { return has_features; }

    //========================================================================================================================
    // Mattrans shape data
    //========================================================================================================================

    /** Load data associated with the current shape. E.g. retrievals, 2D features, etc. */
    bool loadShapeData(std::string const & dataset_dir_,
        std::string const & experiment_dir_,
        std::string const & shape_data_path_);

    //========================================================================================================================
    // Face labels
    //========================================================================================================================

    /** Load face labels from a file. */
    bool loadElementLabels(std::string const & path_);

    /** Get the path of the currently loaded face labels. */
    std::string const & getElementLabelsPath() const { return elem_labels_path; }

    /** Check if the model has currently loaded face labels. */
    bool hasElementLabels() const { return has_elem_labels; }

    //========================================================================================================================
    // Bounding boxes
    //========================================================================================================================

    /** Get the bounding box in object coordinates. */
    AxisAlignedBox3 const & getBounds() const;

    /** Get the bounding box in world coordinates. */
    AxisAlignedBox3 getTransformedBounds() const;

    void updateBounds();

    //========================================================================================================================
    // Display
    //========================================================================================================================

    /** Register a display window to receive events from this model. */
    void registerDisplay(ModelDisplay * display);

    /** Deregister a display window from receiving events from this model. */
    void deregisterDisplay(ModelDisplay * display);

    /** Get the default color of the model. */
    ColorRGBA const & getColor() const { return color; }

    /** Set the default color of the model. */
    void setColor(ColorRGBA const & color_) { color = color_; }

    void uploadToGraphicsSystem(Graphics::RenderSystem & render_system);

    void draw(Graphics::RenderSystem & render_system,
              Graphics::RenderOptions const & options = Graphics::RenderOptions::defaults()) const;

    //========================================================================================================================
    // GUI callbacks
    //========================================================================================================================

    /**
     * Load the model from a disk file.
     *
     * @return True if the model was successfully loaded, else false.
     */
    bool load(std::string path_);

    /**
     * Select a file via a file dialog and load the model from it.
     *
     * @return True if the model was successfully loaded, else false.
     */
    bool selectAndLoad();

  private:
    /** Clear the model mesh. */
    void clearMesh();

    /** Clear the point cloud. */
    void clearPoints();

    /** Get the default path to the file in which features are stored. */
    std::string getDefaultFeaturesPath() const;

    /** Get the default path to the file in which the face labels are stored. */
    std::string getDefaultElementLabelsPath() const;

    MeshGroupPtr mesh_group;
    PointCloudPtr point_cloud;
    std::string path;

    std::string features_path;
    bool has_features;
    PointKDTree * feat_pts_kdtree;
    TheaArray< TheaArray<Real> > features;

    shared_ptr<PA::PythonApi> python_api;

    std::string elem_labels_path;
    bool has_elem_labels;

    ColorRGBA color;
    AxisAlignedBox3 bounds;

    bool valid_pick;
    long picked_feat_pt_index;
    Vector3 picked_feat_pt_position;

    mutable bool valid_kdtree;
    mutable KDTree * kdtree;

    mutable bool valid_vertex_kdtree;
    mutable VertexKDTree * vertex_kdtree;

}; // class Model

} // namespace MatTrans

#endif
