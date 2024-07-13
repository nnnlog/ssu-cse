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


# img = cv.imread("dataset/img_5.png")
img = cv.imread(sys.argv[1])
origin_shape = img.shape
origin_sz = origin_shape[0] * origin_shape[1]
img = cv.resize(img, (1000, 1000))

modified_img = img
if origin_sz < 700 ** 2:
    blur = cv.GaussianBlur(modified_img, (0, 0), 5)
    modified_img = cv.addWeighted(modified_img, 1.5, blur, -0.5, 0)
    # modified_img = cv.GaussianBlur(modified_img, (3, 3), 0.1)
elif origin_sz < 1000 ** 2:
    blur = cv.GaussianBlur(modified_img, (0, 0), 3)
    modified_img = cv.addWeighted(modified_img, 1.5, blur, -0.5, 0)
    # modified_img = cv.GaussianBlur(modified_img, (3, 3), 0.1)
elif origin_sz < 1300 ** 2:
    modified_img = cv.GaussianBlur(modified_img, (5, 5), 1.1)
    blur = cv.GaussianBlur(modified_img, (0, 0), 3)
    modified_img = cv.addWeighted(modified_img, 1.5, blur, -0.5, 0)
else:
    modified_img = cv.GaussianBlur(modified_img, (9, 9), 1.3)
    blur = cv.GaussianBlur(modified_img, (0, 0), 5)
    modified_img = cv.addWeighted(modified_img, 1.3, blur, -0.3, 0)

modified_img = cv.bilateralFilter(modified_img, 10, 32, 16)

dxdy = cv.Canny(modified_img, 100, 450)
# tmp = cv.medianBlur(dxdy, 3)
# tmp = cv.dilate(tmp, np.array([
#     [0, 0, 1, 0, 0],
#     [0, 0, 1, 0, 0],
#     [1, 1, 1, 1, 1],
#     [0, 0, 1, 0, 0],
#     [0, 0, 1, 0, 0]
# ], dtype=np.uint8), iterations=3)
# # tmp = cv.medianBlur(dxdy, 3)
# # cv.imshow("img", tmp)
# # cv.waitKey(0)
# # cv.destroyAllWindows()
#
# dxdy[tmp != 0] = 0
#
# # tmp = cv.medianBlur(dxdy, 5)
#
# tmp = cv.GaussianBlur(tmp, (5, 5), 5)

# cv.imshow("img", dxdy)
# cv.waitKey(0)
# cv.destroyAllWindows()

thres = 90
if origin_sz < 600 ** 2:
    thres = 60
elif origin_sz < 800 * 2:
    thres = 70
elif origin_sz < 900 * 2:
    thres = 80
res = cv.HoughLines(dxdy, 1, np.pi / 180, thres, None, 0, 0)

# print(res)

if res is None:
    print("There is no edge.")
    exit(0)

res_edge = np.zeros((1000, 1000), dtype=np.uint8)

for line in res:
    length = 5000
    for rho, theta in line:
        a = np.cos(theta)
        b = np.sin(theta)
        x0 = a * rho
        y0 = b * rho
        x1 = int(x0 + length * (-b))
        y1 = int(y0 + length * (a))
        x2 = int(x0 - length * (-b))
        y2 = int(y0 - length * (a))
        cv.line(res_edge, (x1, y1), (x2, y2), (255, 255, 255), 1, cv.LINE_AA)


# cv.imshow("dxdy", dxdy)
# cv.waitKey(0)
# cv.imshow("img", res_edge)
# cv.waitKey(0)

def get_linear_by_polar(roh, theta):
    a = np.cos(theta)
    b = np.sin(theta)
    x0 = a * rho
    y0 = b * rho
    x1 = int(x0 + length * (-b))
    y1 = int(y0 + length * (a))
    x2 = int(x0 - length * (-b))
    y2 = int(y0 - length * (a))
    return (x1, y1), (x2, y2)


# https://stackoverflow.com/questions/46565975/find-intersection-point-of-two-lines-drawn-using-houghlines-opencv
from collections import defaultdict


