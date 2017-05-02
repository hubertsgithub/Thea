import json
import os
import shutil

import numpy as np

import app_models
from utils import (bilinear_interpolate, build_idx_dic, ensuredir, load_bbox,
                   trafo_coords)


class PythonApi:

    def __init__(self):
        self.dataset_dir = None
        self.experiment_dir = None
        self.shape = None
        self.photo_id_to_idx = None
        self.features_2D = None
        self.verbose = True

        os.environ.setdefault("DJANGO_SETTINGS_MODULE", "settings")

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

    def retrieve_images(self, clicked_points, feat_3D, feat_idx, feat_count, do_visualize):
        photo_data = []
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
                print 'Loaded bounding box for query shape view from "%s"' % os.path.join(self.experiment_dir, sv.bb_path)
                print 'qbbox: %s' % qbbox

            # Transform coordinates from [-1, 1] to [0, 1]
            qx = (clicked_point['pt_2D']['x'] + 1) / 2
            # Negative because y increases upwards in Thea's coordinate frame,
            # but downwards in the image coordinate frame
            qy = (-clicked_point['pt_2D']['y'] + 1) / 2

            for pr in sv.photo_retrievals:
                # Align query and retrieved image based on bounding boxes
                rbbox = load_bbox(os.path.join(self.experiment_dir, pr.bb_path))
                if self.verbose:
                    print 'Loaded bounding box for retrieved image from "%s"' % os.path.join(self.experiment_dir, pr.bb_path)
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
                photo_data.append(dict(
                    qx=qx, qy=qy,
                    shape_view_path=os.path.join(
                        self.experiment_dir, sv.rendered_view_path.encode('utf-8')),
                    q_img_size=sv.img_size,
                    q_aspect_ratio=float(sv.img_size[0])/sv.img_size[1],
                    rx=rx, ry=ry,
                    photo_path=os.path.join(
                        self.dataset_dir, pr.photo_path.encode('utf-8')),
                    r_img_size=pr.img_size,
                    r_aspect_ratio=float(pr.img_size[0])/pr.img_size[1],
                ))
                feat_2D_arr.append(point_feat_2D)

        if clicked_points:
            # Compute distances
            dists = np.linalg.norm(
                np.array(feat_2D_arr) - feat_3D[np.newaxis, :], axis=1
            )
            # Sort path by closest distance
            photo_data_sorted = []
            for idx in np.argsort(dists):
                pd = dict(photo_data[idx])
                pd['fet_dist'] = dists[idx]
                photo_data_sorted.append(pd)

            ret = photo_data_sorted[:10]
        else:
            ret = []

        if do_visualize:
            self.visualize_retrievals(feat_idx, feat_count, ret)

        return ret

    def _render_webpage(self, webpage_context):
        # Running django's template engine...
        from django.template import Context
        from django.template.loader import get_template
        t = get_template('python/shape_retrieval_results.html')

        c = Context(webpage_context)
        webpage_raw = t.render(c)
        return webpage_raw

    def get_html_fn(self, feat_idx):
        return 'shape_id_%s-feat_idx-%s.html' % (self.shape.shape_id, feat_idx)

    def visualize_retrievals(self, feat_idx, feat_count, photo_data):
        ''' Generates a HTML page to show the results. '''
        results_dir = 'results'
        shape_dir = os.path.join(results_dir, str(self.shape.shape_id))
        ensuredir(shape_dir)

        webpage_context = {}
        webpage_context['STATIC_URL'] = ''
        webpage_context['feat_idx'] = feat_idx
        if feat_idx > 0:
            webpage_context['prev_page_url'] = self.get_html_fn(feat_idx - 1)
        else:
            webpage_context['prev_page_url'] = None
        if feat_idx < feat_count - 1:
            webpage_context['next_page_url'] = self.get_html_fn(feat_idx + 1)
        else:
            webpage_context['next_page_url'] = None

        webpage_context['photo_data'] = []
        for photo_dic in photo_data:
            # Copy photo files
            shape_view_fn = os.path.basename(photo_dic['shape_view_path'])
            shape_view_target_path = os.path.join(shape_dir, shape_view_fn)
            if not os.path.exists(shape_view_target_path):
                shutil.copyfile(photo_dic['shape_view_path'], shape_view_target_path)
            photo_fn = os.path.basename(photo_dic['photo_path'])
            photo_target_path = os.path.join(shape_dir, photo_fn)
            if not os.path.exists(photo_target_path):
                shutil.copyfile(photo_dic['photo_path'], photo_target_path)

            webpage_photo_dic = {}
            webpage_photo_dic['fet_dist'] = photo_dic['fet_dist']
            webpage_photo_dic['shape_view'] = dict(
                image_path=shape_view_fn,
                px=photo_dic['qx'],
                px_aspect=photo_dic['qx'] * photo_dic['q_aspect_ratio'],
                py=photo_dic['qy'],
                aspect_ratio=photo_dic['q_aspect_ratio'],
            )
            webpage_photo_dic['retrieved_photo'] = dict(
                image_path=photo_fn,
                px=photo_dic['rx'],
                px_aspect=photo_dic['rx'] * photo_dic['r_aspect_ratio'],
                py=photo_dic['ry'],
                aspect_ratio=photo_dic['r_aspect_ratio'],
            )
            webpage_context['photo_data'].append(webpage_photo_dic)

        # Generate and save HTML page for the retrieval results
        webpage_raw = self._render_webpage(webpage_context)
        with open(os.path.join(shape_dir, self.get_html_fn(feat_idx)), 'w') as fout:
            fout.write(webpage_raw)
