   10 DATA 1,2,3
   20 PRINT "Starting..."
   30 DATA "some","more","data"
   40 ' Test empty DATA skip    
   50 DATA : DATA 
   60 DATA "done"
   70 PRINT "End of data setup..."
   80 RESTORE 10
   90 PRINT "Data line: ",$data_line
  100 WHILE havedata()
  110     old_data_line = $data_line
  120     READ a
  130     IF $data_line <> old_data_line THEN 
  140         PRINT "New data line: ",$data_line
  150     FI 
  160     PRINT a
  170 WEND 
  180 PRINT "---"
