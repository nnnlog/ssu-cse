import cv2
import cv2 as cv
import imutils
import numpy as np

orb = cv.ORB.create()

src = cv.imread("etc_img/realistic_extract/0.png")
keypoints1, desc1 = orb.detectAndCompute(src, None)
# keypoints1 = orb.detect(src)
# keypoints1, desc1 = orb.compute(src, keypoints1)

# dst = cv.drawKeypoints(src, keypoints, None, (-1, -1, -1), cv.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS)
# cv.imshow('img', dst)
# cv.waitKey()
# cv.destroyAllWindows()


background = cv.imread("etc_img/realistic/0.jpg")
keypoints2, desc2 = orb.detectAndCompute(background, None)
# keypoints2 = orb.detect(background)
# keypoints2, desc2 = orb.compute(background, keypoints2)

# index_params = dict(algorithm=0, trees=5)
# search_params = dict(checks=50)
# matcher = cv2.FlannBasedMatcher(index_params, search_params)
# matches = matcher.knnMatch(desc1, desc2, k=2)
matcher = cv2.BFMatcher(cv.NORM_HAMMING)
matches = matcher.match(desc1, desc2)

# selected = []
# for m, n in matches:
#     if m.distance < 0.7 * n.distance:
#         selected.append(m)

mat, _ = cv.findHomography(cv.KeyPoint.convert(keypoints1), cv.KeyPoint.convert(keypoints2), cv.RANSAC, 5.0, confidence=0.99)
# res = cv.warpPerspective(src, mat, (background.shape[0], background.shape[1]))
# print(cv.KeyPoint.convert(keypoints2))

(h, w) = src.shape[:2]

# 입력 영상의 모서리 4점 좌표
corners1 = np.array([[0, 0], [0, h-1], [w-1, h-1], [w-1, 0]]
                    ).reshape(-1, 1, 2).astype(np.float32)

# 입력 영상에 호모그래피 H 행렬로 투시 변환
corners2 = cv2.perspectiveTransform(corners1, mat)
res = np.copy(background)
# res = np.zeros(background.shape, dtype=background.dtype)
# 다각형 그리기
cv2.polylines(res, [np.int32(corners2)], True, (0, 255, 0), 2, cv2.LINE_AA)


res = cv.drawMatches(src, keypoints1, res, keypoints2, matches, None, flags=cv.DrawMatchesFlags_NOT_DRAW_SINGLE_POINTS)

# sz = max(1.0, res.shape[0] / 720, res.shape[1] / 1080)

res = imutils.resize(res, height=920)
# res = cv.resize(res, (int(res.shape[0] / sz), int(res.shape[1] / sz)))

cv.imshow('img', res)
cv.waitKey()
cv.destroyAllWindows()





