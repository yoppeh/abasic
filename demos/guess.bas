100 rem *** this is a simple computer game
110 let x = int(100 * rnd(1)) + 1
120 print
130 print "I'm thinking of a number from 1 to 100."
140 print "Guess my number!!!"
150 print : print "Your guess: " ; : input g
160 if g < x then print "Try a bigger number." : goto 150
170 if g > x then print "Try a smaller number." : goto 150
180 if g = x then print "That's it!!! You guessed my number."

