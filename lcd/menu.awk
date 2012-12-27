BEGIN{
# script to create the "C" include file for a menu  
#
# usage :
# cat menu.def | awk -f menu.awk | tee menu.h

table_cnt=0
str_cnt=0
inc_cnt=0

INCREMENT[":"] = -1
WIDTH = 8
}



{
gsub(" *#.*$","")
if ( $0 != "" )
   {
   fc = 1 + split($0,FIELDS,"\t")
   if ( fc != WIDTH )
      {
      print "### Unexpected number of fields in the definition file:" fc
      print "### $0"
      }
   else # valid line
      {
      table_cnt++
      TABLE[table_cnt] = $0
      ITEM_CNT[FIELDS[1]] = ITEM_CNT[FIELDS[1]]+1       # counting  the quantity of items per menu
      ITEM_ID[FIELDS[1],ITEM_CNT[FIELDS[1]]] = table_cnt      # ITEM_ID[ MENU , 4 ] = 18      -> the 4th menuitem in the MENU list is in the array at offset 18

      # menu items starting with ":" are directives for this script and not menu item
      if ( FIRST[FIELDS[1]]   == "" )   FIRST[FIELDS[1]]     = table_cnt               # Get the first of each menu group
      if ( !match(FIELDS[2],"^:"))      CALLED[FIELDS[2]]    = CALLED[FIELDS[2]] + 1   # count and list the menu item called (and identify undefined ones)
      if ( !match(FIELDS[3],"^:"))      CALLED[FIELDS[3]]    = CALLED[FIELDS[3]] + 1   # count and list the menu item called (and identify undefined ones)
      if ( INCREMENT[FIELDS[4]] == "" ) INCREMENT[FIELDS[4]] = inc_cnt++               # table for the increment values
      if ( STRINGS[FIELDS[6]] == "" )   STRINGS[FIELDS[6]]   = str_cnt++               # gather all the LCD strings
      if ( STRINGS[FIELDS[7]] == "" )   STRINGS[FIELDS[7]]   = str_cnt++               # and flush the duplicate ones
      } 
   }

}

END{

print "// This file was auto generated from menu.def and menu.awk"
print "// cat menu.def | awk -f menu.awk | tee menu.h"

for ( iii=1 ; iii <= table_cnt ; iii++ )
   {
   split(TABLE[iii],FIELDS,"\t")
   if ( RE_ITEM_CNT[FIELDS[1]] == "" )
      {
      RE_ITEM_CNT[FIELDS[1]] = "1"
      print str_pad("#define "FIELDS[1]"_COUNT ",40) ITEM_CNT[FIELDS[1]]
      print str_pad("#define "FIELDS[1]"_FIRST ",40) FIRST[FIELDS[1]]
      print str_pad("#define "FIELDS[1]"_LAST  ",40) FIRST[FIELDS[1]]+ITEM_CNT[FIELDS[1]]-1
      }
   }

for (iii in CALLED)
   {
   if ( ITEM_CNT[iii] == "" ) print "### problem, a called item was not defined: " iii " : "CALLED[iii]
   }

print "\nPROGMEM const long edit_increment[] = {" 
for ( iii=0 ; iii < inc_cnt ; iii++ )
   {
   for( jjj in INCREMENT )
      {
      ddd = str_pad(jjj,16+2*mmm)
      if ( INCREMENT[jjj] == iii ) 
         {
         if ( iii+1 < inc_cnt ) print str_pad("              " ddd ", ",50) "//  "iii
         else                   print str_pad("              " ddd "};",50) "//  "iii
         }
      }
   }


print "\nPROGMEM const char linetxt[]="
for ( iii=0 ; iii < str_cnt ; iii++ )
   {
   for( jjj in STRINGS )
      {
      tmp = substr(jjj,2,length(jjj)-2)
      mmm = split(jjj,aaa,"[\\][0-9][0-9][0-9]")-1   # for each \000 octal code, we must add 2 characters
      ddd = str_pad(tmp,16+3*mmm)
      if ( STRINGS[jjj] == iii ) 
         {
         if ( iii+1 < str_cnt ) print str_pad("              \"" ddd "\" ",50) "//  "iii
         else                   print str_pad("              \"" ddd "\";",50) "//  "iii
         }
      }
   }

## start ouputting the Menu Array..."
print ""
print "#define MENU_TABLE_WIDTH "WIDTH
print "#define MENU_TABLE_LEN   "table_cnt
print "PROGMEM const char menu_state_machine[]= {  "substr("0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,",1,2+WIDTH*2) " // STATE : 0 is not a valid state, it means 'do nothing'  > there is one more element so that the array starts at 1"
print "//            UNDO      ENTER     PREV      NEXT      INCREMENT OFFSET    LINE1     LINE2  "


UNDO[":"]      = " 0"
UNDO[":POP_1"] = "-1"
UNDO[":POP_2"] = "-2"
UNDO[":POP_3"] = "-3"

for ( iii=1 ; iii <= table_cnt ; iii++ )
   {
   split(TABLE[iii],FIELDS,"\t")
   RE_ITEM_CNT[FIELDS[1]] = RE_ITEM_CNT[FIELDS[1]]+1       # re-count the quantity of items per menu

   if ( UNDO[FIELDS[2]] != "" ) UUU = UNDO[FIELDS[2]]     # undo: do nothing, or pop the menu stack
   else                         UUU = FIRST[FIELDS[2]]    # undo: jump to the first menu item

   if ( FIELDS[3] == ":" )      EEE = "0"
   else                         EEE = FIRST[FIELDS[3]]    # enter: jump to the first menu item

   if ( ITEM_ID[FIELDS[1],1] == iii )  PREV = ITEM_ID[FIELDS[1],ITEM_CNT[FIELDS[1]]] # we are on the first of the section, the previsous one is the last
   else                                PREV = ITEM_ID[FIELDS[1],RE_ITEM_CNT[FIELDS[1]]-2]

   if ( ITEM_ID[FIELDS[1],ITEM_CNT[FIELDS[1]]] == iii ) NEXT = ITEM_ID[FIELDS[1],1] # we are on the last one, the next one is the first
   else                                                 NEXT = ITEM_ID[FIELDS[1],RE_ITEM_CNT[FIELDS[1]]+0]

   INCRE = INCREMENT[FIELDS[4]]
   OFFST = FIELDS[5]
   if ( OFFST == ":" ) OFFST = "-1"
   LINE1 = STRINGS[FIELDS[6]] 
   LINE2 = STRINGS[FIELDS[7]] 

   if ( iii < table_cnt ) vvv = ", "
   else                   vvv = "};"
   print "              "str_pad(UUU",",10)   str_pad(EEE",",10) str_pad(PREV",",10) str_pad(NEXT",",10) \
                         str_pad(INCRE",",10) str_pad(OFFST",",10) str_pad(LINE1",",10) str_pad(LINE2 vvv,10) " // State: "str_pad(iii,3)"  "str_pad(FIELDS[1],20) str_pad(FIELDS[6],25) str_pad(FIELDS[7],25)
   }
 
}

# to have pretty output...
function str_pad(string,pad)
{
return substr(string"                                                                               ",1,pad)
}
