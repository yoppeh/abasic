10 rem *** stars game
20 n = int(100 * rnd(1)) + 1
30 print "Enter your guess (-1 to quit): ";  : input g
40 if g = n then print "Winner!!!" : goto 20
50 if g = -1 then goto 200
60 d = abs(g - n)
70 if d >= 64 then 190
80 if d >= 32 then 180
90 if d >= 16 then 170
100 if d >= 8 then 160
110 if d >= 4 then 150
120 if d >= 2 then 140
130 print "*"; 
140 print "*"; 
150 print "*"; 
160 print "*"; 
170 print "*"; 
180 print "*"; 
190 print "*" : goto 30
200 print "Quitter!!! Answer was "; n
