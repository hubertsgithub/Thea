
class AppShape:
    def __init__(self, shape_id, model_path, shape_view_dic):
        # ID of the shape in Siddhant's database
        self.shape_id = shape_id
        # Relative path to the .obj file we can use to render this shape
        # (relative to the dataset root directory)
        self.model_path = model_path
        # Dictionary of shape views indexed by the shape view index
        self.shape_view_dic = shape_view_dic

    def get_dict(self):
        return dict(
            shape_id=self.shape_id,
            model_path=self.model_path,
            shape_view_dic={
                camera_id: sv.get_dict()
                for camera_id, sv in self.shape_view_dic.iteritems()
            },
        )

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    @staticmethod
    def create_from_json(json_data):
        shape_view_dic = {}
        for sv_id, sv_data in json_data['shape_view_dic'].iteritems():
            shape_view_dic[int(sv_id)] = ShapeView.create_from_json(sv_data)

        return AppShape(
            shape_id=json_data['shape_id'],
            model_path=json_data['model_path'],
            shape_view_dic=shape_view_dic,
        )


class ShapeView:
    def __init__(self, camera_id, camera_path,
                 rendered_view_path, bb_path, photo_retrievals):
        # ID of the camera settings which were used to render this shape view.
        self.camera_id = camera_id
        # Relative path to the file which contains the pose of the camera
        # (relative to the experiment root directory)
        self.camera_path = camera_path
        # Relative path to the rendered view image file
        # (relative to the experiment root directory)
        self.rendered_view_path = rendered_view_path
        # Relative path to the file which contains bounding box information
        # (relative to the experiment root directory)
        self.bb_path = bb_path
        # Retrieved photos sorted by feature distance in ascending order (so
        # best/closest retrieval first)
        self.photo_retrievals = photo_retrievals

    def get_dict(self):
        return dict(
            camera_id=self.camera_id,
            camera_path=self.camera_path,
            rendered_view_path=self.rendered_view_path,
            bb_path=self.bb_path,
            photo_retrievals=[pr.get_dict() for pr in self.photo_retrievals],
        )

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    @staticmethod
    def create_from_json(json_data):
        photo_retrievals = [
            PhotoRetrieval.create_from_json(pr_data)
            for pr_data in json_data['photo_retrievals']
        ]

        return ShapeView(
            camera_id=json_data['camera_id'],
            camera_path=json_data['camera_path'],
            rendered_view_path=json_data['rendered_view_path'],
            bb_path=json_data['bb_path'],
            photo_retrievals=photo_retrievals,
        )


class PhotoRetrieval:
    def __init__(self, photo_id, photo_path, bb_path, feature_dist):
        # ID of the retrieved photo in the Mattrans database
        self.photo_id = photo_id
        # Relative path to the image file (relative to the dataset root directory)
        self.photo_path = photo_path
        # Relative path to the file which contains bounding box information
        # (relative to the experiment root directory)
        self.bb_path = bb_path
        # HoG distance between the shape view's HoG features and the photo's
        # HoG features
        self.feature_dist = feature_dist

    def get_dict(self):
        return dict(
            photo_id=self.photo_id,
            photo_path=self.photo_path,
            bb_path=self.bb_path,
            feature_dist=self.feature_dist,
        )

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    @staticmethod
    def create_from_json(json_data):
        return PhotoRetrieval(
            photo_id=json_data['photo_id'],
            photo_path=json_data['photo_path'],
            bb_path=json_data['bb_path'],
            feature_dist=json_data['feature_dist'],
        )