def segment_by_angle_kmeans(lines, k=2, **kwargs):
    """Groups lines based on angle with k-means.

    Uses k-means on the coordinates of the angle on the unit circle
    to segment `k` angles inside `lines`.
    """

    # Define criteria = (type, max_iter, epsilon)
    default_criteria_type = cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_MAX_ITER
    criteria = kwargs.get('criteria', (default_criteria_type, 10, 1.0))
    flags = kwargs.get('flags', cv.KMEANS_RANDOM_CENTERS)
    attempts = kwargs.get('attempts', 100)

    # returns angles in [0, pi] in radians
    angles = np.array([line[0][1] for line in lines])
    # multiply the angles by two and find coordinates of that angle
    pts = np.array([[np.cos(2 * angle), np.sin(2 * angle)]
                    for angle in angles], dtype=np.float32)

    # run kmeans on the coords
    labels, centers = cv.kmeans(pts, k, None, criteria, attempts, flags)[1:]
    labels = labels.reshape(-1)  # transpose to row vec

    # segment lines based on their kmeans label
    segmented = defaultdict(list)
    for i, line in enumerate(lines):
        segmented[labels[i]].append(line)
    segmented = list(segmented.values())
    # segmented = list(reversed(segmented))
    avg1, avg2 = np.average([abs(np.sin(segmented[0][i][0][1])) for i in range(len(segmented[0]))]), np.average(
        [abs(np.sin(segmented[1][i][0][1])) for i in range(len(segmented[1]))])
    # print(avg1, avg2)
    if avg1 < avg2:
        segmented = list(reversed(segmented))
    return segmented


def intersection(line1, line2, rohThres=40):
    """Finds the intersection of two lines given in Hesse normal form.

    Returns closest integer pixel locations.
    See https://stackoverflow.com/a/383527/5087436
    """
    rho1, theta1 = line1[0]
    rho2, theta2 = line2[0]
    A = np.array([
        [np.cos(theta1), np.sin(theta1)],
        [np.cos(theta2), np.sin(theta2)]
    ])
    if abs(np.linalg.det(A)) <= 10 ** -4:
        if np.abs(rho1 - rho2) <= rohThres:
            return (0, 0)
        else:
            return (-10 ** 9, 0)

    b = np.array([[rho1], [rho2]])
    x0, y0 = np.linalg.solve(A, b)
    # print(int(np.round(x0)), int(np.round(y0)))
    x0, y0 = np.round(x0).astype(dtype=np.int32)[0], np.round(y0).astype(dtype=np.int32)[0]

    if abs(np.linalg.det(A)) <= 10 ** -1.5:
        if np.abs(rho1 - rho2) <= rohThres:
            return (0, 0)
        else:
            return (x0, y0)
    # print(x0, y0)
    return (x0, y0)


def segmented_intersections(lines):
    """Finds the intersections between groups of lines."""

    assert len(lines) == 2

    idx = 0
    inv_X = {}
    inv_Y = {}
    X = []
    Y = []

    for i in range(len(lines[0])): X.append([])
    for i in range(len(lines[1])): Y.append([])

    intersections = []
    intersections_inv = []
    tmp = []
    for x, line1 in enumerate(lines[0]):
        for y, line2 in enumerate(lines[1]):
            pt = intersection(line1, line2)
            if not (0 <= pt[0] <= 999) or not (0 <= pt[1] <= 999): continue
            intersections.append(pt)
            intersections_inv.append((pt[1], pt[0]))
            tmp.append((intersection(line1, line2), idx))
            X[x].append(idx)
            Y[y].append(idx)
            idx += 1

    for i, x in enumerate(X):
        x.sort(key=cmp_to_key(lambda a, b: -1 if intersections[a] < intersections[b] else 0))
        for j in range(len(x)):
            inv_X[x[j]] = (i, j)

    for i, y in enumerate(Y):
        y.sort(key=cmp_to_key(lambda a, b: -1 if intersections_inv[a] < intersections_inv[b] else 0))
        for j in range(len(y)):
            inv_Y[y[j]] = (i, j)

    return intersections, X, Y, inv_X, inv_Y


# Main Logic

# 처음 나온 직선을 수평/수직 선분으로 나눕니다.
segmented = segment_by_angle_kmeans(res)

tmp = []

