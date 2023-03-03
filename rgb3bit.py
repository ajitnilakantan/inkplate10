# Get a reasonable palette for the Inkplate10
# use #define RGB3BIT(r, g, b) ((54UL * (r) + 183UL * (g) + 19UL * (b)) >> 13)
# from the Inkplate Arduino library

from itertools import product

max = [0]*8
min = [256]*8
for r, g, b in product(range(256), range(256), range(256)):
    if r == g == b:
        bit3 = ((54 * (r) + 183 * (g) + 19 * (b)) >> 13)
        if max[bit3] < r:
            max[bit3] = r
        if min[bit3] > r:
            min[bit3] = r

equ = [int(i/7*255) for i in range(8)]
mid = [(min[i]+max[i])//2 for i in range(8)]
print(f"max={max}\nmin={min}\nmid={mid}\nequ={equ}")
