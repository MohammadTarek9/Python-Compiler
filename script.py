"""
A multi-line docstring at the top of the file
"""
myTup = (1,2)
#ERROR 1
#tc1, tc11 = 1
#ERROR 3
# tc3 = 1 # extra space
import math
from math import sqrt
#ERROR 10
#from math import 
x = 1 # a global variable
class MyClass:
    # This is a class-level comment
    def __init__(self, name):
        """Constructor docstring"""
        self.name = name

    def greet(self):
        # Greet method
        print("hello" + self.name)
        def inner_function():
            print("This is an inner function")
#ERROR 6
#def error_function():


def myFunction(x, y=10):
    """
    A function docstring
    """
    str1 = "Hello \" World \"  Next line"
    "hi"
    #ERROR 7
    #tc7 = 
    myList = [1, 2, 3]
    list2 = [4, 5, 6]
    list3 = myList + list2
    wqt = 5
    if wqt not in myList:
        print("not in list")
    if myList is not None:
        print("myList is not None")
    fff = "he"
    fff2 = "llo"
    #ERROR 4
    #tc4 = 01
    fff3 = fff + fff2
    wrt = 10
    #ERROR 5
    #tc5 = 1.2.78
    qq = wqt + wrt 
    print(list3)
    mychar = 'a'
    for i in myList:
        print(i)
    mySet = {4, 5, 6}
    emptyTuple = ()
    emptyList = []
    
    myDict = {7: "seven", 8: "eight", 9: "nine"}
    myTuple = (0, 1, 2)
    myTuple2 = (12+14) # not handled as a tuple
    expr3 = (12, 14) # this is a tuple
    #ERROR 9
    #wrong_tuple = (12, 14
    names = ["hey", "hello", "hi"]
    newSet = {":", ":(", ":D"} # should be a set not a dictionary
    # do something
    total = x + y
    if total > 100:
        return True
    elif total < 100:
        return False
    else:
         return None
wqt = 10

def myFunction2(x, y=10):
    x = 1

try:
    # This is a try block
    result = myFunction(5, 10)
except Exception as e:
    # This is an exception block
    print("An error occurred:", e)

try:
    print("This is a try block")
finally:
    print("This is a finally block")

#ERROR 8 (try without except or finally)
# try:
#     tc7 = 2/5


def main():
    """Main function docstring"""
    obj = MyClass("Moh")
    obj.greet()
    result = myFunction(50, 20)
    x  = math.sqrt(16) # This is a comment
    x+= 1 # This is another comment
    test = True
    test2 = False
    test3 = test | test2
    myyy = 1
    y = myyy << 2 # This is a bitwise left shift operation
    z = y >> 1 # This is a bitwise right shift operation
    q = ~y
    negNum = -1
    r = 20.5
    r**= 2.5
    wqt = 5
    wrt = 10
    qq = wqt + wrt
    aaa, bbb = 1, 2
    num = 101
    rrrr,sss="hey","hello"
    www = rrrr + sss
    tttttt = 4
    rrrrrr = 5
    # ERROR 2
    # unterminated_string = "This is an unterminated string
    # unterminated_string_single = 'This is an unterminated string
    qqqqqq,pppppp = tttttt, rrrrrr
    alpha,beta,gamma = 5,6,7
    qwqwqw, rrr = [1, 2], [3, 4]
    print(y)
    print("Hello World")
    print(result)
if __name__ == "__main__":
    main()
