
import numpy as np


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


def load_bbox(file_path):
    bbox = np.fromfile(file_path, dtype=int, sep=" ")
    if bbox[2] > bbox[3]:
        bbox[1] -= (bbox[2] - bbox[3]) / 2
        bbox[3] = bbox[2]
    else:
        bbox[0] -= (bbox[3] - bbox[2]) / 2
        bbox[2] = bbox[3]

    return bbox


def trafo_coords(qbbox, qsz, rbbox, rsz, rx, ry):
    '''
    :param qbbox: Query image bounding box.
    :param qsz: Query image size [width, height].
    :param rbbox: Retrieved image bounding box.
    :param rsz: Retrieved image size [width, height].
    :param rx: Retrieved image point x coordinate.
    :param ry: Retrieved image point y coordinate.
    '''
    # TODO: What is it doing exactly?
    x0, y0 = rx * rsz[0], ry * rsz[1]
    x, y = (x0 - rbbox[0]) / rbbox[2], (y0 - rbbox[1]) / rbbox[3]
    x1, y1 = x * qbbox[2] + qbbox[0], y * qbbox[3] + qbbox[1]
    rx1, ry1 = 2. * x1 / qsz[0] - 1, 2. * y1 / qsz[1] - 1

    return rx1, ry1

