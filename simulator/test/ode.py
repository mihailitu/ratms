import math
v0 = 20.0
T = 1.5
a = 1.0
b = 1.0
s0 = 1.5
delta = 4.0

l = 5.0
x = 0.0
v = 0.0

dt = 0.5

for i in range(0, 10):
  netDistance = 0 - x - 0
  deltaV = v - 0
  sStar = s0 + v*T +(v*deltaV)/(2*math.sqrt(a*b))
  acc = a * ( 1 - (v/v0)**delta) # - (sStar/netDistance)**2)
  
  x = x + v * dt + (acc * dt * dt)/2
  v = v + acc * dt
  print x, v, acc
