"""
A multi-line docstring at the top of the file
"""
import math
x = 1 # a global variable
class MyClass:
    # This is a class-level comment
    def __init__(self, name):
        """Constructor docstring"""
        self.name = name

    def greet(self):
        # Greet method
        print(f"Hello, {self.name}!")

def myFunction(x, y=10):
    """
    A function docstring
    """
    str1 = "Hello \" World \"  Next line"
    "hi"

    myList = [1, 2, 3]
    for i in myList:
        print(i)
    mySet = {4, 5, 6}
    myDict = {7: "seven", 8: "eight", 9: "nine"}
    myTuple = (0, 1, 2)
    myTuple2 = (12+14)/2 # not handled as a tuple
    names = ["hey", "hello", "hi"]
    newSet = {":", ":(", ":D"} # should be a set not a dictionary
    # do something
    total = x + y
    haga_bayna_aktar = 0
    if total > 100:
        return True
    elif total < 100:
        return False
    else:
        return "Result is {}".format(total)

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
    #y = x << 2 # This is a bitwise left shift operation
    #z = y >> 1 # This is a bitwise right shift operation
    #q = ~y
    r = 20.5
    haga_bayna_aktar = 0.5
    print(f"Test3: {test3}")
    print(result)
if __name__ == "__main__":
    main()