# 수평/수직 선분에 있는 직선끼리는 서로 만나서는 안 됩니다. 교점이 충분한 오차 범위 내 격자 안에서 생기는 직선들을 제거합니다.
res_edge = np.zeros((1000, 1000), dtype=np.uint8)
final_segmented = []
for segment in segmented:
    # random.shuffle(segment)
    curr = []
    vis = [0 for i in range(len(segment))]
    for i in range(len(segment)):
        if vis[i]: continue
        for j in range(i + 1, len(segment)):
            if vis[j]: continue
            x, y = intersection(segment[i], segment[j])
            if get_dist(500, 500, x, y) <= 1100:
                vis[j] = 1
            # if -150 <= x <= 1150 and -150 <= y <= 1150: vis[j] = 1
            # else: tmp.append((x, y))
        curr.append(segment[i])
    for line in curr:
        length = 5000
        for rho, theta in line:
            a = np.cos(theta)
            b = np.sin(theta)
            x0 = a * rho
            y0 = b * rho
            x1 = int(x0 + length * (-b))
            y1 = int(y0 + length * (a))
            x2 = int(x0 - length * (-b))
            y2 = int(y0 - length * (a))
            cv.line(res_edge, (x1, y1), (x2, y2), (255, 255, 255), 1, cv.LINE_8)
    final_segmented.append(curr)

# tmp.sort(key=cmp_to_key(lambda a, b: get_dist(0, 0, *a) - get_dist(0, 0, *b)))
# print(tmp)

# Display detected refined single line
# cv.imshow("img", res_edge)
# cv.waitKey(0)
# cv.destroyAllWindows()

# 선분이 없는 자리에 직선이 그어졌는지 판단하기 위해서, 기존의 에지 검출 영상(Canny)에서 dilate 연산을 3회 적용합니다.
# 그런 후 선분과 에지 영상의 AND 논리 연산을 했을 때, 충분히 많은 에지 위에 선분이 있었는지 판단합니다.
# 이를 통해 선분이 없는 자리에 그어진 직선의 일부분이 상당수 제거됩니다.
dilated_dxdy = cv.dilate(dxdy, np.array([
    [0, 0, 1, 0, 0],
    [0, 0, 1, 0, 0],
    [1, 1, 1, 1, 1],
    [0, 0, 1, 0, 0],
    [0, 0, 1, 0, 0],
], dtype=np.uint8), iterations=3)

# res_edge에는 있고, dxdy에 없는 성분들을 제거하자.
# 인접한 픽셀이 있는가?
res_edge[dilated_dxdy == 0] = 0

# 모든 수평/수직 직선끼리는 교점이 생깁니다. 각 교점을 구하고, 사각형으로 묶기 위한 추가 정보들도 저장합니다.
intersections, X, Y, inv_X, inv_Y = segmented_intersections(final_segmented)
# print(X, Y)


############################################# debug #############################################
# for debug, 수평/수직 성분, 교점을 보여줍니다. 수평과 수직 성분의 색은 다르게 표현합니다.
# res_edge = cv.cvtColor(res_edge, cv.COLOR_GRAY2BGR)
# for color, curr in [((255, 255, 0), final_segmented[0]), ((255, 0, 255), final_segmented[1])]:
#     for line in curr:
#         length = 5000
#         for rho, theta in line:
#             a = np.cos(theta)
#             b = np.sin(theta)
#             x0 = a * rho
#             y0 = b * rho
#             x1 = int(x0 + length * (-b))
#             y1 = int(y0 + length * (a))
#             x2 = int(x0 - length * (-b))
#             y2 = int(y0 - length * (a))
#             cv.line(res_edge, (x1, y1), (x2, y2), color, 1, cv.LINE_8)
#
# for idx, i in enumerate(intersections):
#     # x, y = i
#     # if inv_Y[idx][1] % 2: y += 30
#     # print(str(idx) + " " + str(inv_X[idx]) + " " + str(inv_Y[idx]))
#     # cv.putText(res_edge, str(idx) + " " + str(inv_X[idx]) + " " + str(inv_Y[idx]), (x, y), cv.FONT_HERSHEY_SIMPLEX, 0.3, (255, 255, 255), 1, cv.LINE_AA)
#     cv.circle(res_edge, i, 0, (0, 0, 255), 5)
#
#
# cv.imshow("debug", res_edge)
# cv.waitKey(0)
# cv.destroyAllWindows()


points = []

