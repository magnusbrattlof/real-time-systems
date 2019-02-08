import math

A = 440
freq = []
periods = []

for index in range(-10, 15):
    f = round(A*math.pow(2, index/12))
    freq.append(f)
    periods.append(round(1/(2*f)*math.pow(10, 6)))

print("Freq:\tPeriod:")
for fr, pe in zip(freq, periods):
    print("{} Hz\t{} \u03BCs".format(fr, pe))

