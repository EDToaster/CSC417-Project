from PIL import Image
import numpy as np

im = Image.open("./spiral.bmp")
indexed = np.array(im)

b = []
for l in indexed:
    for val in l:
        b.append(val)

with open("./spiral.b", "wb") as f:
    f.write(bytes(b))
    