
import numpy as np
import h5py


class PythonApi:
    def __init__(self):
        self.image_dir_path = None
        self.retrieved_img_idxs = None

    def load_resources(self, image_dir_path, retrieved_images_path):
        self.image_dir_path = image_dir_path

        #with h5py.File(args.experimentDir + args.retrieval,'r') as hf:
            #result = np.array(hf.get('result'))
             #For now, we expect retrievals for only one query
            #if result.shape[0] != 0:
                #raise ValueError('Retrieval results should contain only')
            #self.retrieved_img_idxs = None
        print 'image_dir_path:', image_dir_path
        print 'retrieved_images_path:', retrieved_images_path

    def get_cameras(self):
        return [{'camera_id': 0, 'camera_path': 'testpath/test'}]

    def retrieve_images(self, clicked_points):
        print clicked_points
        return ['hahha.png']

