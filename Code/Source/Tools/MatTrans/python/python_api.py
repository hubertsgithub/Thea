import json
import os

import numpy as np

import app_models
from utils import bilinear_interpolate, build_idx_dic


class PythonApi:
    def __init__(self):
        self.dataset_dir = None
        self.experiment_dir = None
        self.shape = None
        self.photo_id_to_idx = None
        self.features_2D = None

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
                camera_path=shape_view.camera_path.encode('utf-8'),
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
            for pr in sv.photo_retrievals:
                # TODO: Figure out what coordinate system the 2D point is in,
                # now we assume both coordinates are between [0, 1]
                x = clicked_point['pt_2D']['x']
                y = clicked_point['pt_2D']['y']

                # Get 2D feature for the whole photo
                feat_2D = self.features_2D[self.photo_id_to_idx[pr.photo_id]]

                # Interpolate feature map
                point_feat_2D = bilinear_interpolate(
                    im=feat_2D,
                    x=x * feat_2D.shape[1],
                    y=y * feat_2D.shape[0],
                )
                photo_paths.append(pr.photo_path.encode('utf-8'))
                feat_2D_arr.append(point_feat_2D)

        # Compute distances
        dists = np.linalg.norm(
            np.array(feat_2D_arr) - feat_3D[np.newaxis, :], axis=1
        )
        # Sort path by closest distance
        photo_paths = [photo_paths[idx] for idx in np.argsort(dists)]

        return photo_paths[:10]
