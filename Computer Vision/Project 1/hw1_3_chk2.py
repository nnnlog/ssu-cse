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
    return res


img = cv.imread("sample/img_6.png")
img = cv.resize(img, (1000, 1000))

modified_img = img
# modified_img = cv.GaussianBlur(modified_img, (9, 9), 1.1)
modified_img = cv.bilateralFilter(modified_img, 10, 32, 20)
# modified_img = cv.GaussianBlur(modified_img, (3, 3), 0.1)

dxdy = cv.Canny(modified_img, 100, 450)

# cv.imshow("img", dxdy)
# cv.waitKey(0)
# cv.destroyAllWindows()

res = cv.HoughLines(dxdy, 1, np.pi / 180, 100, None, 0, 0)

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
    attempts = kwargs.get('attempts', )

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
    if np.sin(np.median(segmented[0][1:])) > np.sin(np.median(segmented[1][1:])):
        segmented = list(reversed(segmented))
    return segmented


def intersection(line1, line2):
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
    if abs(np.linalg.det(A)) < 10 ** -2:
        if np.abs(rho1 - rho2) <= 10:
            return (0, 0)
        else:
            return (-10 ** 9, -100)
    b = np.array([[rho1], [rho2]])
    x0, y0 = np.linalg.solve(A, b)
    # print(int(np.round(x0)), int(np.round(y0)))
    x0, y0 = np.round(x0).astype(dtype=np.int32)[0], np.round(y0).astype(dtype=np.int32)[0]
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


res_edge = np.zeros((1000, 1000), dtype=np.uint8)
segmented = segment_by_angle_kmeans(res)

final_segmented = []
for segment in segmented:
    segment = segment
    curr = []
    vis = [0 for i in range(len(segment))]
    for i in range(len(segment)):
        if vis[i]: continue
        for j in range(i + 1, len(segment)):
            if vis[j]: continue
            x, y = intersection(segment[i], segment[j])
            if -50 <= x <= 1050 and -50 <= y <= 1050:
                vis[j] = 1
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

intersections, X, Y, inv_X, inv_Y = segmented_intersections(final_segmented)
# print(X, Y)

res_edge = cv.cvtColor(res_edge, cv.COLOR_GRAY2BGR)
for color, curr in [((255, 255, 0), final_segmented[0]), ((255, 0, 255), final_segmented[1])]:
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
            cv.line(res_edge, (x1, y1), (x2, y2), color, 1, cv.LINE_8)

for idx, i in enumerate(intersections):
    # x, y = pt
    # if inv_Y[idx][1] % 2: y += 30
    # print(str(idx) + " " + str(inv_X[idx]) + " " + str(inv_Y[idx]))
    # cv.putText(res_edge, str(idx) + " " + str(inv_X[idx]) + " " + str(inv_Y[idx]), (x, y), cv.FONT_HERSHEY_SIMPLEX, 0.3, (255, 255, 255), 1, cv.LINE_AA)
    cv.circle(res_edge, i, 0, (0, 0, 255), 5)

# cv.imshow("img", res_edge)
# cv.waitKey(0)
# cv.destroyAllWindows()

points = []

# Find Square
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
        if inv_X[pt3][0] != inv_X[pt4][0]: continue
        # if (pt1, pt2, pt3, pt4) == (14, 25, 92, 51):
        #     print(pt1, pt2, pt3, pt4)
        #     print(y1, y2, inv_X[pt3], inv_X[pt4])
        #     tmp = np.zeros((1000, 1000, 3), dtype=np.uint8)
        #     cv.line(tmp, intersections[pt1], intersections[pt2], (255, 255, 255), 1, cv.LINE_AA)
        #     cv.line(tmp, intersections[pt2], intersections[pt4], (255, 255, 255), 1, cv.LINE_AA)
        #     cv.line(tmp, intersections[pt3], intersections[pt4], (255, 255, 255), 1, cv.LINE_AA)
        #     cv.line(tmp, intersections[pt3], intersections[pt1], (255, 255, 255), 1, cv.LINE_AA)
        #     # cv.imshow("asdf", tmp)
        #     # cv.waitKey(0)
        #     # cv.destroyAllWindows()
        #     continue
        points.append((pt1, pt2, pt4, pt3))

Xcnt = []
Ycnt = []

for i in range(len(X)):
    Xcnt.append([0, 0])
for i in range(len(Y)):
    Ycnt.append([0, 0])

success = []
fail = []

