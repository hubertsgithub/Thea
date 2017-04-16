
import numpy as np
import h5py
import app_models


class PythonApi:
    def __init__(self):
        self.dataset_dir = None
        self.experiment_dir = None
        self.shape_data_path = None
        self.shape = None

    def load_resources(self, dataset_dir, experiment_dir, shape_data_path):
        self.dataset_dir = dataset_dir
        self.experiment_dir = experiment_dir
        self.shape_data_path = shape_data_path

        #with h5py.File(args.experimentDir + args.retrieval,'r') as hf:
            #result = np.array(hf.get('result'))
             #For now, we expect retrievals for only one query
            #if result.shape[0] != 0:
                #raise ValueError('Retrieval results should contain only')
            #self.retrieved_img_idxs = None

    def get_cameras(self):
        return [{'camera_id': 0, 'camera_path': 'testpath/test'}]

    def retrieve_images(self, clicked_points):
        print clicked_points
        return ['hahha.png']

