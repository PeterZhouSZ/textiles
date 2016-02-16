__author__ = "def"

import cv2
import numpy as np

from GarmentSegmentation import GarmentSegmentation
from GarmentDepthMapClustering import GarmentDepthMapClustering
from GarmentPickAndPlacePoints import GarmentPickAndPlacePoints
import GarmentPlot
from utils import load_data

if __name__ == "__main__":

    image_paths, depth_image_paths = load_data('../data/20150902')

    for path_rgb_image, path_depth_image in zip(image_paths, depth_image_paths):
        # Load input data
        image_src = cv2.imread(path_rgb_image)
        depth_image = np.loadtxt(path_depth_image)

        # Garment Segmentation Step
        mask = GarmentSegmentation.background_substraction(image_src)
        approximated_polygon = GarmentSegmentation.compute_approximated_polygon(mask)
        GarmentPlot.plot_mask(mask)
        GarmentPlot.plot_contour(image_src, approximated_polygon)

        # Garment Depth Map Clustering
        preprocessed_depth_image = GarmentDepthMapClustering.preprocess(depth_image, mask)
        labeled_image = GarmentDepthMapClustering.cluster_similar_regions(preprocessed_depth_image)
        GarmentPlot.plot_depth(preprocessed_depth_image)

        # Garment Pick and Place Points
        unfold_paths = GarmentPickAndPlacePoints.calculate_unfold_paths(labeled_image, approximated_polygon)
        bumpiness = GarmentPickAndPlacePoints.calculate_bumpiness(labeled_image, unfold_paths)
        pick_point, place_point = GarmentPickAndPlacePoints.calculate_pick_and_place_points(unfold_paths, bumpiness)