# 모든 사각형을 찾습니다. 사각형의 점은 서로 인접하므로, 두 개의 점만 고정시키면 나머지 두 점은 정해집니다.
for i in range(len(X)):
    for j in range(len(X[i]) - 1):
        # j and j + i
        pt1 = X[i][j]
        pt2 = X[i][j + 1]
        y1, y2 = inv_Y[pt1], inv_Y[pt2]
        if len(Y[y1[0]]) <= y1[1] + 1: continue
        if len(Y[y2[0]]) <= y2[1] + 1: continue
        pt3 = Y[y1[0]][y1[1] + 1]
        pt4 = Y[y2[0]][y2[1] + 1]
        if inv_X[pt3][0] != inv_X[pt4][0]:
            # print(pt1, pt2, pt3, pt4)
            # print(intersections[pt1], intersections[pt2], intersections[pt3], intersections[pt4])
            # tmp = np.zeros((1000, 1000), dtype=np.uint8)
            # cv.line(tmp, intersections[pt1], intersections[pt3], (255, 255, 255), 1, cv.LINE_8)
            # cv.line(tmp, intersections[pt3], intersections[pt2], (255, 255, 255), 1, cv.LINE_8)
            # cv.line(tmp, intersections[pt2], intersections[pt4], (255, 255, 255), 1, cv.LINE_8)
            # cv.line(tmp, intersections[pt4], intersections[pt1], (255, 255, 255), 1, cv.LINE_8)
            # cv.imshow("asdf", tmp)
            # cv.waitKey(0)
            continue
        points.append((pt1, pt2, pt4, pt3))

Xcnt = []
Ycnt = []

for i in range(len(X)):
    Xcnt.append([0, 0])
for i in range(len(Y)):
    Ycnt.append([0, 0])

success = []
fail = []

# dilated dxdy(Canny) 영상과 사각형을 이루는 선분 중 하나만 긋고, 두 영상을 AND 연산을 합니다.
# 만약 AND 연산을 했을 때, 나타나는 픽셀의 비율이(원래 그은 선분에 비해) 적당한 상수(70%) 미만이라면 해당 선은 없었던 것으로 취급합니다. (이 경우를 invalid, 그렇지 않으면 valid)
# 사각형의 valid한 edge가 3개 이상이면 항상 accept하는 사각형으로 취급합니다. 그렇지 않은 사각형을 reject되었다고 정의합니다.
squares = []
for pt1, pt2, pt3, pt4 in points:
    arr = [(pt1, pt2), (pt2, pt3), (pt3, pt4), (pt4, pt1)]
    cnt = 0
    for a, b in arr:
        thick = 2
        # if origin_sz < 700 ** 2: thick = 4
        # elif origin_sz < 1000 ** 2: thick = 6

        tmp = np.zeros((1000, 1000), dtype=np.uint8)
        cv.line(tmp, intersections[a], intersections[b], (255, 255, 255), thick, cv.LINE_8)

        # cv.imshow("asdf", tmp)
        # cv.waitKey(0)

        total = np.count_nonzero(tmp)
        tmp[dilated_dxdy == 0] = 0
        curr = np.count_nonzero(tmp)

        # print(curr / total)
        # cv.imshow("asdf", tmp)
        # cv.waitKey(0)

        rate = 0.85
        if origin_sz < 600 ** 2:
            rate = 0.65
        elif origin_sz < 700 ** 2:
            rate = 0.75
        elif origin_sz < 1000 ** 2:
            rate = 0.8

        if curr / total >= rate:
            cnt += 1
            success.append((a, b))
            for i in [a, b]:
                Xcnt[inv_X[i][0]][0] += 1
                Xcnt[inv_X[i][0]][1] += 1
                Ycnt[inv_Y[i][0]][0] += 1
                Ycnt[inv_Y[i][0]][1] += 1
        else:
            fail.append((a, b))
            for i in [a, b]:
                Xcnt[inv_X[i][0]][1] += 1
                Ycnt[inv_Y[i][0]][1] += 1
    squares.append((cnt, arr, (pt1, pt2, pt3, pt4)))

# 관찰적으로 나온 통계 기법을 활용합니다.
# 실제 체커판을 구성하는 직선은 accepted square로부터의 reference rate가 상당히 높습니다. (못해도 60% 이상)
# 그렇지 않은 직선은 대부분 rejected square를 만들므로 reference rate가 낮습니다.
# 그러므로, 이 직선을 사용하여 만들어지는 rejected square가 존재한다면, 이 사각형도 체커판을 구성하는 것일 확률이 높으므로 accepted로 변경합니다.
Xavg, Yavg = [(np.sum(Xcnt[i][0]) / Xcnt[i][1] if Xcnt[i][1] else 0) for i in range(len(Xcnt))], [
    (np.sum(Ycnt[i][0]) / Ycnt[i][1] if Ycnt[i][1] else 0) for i in range(len(Ycnt))]
