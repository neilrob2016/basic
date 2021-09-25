   10 ON ERROR GOTO 310
   20 DIM dirname,fd,file,cnt
   30 DIM statstr
   40 ' 
   50 WHILE 1
   60     PRINT "Enter dir>";: INPUT dirname
   70     fd = opendir(dirname)
   80     IF NOT fd THEN 
   90         PRINT "Can't open directory: ",syserror$($syserror)
  100         GOTO 60
  110     FI 
  120     ' 
  130     INPUT #fd,file
  140     WHILE NOT $eof
  150         PRINT cnt,": ",file
  160         statstr = lstat$(dirname + "/" + file)
  170         IF statstr = "" THEN 
  180             PRINT "   Can't stat '",file,"': ",strerror$($syserror)
  190         ELSE 
  200             PRINT "   Type : ",element$(statstr,1)
  210             PRINT "   Size : ",element$(statstr,7)," bytes"
  220             PRINT "   Owner: ",element$(statstr,4)
  230             cnt = cnt + 1
  240         FI 
  250         INPUT #fd,file
  260     WEND 
  270     ' 
  280     CLOSEDIR fd
  290 WEND 
  300 ' 
  310 PRINT "ERROR ",$error,": ",error$($error)
