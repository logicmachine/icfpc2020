#!/usr/bin/env python

import sys
import os
import re
import numpy as np

def main(args):
    data = args[1]
    data = data.replace("(", "").replace(")", "").replace("nil", "").replace("|", "").replace(",", " ").strip()
    data = list(map(int, data.split()))
    pos = []
    for i in range(0, len(data) - 1, 2):
        pos.append((data[i], data[i+1]))
    pos = np.array(pos)
    minr = min(pos[:,0])
    minc = min(pos[:,1])
    maxr = max(pos[:,0])
    maxc = max(pos[:,1])
    print("min: {}, {}".format(minr, minc))
    print("max: {}, {}".format(maxr, maxc))
    print("center: {}, {}".format((minr+maxr)/2, (minc+maxc)/2))
    """
    rows = []
    cols = []
    for row in pos:
        rows.append(row[0])
        cols.append(row[1])
    print(rows)
    print(cols)
    """
    pos -= np.array([minr, minc])
    post = []
    for row in pos:
        post.append(tuple(row))
    print(post)
    for j in range(max(pos[:,1]+1)):
        for i in range(max(pos[:,0]+1)):
            print("■" if (i, j) in post else "　", end="")
        print()


if __name__ == "__main__":
    main(sys.argv)

"""
ap ap ap interact galaxy :state ap ap cons 0 0

(0, ((1, ((4, nil), (0, (nil, nil)))), ((((8, 10), ((8, 9), ((8, 8), ((8, 7), ((8, 6), ((8, 5), ((8, 4), ((8, 3), ((8, 2), ((8, 1), ((8, 0), ((8, -1), ((8, -2), ((8, -3), ((8, -4), ((8, -5), ((16, 4), ((15, 4), ((14, 4), ((13, 4), ((12, 4), ((11, 4), ((10, 4), ((9, 4), ((8, 4), ((7, 4), ((6, 4), ((5, 4), ((4, 4), ((3, 4), ((2, 4), ((1, 4), nil)))))))))))))))))))))))))))))))), nil), nil)))

(0, ((1, ((2, nil), (0, (nil, nil)))), ((((0, -3), ((-1, -3), ((-2, -3), ((-3, -3), ((0, 0), ((-1, 0), ((-2, 0), ((-3, 0), ((-3, 0), ((-3, -1), ((-3, -2), ((-3, -3), ((0, -1), ((0, -2), ((0, -3), nil))))))))))))))), (((1, -3), ((2, -3), ((3, -3), ((3, -2), ((3, -1), ((1, 0), ((2, 0), ((3, 0), nil)))))))), (((0, 1), ((3, 1), ((0, 2), ((3, 2), ((0, 3), ((1, 3), ((2, 3), ((3, 3), nil)))))))), (((-3, 1), ((-3, 2), ((-3, 3), ((-2, 3), ((-1, 3), nil))))), nil)))), nil)))

(0, ((0, ((0, nil), (0, (nil, nil)))), ((((-1, -3), ((0, -3), ((1, -3), ((2, -2), ((-2, -1), ((-1, -1), ((0, -1), ((3, -1), ((-3, 0), ((-1, 0), ((1, 0), ((3, 0), ((-3, 1), ((0, 1), ((1, 1), ((2, 1), ((-2, 2), ((-1, 3), ((0, 3), ((1, 3), nil)))))))))))))))))))), (((-7, -3), ((-8, -2), nil)), (nil, nil))), nil)))
"""
