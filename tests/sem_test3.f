$ Program that uses an undeclared variable as array index

real a(5,4,2)
integer b

data a/1.1, 2.2, 3.3/, b/2/

a(2,b,c) = 1.1

end