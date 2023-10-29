$ Program that uses a wrong paramater in a function call

real x
real a(2), b(2)
character c

x = 3 + t(a, b, c)

end

real function t(real n(2), m(2) , integer x)
real e, f(10), g(10)
logical k(10)
complex l(12)
data e/4.0/
data k/.TRUE., .FALSE., .TRUE./
data l/ (0.0:-4.1), (0.01:-3.9)/

t = x ** (n(0)*m(1))
end