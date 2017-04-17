import os
import json
import numpy as np
import app_models


def build_idx_dic(items, show_progress=False):
    #if show_progress:
        #items = progress_bar(items)

    return {
        item: i
        for i, item in enumerate(items)
    }


# http://stackoverflow.com/questions/12729228/simple-efficient-bilinear-interpolation-of-images-in-numpy-and-python
def bilinear_interpolate(im, x, y):
    x = np.asarray(x)
    y = np.asarray(y)
    assert x.shape == y.shape

    x0 = np.floor(x).astype(int)
    x1 = x0 + 1
    y0 = np.floor(y).astype(int)
    y1 = y0 + 1

    x0 = np.clip(x0, 0, im.shape[1] - 1)
    x1 = np.clip(x1, 0, im.shape[1] - 1)
    y0 = np.clip(y0, 0, im.shape[0] - 1)
    y1 = np.clip(y1, 0, im.shape[0] - 1)

    Ia = im[y0, x0, ...]
    Ib = im[y1, x0, ...]
    Ic = im[y0, x1, ...]
    Id = im[y1, x1, ...]

    wa = (x1-x) * (y1-y)
    wb = (x1-x) * (y-y0)
    wc = (x-x0) * (y1-y)
    wd = (x-x0) * (y-y0)

    return wa*Ia + wb*Ib + wc*Ic + wd*Id


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

