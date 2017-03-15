10 rem *** simulation of matrix addition
20 dim a(5,3),b(5,3),c(5,3)
30 print "a array contents:" : print 
40 for x = 1 to 5 : for y = 1 to 3 : read a(x,y) : print a(x,y),  : next y : print  : next x : print  : print 
50 print "b array contents:" : print 
60 for x = 1 to 5 : for y = 1 to 3 : read b(x,y) : print b(x,y),  : next y : print  : next x : print  : print 
70 print "c array contents:" : print 
80 for x = 1 to 5 : for y = 1 to 3 : c(x,y) = a(x,y) + b(x,y) : print c(x,y),  : next y : print  : next x : print  : print 
90 data 7,8,9,3,4,1,6,8,2,1,4,1,8,-10,2
100 data 3,5,8,2,4,7,1,3,1,8,4,9,6,6,6
