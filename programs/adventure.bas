   10 ' 
   20 ' The basics of a simple adventure game.                 
   30 ' This requires at least BASIC 1.1.1 otherwise you will get a DATA      
   40 ' exhausted error.                 
   50 ' 
   60 DIM NUM_ROOMS = 4
   70 DIM NUM_OBJECTS = 3
   80 DIM NUM_DIRS = 8
   90 DIM DESC_END = "*"
  100 DIM INVEN_ROOM = -1
  110 DIM room_text(NUM_ROOMS)
  120 DIM room_dir(NUM_ROOMS,NUM_DIRS)
  130 DIM dir_num_to_name(NUM_DIRS)
  140 DIM dir_num_to_abbrv(NUM_DIRS)
  150 DIM obj_room(NUM_OBJECTS)
  160 DIM TYPE_ITEM = 1
  170 DIM TYPE_NPC = 2
  180 DIM obj_name(NUM_OBJECTS)
  190 DIM obj_desc(NUM_OBJECTS)
  200 DIM obj_type(NUM_OBJECTS)
  210 DIM d,a,i,r
  220 DIM current_room
  230 DIM npc_room
  240 DIM new_room
  250 DIM move_npc
  260 DIM userinput
  270 DIM command
  280 DIM item
  290 ' 
  300 ' Set up direction name maps                
  310 ' 
  320 DATA "north","n","south","s","east","e","west","w"
  330 DATA "up","u","down","d","in","i","out","o"
  340 RESTORE 320
  350 FOR i = 1 TO NUM_DIRS
  360     READ d,a
  370     dir_num_to_name(i) = d
  380     dir_num_to_abbrv(i) = a
  390 NEXT 
  400 ' 
  410 ' ************** ROOM DATA *****************              
  420 ' 
  430 ' Room 1                     
  440 DATA "You are standing in a clearing in a small wood." + chr$(10)
  450 DATA "You are surrounded by large ominous looking trees.",DESC_END
  460 ' Direction ordering is: N,S,E,W,U,D,I,O                    
  470 DATA 2,0,3,0,0,0,0,0
  480 ' 
  490 ' Room 2                     
  500 DATA "You are within the trees. It is very dark here and you can hear" + chr$(10)
  510 DATA "strange sounds coming from up in the trees!",DESC_END
  520 DATA 0,1,0,0,0,0,0,0
  530 ' 
  540 ' Room 3                    
  550 DATA "You are standing under a large oak tree. Sunlight scatters on" + chr$(10)
  560 DATA "the ground around you. Small animals scamper around and birds dart about" + chr$(10)
  570 DATA "above you. There is a hut here you can enter.",DESC_END
  580 DATA 0,0,0,1,0,0,4,0
  590 ' 
  600 ' Room 4   
  610 DATA "You are in a dark, smokey hut. You wonder what fiendish rituals have" + chr$(10)
  620 DATA "happened in this small place",DESC_END
  630 DATA 0,0,0,0,0,0,0,3
  640 ' 
  650 ' Read in room data                    
  660 RESTORE 440
  670 FOR r = 1 TO NUM_ROOMS
  680     ' Read room description                             
  690     READ d
  700     room_text(r) = ""
  710     WHILE d <> DESC_END
  720         room_text(r) = room_text(r) + d
  730         READ d
  740     WEND 
  750     ' Read room directions                             
  760     FOR i = 1 TO NUM_DIRS
  770         READ d
  780         room_dir(r,i) = d
  790     NEXT 
  800 NEXT 
  810 ' 
  820 ' **************** OBJECT DATA ******************               
  830 DATA "Dog",TYPE_NPC,1
  840 DATA "The scruffy dog is a small brown mongrel with a happy face"
  850 ' 
  860 DATA "Squirrel",TYPE_NPC,2
  870 DATA "The squirrel is small and furry and nibbles on some nuts"
  880 ' 
  890 DATA "sword",TYPE_ITEM,2
  900 DATA "The sword is a gleaming finely honed blade."
  910 ' 
  920 ' Read in object data             
  930 RESTORE 830
  940 FOR i = 1 TO NUM_OBJECTS
  950     READ obj_name(i),obj_type(i),obj_room(i)
  960     READ obj_desc(i)
  970 NEXT 
  980 ' 
  990 ' Main loop                           
 1000 ' 
 1010 SEED time()
 1020 current_room = 1
 1030 GOSUB 1510
 1040 WHILE 1
 1050     ' Call object routines        
 1060     GOSUB 1750
 1070     PRINT ">";
 1080     INPUT userinput
 1090     userinput = lower$(strip$(userinput))
 1100     IF userinput = "" THEN GOTO 1070 FI 
 1110     command = element$(userinput,1)
 1120     CHOOSE command
 1130         CASE "exit"
 1140         STOP 
 1150         ' 
 1160         CASE "look"
 1170         GOSUB 1510: CONTLOOP 
 1180         CONTLOOP 
 1190         ' 
 1200         CASE "get"
 1210         CASE "take"
 1220         GOSUB 2400: CONTLOOP 
 1230         ' 
 1240         CASE "drop"
 1250         GOSUB 2610: CONTLOOP 
 1260         ' 
 1270         CASE "inven"
 1280         GOSUB 2770: CONTLOOP 
 1290         ' 
 1300         CASE "examine"
 1310         GOSUB 2930: CONTLOOP 
 1320         ' 
 1330         DEFAULT 
 1340         ' See if we have a valid direction                      
 1350         FOR i = 1 TO NUM_DIRS
 1360             IF command = dir_num_to_name(i) OR command = dir_num_to_abbrv(i) THEN 
 1370                 new_room = room_dir(current_room,i)
 1380                 IF new_room = 0 THEN 
 1390                     PRINT "You cannot go that way"
 1400                     GOTO 1070
 1410                 FI 
 1420                 PRINT "You go: ",dir_num_to_name(i)
 1430                 current_room = new_room
 1440                 GOSUB 1540
 1450                 GOTO 1070
 1460             FI 
 1470         NEXT 
 1480         PRINT "Unknown command"
 1490     CHOSEN 
 1500 WEND 
 1510 ' 
 1520 ' Print room description                        
 1530 ' 
 1540 PRINT room_text(current_room)
 1550 PRINT 
 1560 PRINT "You can go: ";
 1570 FOR i = 1 TO NUM_DIRS
 1580     IF room_dir(current_room,i) THEN 
 1590         PRINT dir_num_to_name(i),"  ";
 1600     FI 
 1610 NEXT 
 1620 PRINT 
 1630 PRINT 
 1640 FOR i = 1 TO NUM_OBJECTS
 1650     IF obj_room(i) = current_room THEN 
 1660         PRINT obj_name(i)
 1670     FI 
 1680 NEXT 
 1690 PRINT 
 1700 RETURN 
 1710 ' 
 1720 ' Run object routines. This is only done when the user has input         
 1730 ' something, not on a timer. Which is possible in BASIC but complex.        
 1740 ' 
 1750 FOR i = 1 TO NUM_OBJECTS
 1760     IF obj_room(i) <> INVEN_ROOM THEN 
 1770         CHOOSE lower$(obj_name(i))
 1780             CASE "scruffy dog": GOSUB 1860: BREAK 
 1790             CASE "squirrel": GOSUB 2020: BREAK 
 1800             CASE "sword": GOSUB 2160: BREAK 
 1810         CHOSEN 
 1820     FI 
 1830 NEXT 
 1840 RETURN 
 1850 ' 
 1860 ' Scruffy dog        
 1870 CHOOSE random(10)
 1880     CASE 1
 1890     CASE 0
 1900     PRINT "Scruffy dog barks": RETURN 
 1910     ' 
 1920     CASE 2
 1930     PRINT "Scruffy dog licks his balls": RETURN 
 1940     ' 
 1950     CASE 4
 1960     CASE 5
 1970     CASE 6
 1980     move_npc = i
 1990     GOSUB 2200
 2000     RETURN 
 2010 CHOSEN 
 2020 ' 
 2030 ' Squirrel        
 2040 CHOOSE random(10)
 2050     CASE 0
 2060     PRINT "Squirrel twitches his nose": RETURN 
 2070     ' 
 2080     CASE 1
 2090     PRINT "Squirrel nibbles some nuts": RETURN 
 2100     ' 
 2110     DEFAULT 
 2120     move_npc = i
 2130     GOSUB 2200
 2140     RETURN 
 2150 CHOSEN 
 2160 ' 
 2170 ' Sword does nothing       
 2180 RETURN 
 2190 ' 
 2200 ' Move an NPC      
 2210 npc_room = obj_room(move_npc)
 2220 d = floor(rand() * NUM_DIRS) + 1
 2230 ' Loop until we find a valid direction out of the room        
 2240 WHILE 1
 2250     IF room_dir(npc_room,d) <> 0 THEN 
 2260         new_room = room_dir(npc_room,d)
 2270         IF npc_room = current_room THEN 
 2280             PRINT obj_name(i)," goes ",dir_num_to_name(d)
 2290         ELSE IF new_room = current_room THEN 
 2300                 PRINT obj_name(i)," enters"
 2310             FI 
 2320         FI 
 2330         obj_room(i) = new_room
 2340         RETURN 
 2350     FI 
 2360     d = d + 1
 2370     IF d > NUM_DIRS THEN d = 1 FI 
 2380 WEND 
 2390 ' 
 2400 ' GET command      
 2410 item = lower$(element$(userinput,2))
 2420 IF item = "" THEN 
 2430     PRINT "Take what?"
 2440     RETURN 
 2450 FI 
 2460 FOR i = 1 TO NUM_OBJECTS
 2470     IF obj_room(i) = current_room AND lower$(obj_name(i)) = item THEN 
 2480         IF obj_type(i) = TYPE_ITEM THEN 
 2490             PRINT "You take ",item
 2500             obj_room(i) = INVEN_ROOM
 2510             RETURN 
 2520         ELSE 
 2530             PRINT "You cannot take the ",item
 2540             RETURN 
 2550         FI 
 2560     FI 
 2570 NEXT 
 2580 PRINT "There is no ",item," here"
 2590 RETURN 
 2600 ' 
 2610 ' DROP command      
 2620 item = lower$(element$(userinput,2))
 2630 IF item = "" THEN 
 2640     PRINT "Drop what?"
 2650     RETURN 
 2660 FI 
 2670 FOR i = 1 TO NUM_OBJECTS
 2680     IF obj_room(i) = INVEN_ROOM AND lower$(obj_name(i)) = item THEN 
 2690         PRINT "You drop the ",item
 2700         obj_room(i) = current_room
 2710         RETURN 
 2720     FI 
 2730 NEXT 
 2740 PRINT "You do not have the ",item
 2750 RETURN 
 2760 ' 
 2770 ' INVEN command      
 2780 d = 1
 2790 FOR i = 1 TO NUM_OBJECTS
 2800     IF obj_room(i) = INVEN_ROOM THEN 
 2810         IF d THEN 
 2820             PRINT "You are carrying:"
 2830             d = 0
 2840         FI 
 2850         PRINT "   ",obj_name(i)
 2860     FI 
 2870 NEXT 
 2880 IF d THEN 
 2890     PRINT "You not carrying anything"
 2900 FI 
 2910 RETURN 
 2920 ' 
 2930 ' EXAMINE command     
 2940 item = lower$(element$(userinput,2))
 2950 IF item = "" THEN 
 2960     PRINT "Examine what?"
 2970     RETURN 
 2980 FI 
 2990 FOR i = 1 TO NUM_OBJECTS
 3000     IF (obj_room(i) = current_room OR obj_room(i) = INVEN_ROOM) AND lower$(obj_name(i)) = item THEN 
 3010         PRINT obj_desc(i)
 3020         RETURN 
 3030     FI 
 3040 NEXT 
 3050 PRINT "There is no ",item," here or held by you"
 3060 RETURN 
