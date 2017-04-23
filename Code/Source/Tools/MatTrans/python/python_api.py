import json
import os

import numpy as np

import app_models
from utils import bilinear_interpolate, build_idx_dic, load_bbox, trafo_coords


class PythonApi:
    def __init__(self):
        self.dataset_dir = None
        self.experiment_dir = None
        self.shape = None
        self.photo_id_to_idx = None
        self.features_2D = None
        self.verbose = True

    def load_resources(self, dataset_dir, experiment_dir, shape_data_path):
        self.dataset_dir = dataset_dir
        self.experiment_dir = experiment_dir
        json_data = json.load(open(shape_data_path))
        self.shape = app_models.AppShape.create_from_json(json_data)

        # Load 2D features for all photos
        features_2D_data = dict(np.load(os.path.join(experiment_dir, 'img_2D_fets.npz')))
        self.photo_id_to_idx = build_idx_dic(features_2D_data['ids'])
        self.features_2D = features_2D_data['features']

    def get_cameras(self):
        camera_dic = {}
        for camera_id, shape_view in self.shape.shape_view_dic.iteritems():
            # Don't add cameras twice
            if camera_id in camera_dic:
                continue

            camera_dic[camera_id] = dict(
                camera_id=camera_id,
                camera_path=os.path.join(
                    self.experiment_dir,
                    shape_view.camera_path.encode('utf-8'),
                )
            )

        return camera_dic.values()

    def retrieve_images(self, clicked_points, feat_3D):
        photo_paths = []
        feat_2D_arr = []
        for clicked_point in clicked_points:
            cid = clicked_point['camera_id']
            if cid not in self.shape.shape_view_dic:
                raise ValueError('The camera ID (id: %s) corresponding to the \
                    clicked point has to be among the shape views!' % cid)

            # Get all retrieved photos corresponding to this shape view
            sv = self.shape.shape_view_dic[cid]
            qbbox = load_bbox(os.path.join(self.experiment_dir, sv.bb_path))
            if self.verbose:
                print 'qbbox: %s' % qbbox

            # Transform coordinates from [-1, 1] to [0, 1]
            qx = (clicked_point['pt_2D']['x'] + 1) / 2
            # Negative because y increases upwards in Thea's coordinate frame,
            # but downwards in the image coordinate frame
            qy = (-clicked_point['pt_2D']['y'] + 1) / 2

            for pr in sv.photo_retrievals:
                # TODO: This is incorrect, probably we want to transform the
                # other way
                # Align query and retrieved image based on bounding boxes
                rbbox = load_bbox(os.path.join(self.experiment_dir, pr.bb_path))
                if self.verbose:
                    print 'rbbox: %s' % rbbox

                rx, ry = trafo_coords(
                    qbbox=qbbox, qsz=sv.img_size, rbbox=rbbox, rsz=pr.img_size,
                    qx=qx, qy=qy)
                if self.verbose:
                    print 'qx: %s, qy: %s, rx: %s, ry: %s' % (qx, qy, rx, ry)

                # Get 2D feature for the whole photo
                feat_2D = self.features_2D[self.photo_id_to_idx[pr.photo_id]]

                # Interpolate feature map
                point_feat_2D = bilinear_interpolate(
                    im=feat_2D,
                    x=rx * feat_2D.shape[1],
                    y=ry * feat_2D.shape[0],
                )
                photo_paths.append(os.path.join(
                    self.dataset_dir, pr.photo_path.encode('utf-8')))
                feat_2D_arr.append(point_feat_2D)

        # Compute distances
        dists = np.linalg.norm(
            np.array(feat_2D_arr) - feat_3D[np.newaxis, :], axis=1
        )
        # Sort path by closest distance
        photo_paths = [photo_paths[idx] for idx in np.argsort(dists)]

        return photo_paths[:10]
