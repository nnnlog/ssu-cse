import random
import sys

import cv2 as cv
import numpy as np
from math import *
import matplotlib.pyplot as plt
from functools import cmp_to_key


def get_dist(a, b, c, d):
    return sqrt(pow(a - c, 2) + pow(b - d, 2))


def convert_image_to_square(img, arr):
    sz = int(max(get_dist(*arr[1], *arr[0]), get_dist(*arr[2], *arr[1]), get_dist(*arr[3], *arr[2]),
                 get_dist(*arr[3], *arr[0])))
    arr = np.array(arr, dtype=np.float32)
    mat = cv.getPerspectiveTransform(arr,
                                     np.array([
                                         (0, 0),
                                         (sz - 1, 0),
                                         (sz - 1, sz - 1),
                                         (0, sz - 1)
                                     ], dtype=np.float32))
    res = cv.warpPerspective(img, mat, [sz, sz])
    return res, mat


img = cv.imread("sample/img_15.png")
# img = cv.imread(sys.argv[1])
origin_shape = img.shape
origin_sz = origin_shape[0] * origin_shape[1]
img = cv.resize(img, (1000, 1000))

blur = cv.GaussianBlur(img, (0, 0), 3)
modified_img = cv.addWeighted(img, 1.5, blur, -0.5, 0)

gray = cv.cvtColor(modified_img, cv.COLOR_BGR2GRAY)
gray = cv.Canny(gray, 20, 100)

# tmp = np.zeros(shape=img.shape, dtype=np.uint8)
# cv.circle(tmp, center, radius, (255, 255, 255), cv.FILLED)
# if np.count_nonzero(cv.bitwise_and(curr, tmp)) <= curr.shape[0] * curr.shape[1] // 3: continue
# cv.circle(mask, center, radius, (255, 255, 255), cv.FILLED)
# ok = 1
# tmp = np.copy(curr)
# cv.circle(tmp, center, radius, (255, 0, 255))

# tmp = cv.adaptiveThreshold(gray, 255, cv.ADAPTIVE_THRESH_GAUSSIAN_C, cv.THRESH_BINARY, 7, 2)


cv.imshow("img", gray)
cv.waitKey(0)
# cv.imshow("img", tmp)
# cv.waitKey(0)

colors = []

rows = gray.shape[0]
circles = cv.HoughCircles(gray, cv.HOUGH_GRADIENT, 1, rows / 8,
                          param1=60, param2=20,
                          minRadius=10, maxRadius=50)
if circles is not None:
    mask = np.zeros(shape=img.shape, dtype=np.uint8)
    circles = np.uint16(np.around(circles))
    ok = 0
    for i in circles[0, :]:
        center = (i[0], i[1])
        radius = i[2]
        # tmp = np.zeros(shape=gray.shape, dtype=np.uint8)
        # cv.circle(tmp, center, radius, (255, 255, 255), cv.FILLED)
        # if np.count_nonzero(cv.bitwise_and(img, tmp)) <= img.shape[0] * img.shape[1] // 8: continue
        cv.circle(mask, center, radius, (255, 255, 255), cv.FILLED)
        ok = 1
        # tmp = np.copy(img)
        cv.circle(img, center, radius, (255, 0, 255), 2)

        # cv.imshow("img", tmp)
        # cv.waitKey(0)
        hsl = cv.cvtColor(img, cv.COLOR_BGR2HLS)
        Lchanneld = cv.bitwise_and(hsl, mask)[:, :, 1]
        lvalueld = cv.mean(Lchanneld)[0]
        colors.append(lvalueld)
        # curr_gray = cv.cvtColor(img, cv.COLOR_BGR2GRAY)
        # colors.append(np.median(curr_gray[np.nonzero(cv.bitwise_and(curr_gray, mask))]))
    # cv.imshow("img", curr)
    # cv.waitKey(0)

print(colors)
if len(colors) == 0:
    print(f"Bright : 0, Dark : 0")
    exit(0)

colors = np.array(colors, dtype=np.float32)
labels, centers = cv.kmeans(colors, 2, None, (cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_MAX_ITER, 10, 1.0), 10,
                            cv.KMEANS_RANDOM_CENTERS)[1:]
labels = labels.reshape(-1)  # transpose to row vec
groups = [[], []]
for i, color in enumerate(colors):
    groups[labels[i]].append(color)
if np.median(groups[0]) > np.median(groups[1]): groups = list(reversed(groups))

# groups = [[], []]
# for i, color in enumerate(colors):
#     groups[0 if np.mean(color) < 127 else 1].append(color)


print(f"Bright : {len(groups[0])}, Dark : {len(groups[1])}")
# print(groups)

cv.imshow("img", img)
cv.waitKey(0)
# cv.imshow("img", dxdy)
# cv.waitKey(0)
# cv.imshow("img", res_edge)
# cv.waitKey(0)
cv.destroyAllWindows()
