   10 ' Calculates the range and flight time of a ballistic projectile in 
   20 ' a vacuum 
   30 DIM vel,ang
   40 DEFEXP dist = (vel * vel * sin(2 * ang)) / 9.8
   50 DEFEXP ftime = sqrt(2) * vel / 9.8
   60 ' 
   70 WHILE 1
   80     PRINT "Velocity (m/s): ";: INPUT vel
   90     PRINT "Angle (deg): ";: INPUT ang
  100     vel = tonum(vel)
  110     ang = tonum(ang)
  120     PRINT "Distance = ",!dist,"m"
  130     PRINT "Time = ",!ftime," secs"
  140     PRINT 
  150 WEND 
