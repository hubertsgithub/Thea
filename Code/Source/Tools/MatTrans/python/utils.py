
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
    '''
    Loads the bounding box as [left upper pixel x, left upper pixel y, width, height].
    '''
    return np.fromfile(file_path, dtype=int, sep=" ")


def trafo_coords(qbbox, qsz, rbbox, rsz, qx, qy):
    '''
    Compute coordinate of the query point in the retrieval image.
    :param qbbox: Query image bounding box [left upper pixel x, left upper pixel y, width, height].
    :param qsz: Query image size [width, height].
    :param rbbox: Retrieved image bounding box [left upper pixel x, left upper pixel y, width, height].
    :param rsz: Retrieved image size [width, height].
    :param qx: Query image point x coordinate in [0, 1].
    :param qy: Query image point y coordinate in [0, 1].
    '''
    # Get pixel coordinates in the query image
    qx_p, qy_p = qx * qsz[0], qy * qsz[1]
    # Get relative coordinates to the bounding box in the query image
    qx_bb, qy_bb = (qx_p - qbbox[0]) / qbbox[2], (qy_p - qbbox[1]) / qbbox[3]
    # Get pixel coordinates in the retrieved image
    rx_p, ry_p = qx_bb * rbbox[2] + rbbox[0], qy_bb * rbbox[3] + rbbox[1]
    # Get normalized coordinates in the retrieved image
    return rx_p / rsz[0], ry_p / rsz[1]

