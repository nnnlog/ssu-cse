import cv2 as cv
import numpy as np
from math import *
import matplotlib.pyplot as plt

img = cv.imread("sample/img_1.png")
img = cv.resize(img, (1000, 1000))


def convert_image_to_square(arr):
    sz = max(abs(arr[1][0] - arr[0][0]), abs(arr[2][1] - arr[1][1]), abs(arr[3][0] - arr[2][0]),
             abs(arr[3][1] - arr[0][1]))
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


def get_dist(a, b, c, d):
    return sqrt(pow(a - c, 2) + pow(b - d, 2))

def get_dist_between_two_skew_line_in_xy_coord(A, B):
    a, b, c, d = A
    w, x, y, z = B

    t1 = np.arctan2(c - a, d - b)
    r1 = a * np.cos(t1) + b * np.sin(t1)

    t2 = np.arctan2(y - w, z - x)
    r2 = w * np.cos(t2) + x * np.sin(t2)

    def _internal_solve(mid):
        if np.sin(t2) == 0 or np.sin(t1) == 0:
            return abs(a - w)
        l, r = w, y
        y1 = (r1 - mid * np.cos(t1)) / np.sin(t1)
        while l + 2 < r:
            p, q = (l + l + r) / 3, (l + r + r) / 3
            y2_p = (r2 - p * np.cos(t2)) / np.sin(t2)
            y2_q = (r2 - q * np.cos(t2)) / np.sin(t2)
            if get_dist(mid, y1, p, y2_p) < get_dist(mid, y1, q, y2_q):
                r = q
            else:
                l = p
        l = (l + r) / 2
        return get_dist(mid, y1, l, (r2 - l * np.cos(t2)) / np.sin(t2))

    l, r = a, c
    while l + 2 < r:
        p, q = (l + l + r) / 3, (l + r + r) / 3
        s1, s2 = _internal_solve(p), _internal_solve(q)
        if s1 < s2:
            r = q
        else:
            l = p

    return _internal_solve((l + r) / 2)


img = cv.bilateralFilter(img, 10, 128, 50)
dxdy = cv.Canny(img, 100, 400)
cv.imshow("img", dxdy)
# cv.waitKey(0)
# res = cv.HoughLinesP(dxdy, 1, np.pi / 180, 10, minLineLength=5, maxLineGap=20)
res = cv.HoughLines(dxdy, 1, np.pi / 180, 150, None, 0, 0)
# print(res)

res_edge = np.zeros((1000, 1000), dtype=np.uint8)
for_kmean = []

# for arr in res:
#     a, b, c, d = arr[0]
#     theta = atan2(d - b, c - a)
#     rho = a * np.cos(theta) + b * np.sin(theta)
#     for_kmean.append([(rho, theta)])
#     cv.line(res_edge, (a, b), (c, d), (255, 255, 255), 1, cv.LINE_AA)

for line in res:
    length = 5000
    for rho, theta in line:
        for_kmean.append((rho, theta * 360 / np.pi))
        a = np.cos(theta)
        b = np.sin(theta)
        x0 = a * rho
        y0 = b * rho
        x1 = int(x0 + length * (-b))
        y1 = int(y0 + length * (a))
        x2 = int(x0 - length * (-b))
        y2 = int(y0 - length * (a))
        cv.line(res_edge, (x1, y1), (x2, y2), (255, 255, 255), 1, cv.LINE_AA)

# cv.imshow("img", dxdy)
# cv.waitKey(0)
# cv.imshow("img", res_edge)
# cv.waitKey(0)

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
    attempts = kwargs.get('attempts', 10)

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
    if abs(np.linalg.det(A)) < 10 ** -5:
        if np.abs(rho1 - rho2) <= 5: return (0, 0)
        else: return (-100, -100)
    b = np.array([[rho1], [rho2]])
    x0, y0 = np.linalg.solve(A, b)
    x0, y0 = int(np.round(x0)), int(np.round(y0))
    return (x0, y0)


def segmented_intersections(lines):
    """Finds the intersections between groups of lines."""

    intersections = []
    for i, group in enumerate(lines[:-1]):
        for next_group in lines[i+1:]:
            for line1 in group:
                for line2 in next_group:
                    intersections.append(intersection(line1, line2))

    return intersections


