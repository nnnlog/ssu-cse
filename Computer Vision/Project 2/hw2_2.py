from ultralytics.utils.plotting import Annotator

from ultralytics import YOLO

import sys

import cv2
import imutils
import numpy as np


def ccw(A, B, C):
    a, b = A
    c, d = B
    e, f = C
    return a * d + c * f + b * e - b * c - d * e - a * f


def detect_empire_state_building(image_path, template_path, template_sz):
    # Read the image and the template
    img = cv2.imread(image_path)
    template = cv2.imread(template_path)  # Convert to grayscale
    original = np.copy(img)
    if img is None or template is None: return None

    img = cv2.bilateralFilter(img, 9, 32, 32)
    template = cv2.bilateralFilter(template, 9, 32, 32)

    img = cv2.GaussianBlur(img, (3, 3), 0.1)
    # img = cv2.addWeighted(img, 1.5, blur, -0.5, 0)
    template = cv2.GaussianBlur(template, (3, 3), 0.1)
    # template = cv2.addWeighted(template, 1.5, blur, -0.5, 0)

    # img = imutils.resize(img, height=1080)
    # template = imutils.resize(template, height=920)
    if sz is not None: template = imutils.resize(template, height=template_sz)

    # Initialize the SIFT detector
    sift = cv2.SIFT.create()

    # Find the key points and descriptors with SIFT
    # kp1, des1 = sift.detectAndCompute(cv2.cvtColor(template, cv2.COLOR_BGR2GRAY), None)
    # kp2, des2 = sift.detectAndCompute(cv2.cvtColor(img, cv2.COLOR_BGR2GRAY), None)
    kp1, des1 = sift.detectAndCompute(template, None)
    kp2, des2 = sift.detectAndCompute(img, None)

    # FLANN parameters
    FLANN_INDEX_KDTREE = 1
    index_params = dict(algorithm=FLANN_INDEX_KDTREE, trees=5)
    search_params = dict(checks=50)

    # FLANN based matcher
    flann = cv2.FlannBasedMatcher(index_params, search_params)
    matches = flann.knnMatch(des1, des2, k=2)

    # Need to draw only good matches, so create a mask
    matchesMask = [[0, 0] for _ in range(len(matches))]

    # ratio test as per Lowe's paper
    total = 0
    for i, (m, n) in enumerate(matches):
        if m.distance < 0.7 * n.distance:
            matchesMask[i] = [1, 0]
            total += 1

    if total < 4:
        return None

    draw_params = dict(
        matchesMask=matchesMask,
        flags=0,
    )

    img_matches = np.copy(img)
    # img_matches = np.zeros(img.shape, dtype=img.dtype)

    # Use findHomography to find a perspective transformation
    src_pts = np.float32([kp1[m[0].queryIdx].pt for i, m in enumerate(matches) if matchesMask[i][0] == 1]).reshape(-1,
                                                                                                                   1, 2)
    dst_pts = np.float32([kp2[m[0].trainIdx].pt for i, m in enumerate(matches) if matchesMask[i][0] == 1]).reshape(-1,
                                                                                                                   1, 2)

    M, mask = cv2.findHomography(src_pts, dst_pts, cv2.RANSAC, 5.0)
    if M is None: return None
    # Apply perspective transformation to the corners of the image
    w, h = template.shape[:2]
    corners = np.float32([[0, 0], [0, w - 1], [h - 1, w - 1], [h - 1, 0]]).reshape(-1, 1, 2)
    transformed_corners = cv2.perspectiveTransform(corners, M)

    w, h = img_matches.shape[:2]
    transformed_corners[0][0][0] = max(0, min(h, int(transformed_corners[0][0][0])))
    transformed_corners[0][0][1] = max(0, min(w, int(transformed_corners[0][0][1])))
    transformed_corners[1][0][0] = max(0, min(h, int(transformed_corners[1][0][0])))
    transformed_corners[1][0][1] = max(0, min(w, int(transformed_corners[1][0][1])))
    transformed_corners[2][0][0] = max(0, min(h, int(transformed_corners[2][0][0])))
    transformed_corners[2][0][1] = max(0, min(w, int(transformed_corners[2][0][1])))
    transformed_corners[3][0][0] = max(0, min(h, int(transformed_corners[3][0][0])))
    transformed_corners[3][0][1] = max(0, min(w, int(transformed_corners[3][0][1])))

    # print(M, corners, transformed_corners)
    mnX = 1e9
    mxX = 0
    mnY = 1e9
    mxY = 0
    for i in range(0, 4):
        # print(ccw(transformed_corners[i][0], transformed_corners[(i + 1) % 4][0], transformed_corners[(i + 2) % 4][0]))
        x, y = transformed_corners[i][0]
        if ccw(transformed_corners[i][0], transformed_corners[(i + 1) % 4][0], transformed_corners[(i + 2) % 4][0]) > 0:
            return None
        mnX = min(mnX, x)
        mxX = max(mxX, x)
        mnY = min(mnY, y)
        mxY = max(mxY, y)

    # print(mnX, mxX, mnY, mxY)
    if mnY == mxY or (mxX - mnX) / (mxY - mnY) > 0.5:
        return None
    if (mxX - mnX) * (mxY - mnY) < 100:
        return None

    cnt = 0
    for i, v in enumerate(matchesMask):
        if v[0] == 0: continue
        x, y = kp2[matches[i][0].trainIdx].pt
        if mnX > x or x > mxX: continue
        if mnY > y or y > mxY: continue
        cnt += 1
    # print(cnt / total, cnt, total, (mxX - mnX) / (mxY - mnY))
    # print(mnX, mxX, mnY, mxY)
    if cnt / total <= 0.1 or cnt <= 10:
        return None

    # Draw the transformed rectangle on the second image
    img_rect = cv2.polylines(img_matches, [np.int32(transformed_corners)], True, (0, 0, 255), 3, cv2.LINE_AA)
    original = cv2.polylines(original, [np.int32(transformed_corners)], True, (0, 0, 255), 3, cv2.LINE_AA)
    img_matches = np.copy(img_rect)

    # cv2.imshow("Matches", img_matches)
    # cv2.waitKey(0)
    # cv2.destroyAllWindows()

    # img_matches = cv2.drawMatchesKnn(template, kp1, img_matches, kp2, matches, None, **draw_params)

    # img_matches = imutils.resize(img_matches, height=920)
    # cv2.imshow("Matches", img_matches)
    # cv2.waitKey(500)

    img_rect = imutils.resize(img_rect, height=920)
    original = imutils.resize(original, height=920)
    return original

model = YOLO("best.pt")

im = sys.argv[1]
# im = f"tests_yes/{i}.jpg"
results = model.predict(im)

for r in results:
    annotator = Annotator(cv2.imread(im))

    cnt = 0

    boxes = r.boxes
    for box in boxes:
        if box.conf[0] < 0.85: continue
        annotator.box_label(box.xyxy[0], color=(0, 0, 255))
        cnt += 1
        break

    if cnt == 0:
        ok = False
        for sz in [920]:
            if ok: break
            for j in range(0, 6):
                res = detect_empire_state_building(im, f"models/{j}.jpg", sz)
                if res is not None:
                    ok = True
                    img = res
                    break
        if not ok:
            print("False")
            break
    else:
        img = annotator.result()

    # print("True")
    img = imutils.resize(img, height=920)
    cv2.imshow('res', img)
    cv2.waitKey()
    cv2.destroyAllWindows()
