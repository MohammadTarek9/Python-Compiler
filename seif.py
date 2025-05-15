class Car:

    def __init__(self, name):
        self.name = name

    def printname(self):
        print(self.name)

x = 1 if True else 0
print("yes" if True else "no")
print("yes") if False else print("no")
y = 20
y = 10 < (x if True else y)
my_list = [1, 2, 3 if False else 4]
car1 = Car("Honda")
car1.printname()
mySet = {4, 5, 6}


class MyClass:
    # This is a class-level comment
    def __init__(self, name):
        """Constructor docstring"""
        self.name = name

    def greet(self):
        # Greet method
        print("hello" + self.name)