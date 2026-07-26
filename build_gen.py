import sys
content = chr(10).join(sys.stdin.read().splitlines())
with open(sys.argv[1], "w", encoding="utf-8") as fp:
    fp.write(content)
