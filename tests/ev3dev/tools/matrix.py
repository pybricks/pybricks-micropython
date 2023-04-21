from pybricks.tools import Matrix, vector, cross
from pybricks.parameters import Axis

# Test backwards compatibility imports
from pybricks.geometry import vector as _vector, Matrix as _Matrix, Axis as _Axis

assert Matrix == _Matrix
assert vector == _vector
assert Axis == _Axis

# Basic matrix algebra
A = Matrix(
    [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
    ]
)
B = -A
C = A - A.T

print("A =", A)
print("B = -A =", B)
print("A.T =", A.T)
print("A + B =", A + B)
print("C = A - A.T =", C)
print("C + C.T =", C + C.T)
print("A * A.T =", A * A.T)
print("A.shape =", A.shape)
print("A * 3 =", A * 3)
print("3 * A =", 3 * A)
print("cross(Axis.X * 4, Axis.Y * 3) = ", cross(Axis.X * 4, Axis.Y * 3))

# Reading elements
print("B[0, 1] =", B[0, 1])
print("B.T[0, 1] =", B.T[0, 1])

# Vector basics
b = vector(3, 4, 0)
c = b.T
print("b = vector(3, 4, 0) =", b)
print("b.shape =", b.shape)
print("abs(b) =", abs(b))
print("[v for v in b * 2] =", [v for v in b * 2])
print("c = b.T", c)
print("A * b =", A * b)

# Dealing with resulting scalar types
print("b.T * b =", b.T * b)
print("type(b.T * b) =", type(b.T * b))
print("b.T * A * b =", b.T * A * b)
print("(b.T * A * b) * A / 2 =", (b.T * A * b) * A / 2)

# Nonsquare matrices
D = Matrix(
    [
        [0, 1, 0, 2],
        [3, 0, 4, 0],
    ]
)
print("D =", D)
print("D.shape =", D.shape)
print("D.T.shape =", D.T.shape)
print("D.T * D =", D.T * D)
print("D * D.T =", D * D.T)

# Test catch of dimension errors
try:
    b - c
except ValueError:
    pass
try:
    b * A
except ValueError:
    pass

# 1x1 matrix is scalar and is converted to float.

print(type(Matrix([[0]])))

# 0-sized dimensions raise errors.

try:
    Matrix([])
except ValueError:
    print("ValueError")

try:
    Matrix([[]])
except ValueError:
    print("ValueError")

# Mismatched dimensions raise error.

try:
    Matrix([[0], [1, 2]])
except ValueError:
    print("ValueError")

# iterator
print(*B)
print(*B.T)