# print(Xavg, Yavg)
accepted_square = []

tmp = np.zeros((1000, 1000, 3), dtype=np.uint8)
final_result = np.zeros((1000, 1000, 3), dtype=np.uint8)
for cnt, arr, inter in squares:
    ok = 0
    if cnt >= 3:
        ok = 1
    else:
        curr = 0
        ok = 0
        for a, b in arr:
            for i in [a, b]:
                lim = 0.65
                if origin_sz < 600 ** 2:
                    lim = 0.65
                elif origin_sz < 800 ** 2:
                    lim = 0.65
                if Xavg[inv_X[i][0]] <= lim: ok -= 1
                if Yavg[inv_Y[i][0]] <= lim: ok -= 1
        if ok > -4:
            ok = 1
        else:
            # print(cnt, ok)
            # for a, b in arr:
            #     cv.line(tmp, intersections[a], intersections[b], (255, 255, 255), 1, cv.LINE_AA)
            # cv.waitKey(0)
            ok = 0
    if ok:
        accepted_square.append(((arr[0][0], arr[0][1], arr[1][0], arr[1][1]), inter))
        # for a, b in arr:
        #     cv.line(img, intersections[a], intersections[b], (255, 0, 255), 1, cv.LINE_AA)
        # cv.line(modified_img, intersections[a], intersections[b], (255, 0, 255), 1, cv.LINE_AA)

# cv.imshow("ASDFASDFASF", tmp)
if len(accepted_square) == 0:
    print("There is no square.")
    exit(0)

# 이제 인접한 사각형끼리 묶어 사각형 그룹을 만듭니다. 잘 알려진 Union-Find 알고리즘을 사용합니다.
UF = [i for i in range(len(accepted_square))]
UF_cnt = [1 for i in range(len(accepted_square))]


def _find(u):
    if UF[u] == u: return u
    UF[u] = _find(UF[u])
    return UF[u]


def _merge(u, v):
    u, v = _find(u), _find(v)
    if u == v: return
    UF[u] = v
    UF_cnt[v] += UF_cnt[u]


for i in range(len(accepted_square)):
    for j in range(i + 1, len(accepted_square)):
        a, b = accepted_square[i][0], accepted_square[j][0]
        exist = {}
        for k in a: exist[k] = 1
        ok = 0
        for k in b:
            if k in exist:
                ok = 1
                break
        if ok:
            _merge(i, j)

S = set()
for i in range(len(accepted_square)):
    S.add((UF_cnt[_find(i)], _find(i)))

# 컴포넌트 중 사각형의 개수가 가장 많은 것을 "체커판"으로써 택합니다.
cnt, root = sorted(S)[-1]

# 체커판 안에는 모든 사각형이 포함되어 있다고 할 수 있을까요? 그렇지 않습니다.
# 이제 이 체커판의 BoundingBox를 찾기 위해 다시 경험적 통계 기법을 사용합니다.

# 직선이 사각형에 의해 참조되는 횟수를 셉니다.
cntX, cntY = [0 for i in range(len(X))], [0 for i in range(len(Y))]
for i, (_, (pt1, pt2, pt3, pt4)) in enumerate(accepted_square):
    if _find(i) != root: continue
    cntX[inv_X[pt1][0]] += 1
    cntX[inv_X[pt2][0]] += 1
    cntX[inv_X[pt3][0]] += 1
    cntX[inv_X[pt4][0]] += 1
    cntY[inv_Y[pt1][0]] += 1
    cntY[inv_Y[pt2][0]] += 1
    cntY[inv_Y[pt3][0]] += 1
    cntY[inv_Y[pt4][0]] += 1

# print(cntX, cntY)

_cntX, _cntY = {}, {}

for i in cntX:
    if not i in _cntX: _cntX[i] = 0
    _cntX[i] += 1
for i in cntY:
    if not i in _cntY: _cntY[i] = 0
    _cntY[i] += 1

# BoundingBox를 계산합니다. 체커판의 테두리를 구성하는 직선들의 참조 횟수는 체커판 중간에 있는 직선보다 2배 작아야 합니다. 이를 계산합니다.
midX = max(_cntX)
midY = max(_cntY)
# midX = max(max(_cntX, key=_cntX.get), np.median(cntX))
# midY = max(max(_cntY, key=_cntY.get), np.median(cntY))