squares = []
for pt1, pt2, pt3, pt4 in points:
    arr = [(pt1, pt2), (pt2, pt3), (pt3, pt4), (pt4, pt1)]
    cnt = 0
    for a, b in arr:
        tmp = np.zeros((1000, 1000), dtype=np.uint8)
        cv.line(tmp, intersections[a], intersections[b], (255, 255, 255), 3, cv.LINE_8)

        # cv.imshow("asdf", tmp)
        # cv.waitKey(0)

        total = np.count_nonzero(tmp)
        tmp[dilated_dxdy == 0] = 0
        curr = np.count_nonzero(tmp)

        # print(curr / total)
        # cv.imshow("asdf", tmp)
        # cv.waitKey(0)

        # cv.destroyAllWindows()
        if curr / total >= 0.8:
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

Xavg, Yavg = [(np.sum(Xcnt[i][0]) / Xcnt[i][1] if Xcnt[i][1] else 0) for i in range(len(Xcnt))], [
    (np.sum(Ycnt[i][0]) / Ycnt[i][1] if Ycnt[i][1] else 0) for i in range(len(Ycnt))]
# print(Xavg, Yavg)
accepted_square = []

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
                if Xavg[inv_X[i][0]] <= 0.6: ok -= 1
                if Yavg[inv_Y[i][0]] <= 0.6: ok -= 1
        if ok > -4:
            ok = 1
        else:
            # tmp = np.zeros((1000, 1000, 3), dtype=np.uint8)
            # for a, b in arr:
            #     cv.line(tmp, intersections[a], intersections[b], (255, 255, 255), 1, cv.LINE_AA)
            # cv.imshow("asdf", tmp)
            # cv.waitKey(0)
            # cv.destroyAllWindows()
            ok = 0
    if ok:
        accepted_square.append(((arr[0][0], arr[0][1], arr[1][0], arr[1][1]), inter))
        for a, b in arr:
            cv.line(final_result, intersections[a], intersections[b], (255, 255, 255), 1, cv.LINE_AA)
            cv.line(img, intersections[a], intersections[b], (255, 0, 255), 1, cv.LINE_AA)

if len(accepted_square) == 0:
    print("There is no square.")
    exit(0)

# Make square component using union-find
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

cnt, root = sorted(S)[-1]

n = np.round(np.sqrt(cnt))

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

xx, yy = [], []
for idx, val in enumerate(cntX):
    if val < 1: continue
    # if val < np.sqrt(midX): continue
    xx.append((val, idx))
for idx, val in enumerate(cntY):
    if val < 1: continue
    # if val < np.sqrt(midY): continue
    yy.append((val, idx))
xx.sort()
yy.sort()
# print(xx, yy)
a, b = xx[0][1], xx[1][1]
c, d = yy[0][1], yy[1][1]

print("board size: " + str(int(xx[len(xx) // 2][0] // 4)) + " * " + str(int(yy[len(yy) // 2][0] // 4)))

l1, l2, l3, l4 = final_segmented[0][a], final_segmented[0][b], final_segmented[1][c], final_segmented[1][d]

if l1[0][0] > l2[0][0]:
    l1, l2 = l2, l1
if l3[0][0] > l4[0][0]:
    l3, l4 = l4, l3

# print(a, b, c, d)
for line in [l1, l2, l3, l4]:
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
        cv.line(img, (x1, y1), (x2, y2), (255, 0, 0), 2, cv.LINE_8)

pt1, pt2, pt3, pt4 = intersection(l1, l3), intersection(l1, l4), intersection(l2, l4), intersection(l2, l3)
for i in range(2):
    def ccw(a, b, c, d, e, f):
        return a * d + c * f + b * e - b * c - d * e - a * f
    # print(l1, l2, l3, l4)
    pt1, pt2, pt3, pt4 = intersection(l1, l3), intersection(l1, l4), intersection(l2, l4), intersection(l2, l3)
    if ccw(*pt1, *pt2, *pt3) < 0:
        l3, l4 = l4, l3
        continue
    for i, pt in enumerate([pt1, pt2, pt3, pt4]):
        cv.putText(img, str(i) + " " + str(pt), pt, cv.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 0), 1, cv.LINE_AA)
    print("projection coordinates:", pt1, pt2, pt3, pt4)
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

cv.imshow("img", img)
cv.waitKey(0)
cv.destroyAllWindows()

res = convert_image_to_square(img, [pt1, pt2, pt3, pt4])
cv.imshow("img", res)
cv.waitKey(0)
cv.destroyAllWindows()
