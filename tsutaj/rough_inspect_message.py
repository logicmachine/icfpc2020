"""
## galaxy.txt を雑に解析

- わかったこと
  - 行数と '=' の数が一致
  - ドキュメントですでに定義されているものしか使われてない
  - インタプリタがあれば galaxy の定義を解釈可能
"""

import pathlib
import requests

def is_number(s):
    return s.isdigit()

def is_colon_number(s):
    return s[1:].isdigit()

def main():
    if pathlib.Path("galaxy.txt").exists():
        print("[!] open local file.")
        with open("galaxy.txt", "r") as f:
            lines = f.readlines()
    else:
        print("[!] download from internet.")
        url = "https://message-from-space.readthedocs.io/en/latest/_downloads/48286a0b4dac94efc1b23fb2c3a41bef/galaxy.txt"
        response = requests.get(url)
        lines = response.content.decode().splitlines()

    print("# of lines", len(lines))
    cnt_number = 0
    cnt_colon_number = 0
    others = {}
    for line in lines:
        line = line.rstrip()
        elems = line.split()
        for elem in elems:
            if is_number(elem):
                cnt_number += 1
            elif is_colon_number(elem):
                cnt_colon_number += 1
            else:
                others.setdefault(elem, 0)
                others[elem] += 1

    print("cnt_number", cnt_number)
    print("cnt_colon_number", cnt_colon_number)
    for k, v in others.items():
        print(k, v)

if __name__ == "__main__":
    main()
