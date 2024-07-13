import sys

import cv2 as cv
import numpy as np


img = cv.imread(sys.argv[1])
# img = cv.imread("sample/img_1.png")
img = cv.resize(img, (1000, 1000))

def convert_image_to_square(arr):
    sz = max(abs(arr[1][0] - arr[0][0]), abs(arr[2][1] - arr[1][1]), abs(arr[3][0] - arr[2][0]), abs(arr[3][1] - arr[0][1]))
    arr = np.array(arr, dtype=np.float32)
    mat = cv.getPerspectiveTransform(arr,
                                     np.array([
                                         (0, 0),
                                         (sz - 1, 0),
                                         (sz - 1, sz - 1),
                                         (0, sz - 1)
                                     ], dtype=np.float32))
    res = cv.warpPerspective(img, mat, [sz, sz])
    return res

arr = []
def on_click(event, x, y, flag, param):
    if event == cv.EVENT_LBUTTONDOWN:
        arr.append((x, y))
        if len(arr) == 4:
            res = convert_image_to_square(arr)
            cv.imshow("res", res)

cv.imshow("img", img)
cv.setMouseCallback("img", on_click)

cv.waitKey(0)
