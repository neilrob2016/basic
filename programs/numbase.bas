   10 ' Prints out number bases  
   20 DATA "0b101","077","123","456.78","0xFF"
   30 RESTORE 20
   40 DIM str
   50 LOOP 5
   60     READ str
   70     PRINT "Str = ",rpad$(str," ",10),", base = ",numstrbase(str)
   80 LEND 