res_edge = np.zeros((1000, 1000, 3), dtype=np.uint8)
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
            print(x, y)
            if -30 <= x <= 1030 and -30 <= y <= 1030:
                vis[j] = 1
        curr.append(segment[i])
    for line in curr:
        length = 5000
        for rho, theta in line:
            for_kmean.append((rho, theta * 360 / np.pi))
            a = np.cos(theta)
            b = np.sin(theta)
            x0 = a * rho
            y0 = b * rho
            x1 = int(x0 + length * (-b))
            y1 = int(y0 + length * (a))
            x2 = int(x0 - length * (-b))
            y2 = int(y0 - length * (a))
            cv.line(res_edge, (x1, y1), (x2, y2), (255, 255, 255), 1, cv.LINE_AA)
    final_segmented.append(curr)

intersections = segmented_intersections(final_segmented)

# final_intersections = []
# vis = [0 for i in range(len(intersections))]
# res_edge = cv.cvtColor(res_edge, cv.COLOR_GRAY2BGR)
# for i in range(len(intersections)):
#     if vis[i]: continue
#     for j in range(i + 1, len(intersections)):
#         if vis[j]: continue
#         if get_dist(intersections[i][0], intersections[i][1], intersections[j][0], intersections[j][1]) <= 15:
#             vis[j] = 1
#             continue
#     final_intersections.append(intersections[i])
final_intersections = intersections
for i in final_intersections:
    cv.circle(res_edge, i,  0, (0, 0, 255), 5)

cv.imshow("img", res_edge)
cv.waitKey(0)

# for i in range(1000):
#     for j in range(1000):
#         if res_edge[i][j] == 0: continue
#         cnt = 0
#         for dx in range(-5, 6):
#             for dy in range(-5, 6):
#                 cnt += dxdy[max(0, min(999, i + dx))][max(0, min(999, j + dy))] != 0
#         if cnt == 0: res_edge[i][j] = 0


# res_edge = cv.cvtColor(searched_edge, cv.COLOR_BGR2GRAY)

# dxdy = cv.Canny(img, 50, 400)
# res = cv.HoughLinesP(dxdy, 1, np.pi / 180, 25, minLineLength=3, maxLineGap=20)
# nxt_edge = np.zeros(img.shape, dtype=np.uint8)
#
# for arr in res:
#     a, b, c, d = arr[0]
#     cv.line(nxt_edge, (a, b), (c, d), (255, 255, 255), 1, cv.LINE_AA)
# res_edge = nxt_edge
#
# cv.imshow("img", res_edge)
# cv.waitKey(0)
# dxdy = cv.Canny(res_edge, 250, 500)
# res = cv.HoughLinesP(dxdy, 1, np.pi / 180, 25)
# res_edge = cv.cvtColor(searched_edge, cv.COLOR_GRAY2BGR)

lines = []
for i in range(2500):
    tmp = []
    for j in range(360):
        tmp.append([])
    lines.append(tmp)

s1, s2 = [], []
for line in res:
    a, b, c, d = line[0]

    theta0 = np.arctan2(c - a, d - b)
    roh = a * np.cos(theta0) + b * np.sin(theta0)

    # theta1 = np.arctan2(d - b, c - a)

    lines[floor(roh + 1000)][floor(theta0 * 180 / np.pi)].append((a, b, c, d))
    s1.append(floor(roh))
    s2.append(floor(theta0 * 180 / np.pi))
    # cnt[floor(roh + 1000) // 50][floor(theta0) // 5].append((a, b, c, d))

    # a, b = (a + c) / 2, (b + d) / 2
    #
    # alpha = (mid + sqrt(pow(a - c, 2) + pow(b - d, 2))) * 1.5
    # w, x, y, z = a - alpha * np.cos(theta), b - alpha * np.sin(theta), a + alpha * np.cos(theta), b + alpha * np.sin(theta)
    # w, x, y, z = int(w), int(x), int(y), int(z)

    # for i in range(-alpha, alpha, 0.5):
    #     print(i)
    # cv.line(res_edge, (w, x), (y, z), (0, 0, 255), 1, cv.LINE_AA)

# plt.xlim(0, 2500)
# plt.ylim(0, 360)
# plt.scatter(s1, s2, s=[1 for i in range(len(s1))])
# plt.savefig('distribution.png', dpi=1000)

