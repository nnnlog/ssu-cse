import cv2
import imutils
import numpy as np


def detect_empire_state_building(image_path, template_path):
    # Read the image and the template
    img = cv2.imread(image_path)
    template = cv2.imread(template_path, 0)  # Convert to grayscale

    # Initialize the ORB detector
    orb = cv2.ORB.create()

    # Find the key points and descriptors with ORB
    kp1, des1 = orb.detectAndCompute(template, None)
    kp2, des2 = orb.detectAndCompute(img, None)

    # Create BFMatcher (Brute Force Matcher)
    bf = cv2.BFMatcher(cv2.NORM_HAMMING, crossCheck=True)

    # Match descriptors
    matches = bf.match(des1, des2)

    # Sort them in ascending order of distance
    matches = sorted(matches, key=lambda x: x.distance)

    print(len(matches))

    # Draw the first 10 matches
    img_matches = cv2.drawMatches(template, kp1, img, kp2, matches[:10000], None, flags=cv2.DrawMatchesFlags_NOT_DRAW_SINGLE_POINTS)


    img_matches = imutils.resize(img_matches, height=920)

    cv2.imshow("Matches", img_matches)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

def compare_images(img1, img2):
    # Load images
    image1 = cv2.imread(img1)
    image2 = cv2.imread(img2)

    # Create ORB detector
    orb = cv2.ORB.create()

    # Find the keypoints and descriptors with ORB
    kp1, des1 = orb.detectAndCompute(image1, None)
    kp2, des2 = orb.detectAndCompute(image2, None)

    des1 = np.float32(des1)
    des2 = np.float32(des2)

    # FLANN parameters
    FLANN_INDEX_KDTREE = 1
    index_params = dict(algorithm=FLANN_INDEX_KDTREE, trees=5)
    search_params = dict(checks=50)  # or pass an empty dictionary

    # FLANN matcher
    flann = cv2.FlannBasedMatcher(index_params, search_params)

    # Match descriptors using K-Nearest Neighbors
    matches = flann.knnMatch(des1, des2, k=2)

    # Need to draw only good matches, so create a mask
    matchesMask = [[0, 0] for _ in range(len(matches))]

    # Ratio test as per Lowe's paper
    for i, (m, n) in enumerate(matches):
        print(m.distance, n.distance)
        if m.distance < 0.9 * n.distance:
            matchesMask[i] = [1, 0]

    # Use findHomography to find a perspective transformation
    src_pts = np.float32([kp1[m[0].queryIdx].pt for i, m in enumerate(matches) if matchesMask[i][0] == 1]).reshape(-1, 1, 2)
    dst_pts = np.float32([kp2[m[0].trainIdx].pt for i, m in enumerate(matches) if matchesMask[i][0] == 1]).reshape(-1, 1, 2)

    M, mask = cv2.findHomography(src_pts, dst_pts, cv2.RANSAC, 5.0)

    # Apply perspective transformation to the corners of the image
    h, w, _ = image1.shape
    corners = np.float32([[0, 0], [0, h - 1], [w - 1, h - 1], [w - 1, 0]]).reshape(-1, 1, 2)
    transformed_corners = cv2.perspectiveTransform(corners, M)

    # Draw the transformed rectangle on the second image
    image_matches = cv2.polylines(image2.copy(), [np.int32(transformed_corners)], True, (0, 255, 0), 3, cv2.LINE_AA)

    # Draw matches on the images
    draw_params = dict(matchColor=(0, 255, 0), singlePointColor=(255, 0, 0), matchesMask=matchesMask, flags=0)
    img_matches = cv2.drawMatchesKnn(image1, kp1, image_matches, kp2, matches, None, **draw_params)

    img_matches = imutils.resize(img_matches, height=920)

    # Display the result
    cv2.imshow('Image Matches', img_matches)
    cv2.waitKey(0)
    cv2.destroyAllWindows()


# Example usage
# detect_empire_state_building("etc_img/realistic/2.jpg", "etc_img/virtual/2.jpg")
compare_images("etc_img/realistic/2.jpg", "etc_img/realistic_extract/2.jpg")
