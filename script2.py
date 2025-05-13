"""
This is a simple script that demonstrates the use of try-except-finally blocks in Python."""

import math as m

x = 10
y = 8
try:
    if x > y:
        print("x is greater than y")
    elif x < y:
        print("x is less than y")
    else:
        print("x is equal to y")
except Exception as e:
    print("error")
finally:
    print("done")
m.sqrt(16)
mystr = "Hello"