# for i in range(2500):
#     for j in range(360):
#         if len(cnt[i][j]) == 0:
#             continue
#         cnt[i][j] = sorted(cnt[i][j])
#         groups = []
#         curr = [cnt[i][j][0]]
#         a, b, c, d = cnt[i][j][0]
#         for k in range(1, len(cnt[i][j])):
#             w, x, y, z = cnt[i][j][k]
#             if get_dist_between_two_skew_line_in_polar_coord((a, b, c, d), (w, x, y, z)) <= 5:
#             # if pow(w - c, 2) + pow(x - d, 2) <= 100:
#                 curr.append((w, x, y, z))
#             else:
#                 groups.append(curr)
#                 curr = [(w, x, y, z)]
#             a, b, c, d = w, x, y, z
#         if len(curr) > 0:
#             groups.append(curr)
#         # print(groups)
#         for group in groups:
#             sum = 0
#             for a, b, c, d in group:
#                 sum += get_dist(a, b, c, d)
#             if sum <= 5:
#                 break
#             a, b, c, d = group[0]
#             w, x, y, z = group[len(group) - 1]
#             dist = pow(a - y, 2) + pow(b - z, 2)
#             if dist >= 1:
#                 cv.line(res_edge, (a, b), (y, z), (0, 0, 255), 1, cv.LINE_AA)
#             # print(len(group))
#         # if len(cnt[i][j]) >= 1:
#         #     print(i, j, len(cnt[i][j]))
#         #     for a, b, c, d in cnt[i][j]:
#         #         cv.line(res_edge, (a, b), (c, d), (0, 0, 255), 1, cv.LINE_AA)
# # print(l)

# clustering lines using 2d prefix sum
sum = []
for i in range(2501):
    tmp = []
    for j in range(361):
        tmp.append(0)
    sum.append(tmp)

for i in range(1, 2501):
    for j in range(1, 361):
        sum[i][j] = sum[i - 1][j] + sum[i][j - 1] - sum[i - 1][j - 1] + len(lines[i - 1][j - 1])


def get_sum(a, b, c, d):
    a, b, c, d = int(a), int(b), int(c), int(d)
    c = min(c, 2500)
    d = min(d, 360)
    return sum[c][d] - sum[a - 1][d] - sum[c][b - 1] + sum[a - 1][b - 1]


vis = []
for i in range(2501):
    tmp = []
    for j in range(361):
        tmp.append(0)
    vis.append(tmp)

for i in range(1, 2501):
    for j in range(1, 361):
        if vis[i][j] == 1: continue
        l, r = 0, 100


        def __internal_solve(mid):
            return get_sum(i - 1, j - 1, i + mid // 10, j + mid) - floor(mid // 2)


        while l + 2 < r:
            p, q = (l + l + r) / 3, (l + r + r) / 3
            if __internal_solve(p) < __internal_solve(q):
                l = p
            else:
                r = q
        l = floor((l + r) // 2)
        expected = []
        for k in range(l + 1):
            for u in range(l // 10 + 1):
                vis[min(2500, i + k)][min(360, j + u)] = 1
                for x in lines[min(2500, i + k) - 1][min(360, j + u) - 1]:
                    expected.append(x)
        if len(expected) <= 0: continue
        expected = sorted(expected)
        groups = []
        curr = [expected[0]]
        a, b, c, d = expected[0]
        for k in range(1, len(expected)):
            w, x, y, z = expected[k]
            if get_dist_between_two_skew_line_in_xy_coord((a, b, c, d), (w, x, y, z)) <= 50:
                curr.append((w, x, y, z))
            else:
                groups.append(curr)
                curr = [(w, x, y, z)]
            a, b, c, d = w, x, y, z
        if len(curr) > 0:
            groups.append(curr)
        # print(groups)
        for group in groups:
            dist_sum = 0
            theta_sum = 0
            xs, ys = 0, 0
            thetas = []
            for a, b, c, d in group:
                curr_dist = get_dist(a, b, c, d)
                dist_sum += curr_dist
                theta_sum += atan2(b - d, a - c)
                xs += (a + c) * curr_dist / 2
                ys += (b + d) * curr_dist / 2
                thetas.append(atan2(b - d, a - c))
            if dist_sum <= 5:
                break
            a, b, c, d = group[0]
            w, x, y, z = group[len(group) - 1]
            alpha = dist_sum / 2

            mx, my = xs / dist_sum, ys / dist_sum
            mx, my = int(mx), int(my)
            # theta_avg = theta_sum / len(groups)
            theta_avg = np.median(thetas)
            if alpha >= 30:
                cv.line(res_edge, (int(mx + alpha * np.cos(theta_avg)), int(my + alpha * np.sin(theta_avg))),
                        (int(mx - alpha * np.cos(theta_avg)), int(my - alpha * np.sin(theta_avg))), (0, 0, 255), 1,
                        cv.LINE_AA)
            # print(len(group))


def on_click(event, x, y, flag, param):
    if event == cv.EVENT_LBUTTONDOWN:
        print(x, y)


cv.imshow("img", res_edge)
cv.setMouseCallback("img", on_click)

cv.waitKey(0)
cv.destroyAllWindows()