xx, yy = [], []
for idx, val in enumerate(cntX):
    if val < midX / 5: continue
    # if val < np.sqrt(midX) / 1.5: continue
    xx.append((val, idx))
for idx, val in enumerate(cntY):
    if val < midY / 5: continue
    # if val < np.sqrt(midY) / 1.5: continue
    yy.append((val, idx))
xx.sort()
yy.sort()
# print(xx, yy)
a, b = xx[0][1], xx[1][1]
c, d = yy[0][1], yy[1][1]

_cntX, _cntY = {}, {}

for i, _ in xx:
    if not i in _cntX: _cntX[i] = 0
    _cntX[i] += 1
for i, _ in yy:
    if not i in _cntY: _cntY[i] = 0
    _cntY[i] += 1

# midX = max(_cntX, key=_cntX.get)
# midY = max(_cntY, key=_cntY.get)
midX = max(_cntX.keys())
midY = max(_cntY.keys())

ansX, ansY = int(midX // 4), int(midY // 4)
# print("detected board size: " + str(ansX) + " * " + str(ansY))

if ansX != ansY: ansX, ansY = max(ansX, ansY), max(ansX, ansY)
if ansX < 10:
    ansX, ansY = 8, 8
else:
    ansX, ansY = 10, 10
# print("refined board size (8*8 or 10*10): " + str(ansX) + " * " + str(ansY))

# 테두리를 구성하는 4개의 직선을 가져옵니다.
l1, l2, l3, l4 = final_segmented[0][a], final_segmented[0][b], final_segmented[1][c], final_segmented[1][d]

if l1[0][0] > l2[0][0]:
    l1, l2 = l2, l1
if l3[0][0] > l4[0][0]:
    l3, l4 = l4, l3

# print(a, b, c, d)
# for line in [l1, l2, l3, l4]:
#     length = 5000
#     for rho, theta in line:
#         a = np.cos(theta)
#         b = np.sin(theta)
#         x0 = a * rho
#         y0 = b * rho
#         x1 = int(x0 + length * (-b))
#         y1 = int(y0 + length * (a))
#         x2 = int(x0 - length * (-b))
#         y2 = int(y0 - length * (a))
#         cv.line(img, (x1, y1), (x2, y2), (255, 0, 0), 2, cv.LINE_8)

# 테두리 직선 간의 교점을 구합니다.
pt1, pt2, pt3, pt4 = intersection(l1, l3), intersection(l1, l4), intersection(l2, l4), intersection(l2, l3)
for i in range(2):
    def ccw(a, b, c, d, e, f):
        return a * d + c * f + b * e - b * c - d * e - a * f


    # print(l1, l2, l3, l4)
    pt1, pt2, pt3, pt4 = intersection(l1, l3), intersection(l1, l4), intersection(l2, l4), intersection(l2, l3)
    # 반시계 방향이면 시계 방향으로 돌립니다.
    if ccw(*pt1, *pt2, *pt3) < 0:
        l3, l4 = l4, l3
        continue
    # for i, pt in enumerate([pt1, pt2, pt3, pt4]):
    #     cv.putText(img, str(i) + " " + str(pt), pt, cv.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 0), 1, cv.LINE_AA)
    # print("projection coordinates:", pt1, pt2, pt3, pt4)
    break

# print(pt1, pt2, pt3, pt4)

# projection_img;

# cv.imshow("img", dilated_dxdy)
# cv.waitKey(0)
# cv.destroyAllWindows()
#
# cv.imshow("img", final_result)
# cv.waitKey(0)
# cv.destroyAllWindows()

# cv.imshow("img", img)
# cv.waitKey(0)
# cv.destroyAllWindows()

# Projection!
img, mat = convert_image_to_square(img, [pt1, pt2, pt3, pt4])

modified_img = cv.bilateralFilter(img, -1, 32, 16)

gray = cv.cvtColor(modified_img, cv.COLOR_BGR2GRAY)
gray = cv.Canny(gray, 20, 100)
gray = cv.GaussianBlur(gray, (5, 5), 2)

# tmp = np.zeros(shape=img.shape, dtype=np.uint8)
# cv.circle(tmp, center, radius, (255, 255, 255), cv.FILLED)
# if np.count_nonzero(cv.bitwise_and(curr, tmp)) <= curr.shape[0] * curr.shape[1] // 3: continue
# cv.circle(mask, center, radius, (255, 255, 255), cv.FILLED)
# ok = 1
# tmp = np.copy(curr)
# cv.circle(tmp, center, radius, (255, 0, 255))

# gray = cv.adaptiveThreshold(gray, 255, cv.ADAPTIVE_THRESH_GAUSSIAN_C, cv.THRESH_BINARY, 7, 2)
_, gray = cv.threshold(gray, 0, 255, cv.THRESH_BINARY + cv.THRESH_OTSU)

# cv.imshow("img", gray)
# cv.waitKey(0)
# cv.imshow("img", tmp)
# cv.waitKey(0)

colors = []

rows = gray.shape[0]
circles = cv.HoughCircles(gray, cv.HOUGH_GRADIENT, 1, rows / 12,
                          param1=60, param2=20,
                          minRadius=20, maxRadius=50)
if circles is not None:
    circles = np.uint16(np.around(circles))
    ok = 0
    for i in circles[0, :]:
        mask = np.zeros(shape=gray.shape, dtype=np.uint8)
        center = (i[0], i[1])
        radius = i[2] - 15
        # tmp = np.zeros(shape=gray.shape, dtype=np.uint8)
        # tmp = np.copy(modified_img)
        # cv.circle(tmp, center, radius, (255, 0, 255))

        # if np.count_nonzero(cv.bitwise_and(img, tmp)) <= img.shape[0] * img.shape[1] // 8: continue
        cv.circle(mask, center, radius, (255, 255, 255), cv.FILLED)
        ok = 1
        cv.circle(img, center, radius, (255, 0, 255), 2)

        # colors.append(cv.mean(modified_img, mask=mask)[:3])
        # colors.append(cv.mean(cv.cvtColor(modified_img, cv.COLOR_BGR2HLS), mask=cv.cvtColor(mask, cv.COLOR_BGR2GRAY))[:3])

        # blur = np.copy(modified_img)
        # blur = cv.GaussianBlur(blur, (5, 5), 1)
        # curr_gray = cv.cvtColor(blur, cv.COLOR_BGR2GRAY)
        hls = cv.cvtColor(modified_img, cv.COLOR_BGR2HLS)
        # curr = []
        # # print(np.array(np.where(mask == 255)).T)
        # for i, j in np.array(np.where(mask == 255)).T:
        #     # curr.append(modified_img[i][j])
        #     curr.append(hls[i][j][1])

        curr = cv.mean(hls, mask=mask)[1]
        colors.append(curr)
        # print(curr)
        # tmp = np.copy(modified_img)
        # tmp[mask != 255] = 0
        # cv.imshow("img", tmp)
        # cv.waitKey(0)

        # Lchanneld = cv.bitwise_and(hsl, mask)[:, :, 1]
        # print(cv.bitwise_and(hsl, mask)[0][0])
        # lvalueld = cv.mean(Lchanneld)[0]
        # print(np.mean(curr), np.median(curr))
        # colors.append(np.median(curr))
        # curr_gray = cv.cvtColor(img, cv.COLOR_BGR2GRAY)
        # colors.append(np.median(curr_gray[np.nonzero(cv.bitwise_and(curr_gray, mask))]))
    # cv.imshow("img", curr)
    # cv.waitKey(0)

# print(colors)
if len(colors) <= 1:
    print(f"w:0 b:0")
    exit(0)

colors = np.array(colors, dtype=np.float32)
labels, centers = cv.kmeans(colors, 2, None, (cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_MAX_ITER, 10, 1.0), 1000,
                            cv.KMEANS_RANDOM_CENTERS)[1:]
labels = labels.reshape(-1)  # transpose to row vec
groups = [[], []]
for i, color in enumerate(colors):
    groups[labels[i]].append(color)
if np.median(groups[0]) < np.median(groups[1]): groups = list(reversed(groups))

# groups = [[], []]
# for i, color in enumerate(colors):
#     groups[0 if color < np.mean(cv.cvtColor(modified_img, cv.COLOR_BGR2GRAY)) else 1].append(color)

print(f"w:{len(groups[0])} b:{len(groups[1])}")
# print(groups)
# print(np.mean(cv.cvtColor(modified_img, cv.COLOR_BGR2GRAY)), np.mean(groups[0]), np.mean(groups[1]))

cv.imshow("img", img)
cv.waitKey(0)

cv.destroyAllWindows()
