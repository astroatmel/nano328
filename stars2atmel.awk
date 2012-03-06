BEGIN{
#
# cat coords_85stars_mag3.txt coords_my_points.txt | awk -f stars2atmel.awk
#
NB = 0
NBNB = 0
STAR_NAME_LEN=10
CONSTEL_NAME_LEN=12
KNOWN["24gam Ori"]="Bellatrix "
KNOWN["19bet Ori"]="Rigel     "
KNOWN["34del Ori"]="Mintaka   "
KNOWN["46eps Ori"]="Alnilam   "
KNOWN["50zet Ori"]="Alnitak   "
KNOWN["53kap Ori"]="Ori       "
KNOWN["58alp Ori"]="Betelgeuse"
KNOWN["9alp CMa" ]="Sirius    "
KNOWN["30alp Hya"]="Alphard   "
KNOWN["50alp Cyg"]="Deneb     "
KNOWN["3alp Lyr" ]="Vega      "
KNOWN["53alp Aql"]="Altair    "

KCONST["And"]=""
KCONST["Ant"]=""
KCONST["Aps"]=""
KCONST["Aql"]="Aquila"
KCONST["Aqr"]=""
KCONST["Ara"]=""
KCONST["Ari"]=""
KCONST["Aur"]=""
KCONST["Boo"]=""
KCONST["CMa"]="Canis Major"
KCONST["CMi"]="Canis Minor"
KCONST["CVn"]=""
KCONST["Cae"]=""
KCONST["Cam"]=""
KCONST["Cap"]=""
KCONST["Car"]=""
KCONST["Cas"]="Casiopia"
KCONST["Cen"]=""
KCONST["Cep"]=""
KCONST["Cet"]=""
KCONST["Cha"]=""
KCONST["Cir"]=""
KCONST["Cnc"]=""
KCONST["Col"]=""
KCONST["Com"]=""
KCONST["CrA"]=""
KCONST["CrB"]=""
KCONST["Crt"]=""
KCONST["Cru"]=""
KCONST["Crv"]=""
KCONST["Cyg"]="Cygnus"
KCONST["Del"]=""
KCONST["Dor"]=""
KCONST["Dra"]=""
KCONST["Equ"]=""
KCONST["Eri"]=""
KCONST["For"]=""
KCONST["Gem"]="Gemini"
KCONST["Gru"]=""
KCONST["Her"]=""
KCONST["Hor"]=""
KCONST["Hya"]="Hydra"
KCONST["Hyi"]=""
KCONST["Ind"]=""
KCONST["LMi"]=""
KCONST["Lac"]=""
KCONST["Leo"]="Leo"
KCONST["Lep"]=""
KCONST["Lib"]=""
KCONST["Lup"]=""
KCONST["Lyn"]=""
KCONST["Lyr"]="Lyra"
KCONST["Mic"]=""
KCONST["Mon"]=""
KCONST["Mus"]=""
KCONST["Nor"]=""
KCONST["Oct"]=""
KCONST["Oph"]=""
KCONST["Ori"]="Orion"
KCONST["Pav"]=""
KCONST["Peg"]=""
KCONST["Per"]="Perceus"
KCONST["Phe"]=""
KCONST["Pic"]=""
KCONST["PsA"]=""
KCONST["Psc"]=""
KCONST["Pup"]=""
KCONST["Pyx"]=""
KCONST["Ret"]=""
KCONST["Scl"]=""
KCONST["Sco"]=""
KCONST["Sct"]=""
KCONST["Ser"]=""
KCONST["Sex"]=""
KCONST["Sge"]=""
KCONST["Sgr"]=""
KCONST["Tau"]="Taurus"
KCONST["Tel"]=""
KCONST["TrA"]=""
KCONST["Tri"]=""
KCONST["Tuc"]=""
KCONST["UMa"]=""
KCONST["UMi"]=""
KCONST["Vel"]=""
KCONST["Vir"]=""
KCONST["Vol"]=""
KCONST["Vul"]=""
}
# cat 85stars_mag3.txt | awk -f stars2atmel.awk 
#
# Awk script to generate an include file for the Atmel Telescope controller
# note that because we work in Fixed point, all values are positive 0-360  0-0.9999999
# for convenience, the inputs are sorted by RA value 0h -> 23h
#
#
#
# This is an example input:
#-# :set tabstop=21
#-# <TAB> separated fields
#-# http://vizier.u-strasbg.fr/viz-bin/VizieR-4
#-# Filter used : Mag < 3    DEC > -10  DEC < 50
#-# 85 stars to be used for telescope alignment (atmel controller)
#-#       RA      DEC     Name+Constel    Intensity
#-28      00 08 23.259    +29 05 25.56    21alp And       2.060
#-65      00 13 14.153    +15 11 00.95    88gam Peg       2.830
#-27      00 43 35.371    -17 59 11.78    16bet Cet       2.050     # Converted to: +342 deg 0 min 48.22 sec
#-30      01 09 43.924    +35 37 14.01    43bet And       2.080
#-53      01 54 38.409    +20 48 28.93    6bet Ari        2.660
#-34      02 03 53.953    +42 19 47.01    57gam1And       2.170
#-
{
if ( match($0,"^[^#]"))   # if not a comment ...collect data
   {
   split($0,Fields,"\t")
   Name[NB]  = Fields[4] ;  gsub(" *$","",Name[NB])
   RA[NB]    = Fields[2] 
   DE[NB]    = Fields[3] 
   SName[NB] = substr(Name[NB],1,length(Name[NB])-3) ; gsub(" *$","",SName[NB])
   CName[NB] = substr(Name[NB],length(Name[NB])-2) 
   KName[CName[NB]] = CName[NB]
   # print "NAME:"Name[NB] "  RA:"RA[NB] "  DE:"DE[NB] "  "SName[NB] " - "CName[NB]
   if ( Fields[1] != "--" ) NBNB++
   else KNOWN[NB]=Name[NB]
   NB++
   }
}

END{
CCC=0
qqq = asorti(KName)
for( iii=1 ; iii<=qqq ; iii++ ) 
   {
   # print KName[iii] " -- " KCONST[KName[iii]]
   Kid[KName[iii]]=iii
   CCC++
   }
# print "Nb of constellations:"CCC


# This is an example output:
#-
#-#define STAR_NAME_LEN 23
#-PROGMEM const char pgm_stars_name[]   =  
#-   {
#-   "Origin: Ra=0, Dec=0   \0"     /*  0   */  \
#-   "Orion:Betelgeuse (Red)\0"     /*  1   */  \
#-   "Gynus:Sadr            \0"     /*  16  */  \
#-   };
print "PROGMEM const char pgm_stars_name[]   =  // format: byte:StarID byte:ConstelationId Star Name string"
print "   {                                     // Names are for only a few prefered stars to save memory space"
count=0
for ( iii=0 ; iii<NB ; iii++)
   {
   if ( iii>=NBNB ) 
      {
      SSSN = substr(KNOWN[iii]"          ",1,STAR_NAME_LEN)
      printf("   \"\\0%03o\\0%03o%s\"   /* Coord ID:%3d  */  \\\n",iii,511,SSSN,iii)
      count++
      }
   else if ( KNOWN[Name[iii]]!="" )
      {
      SSSN = substr(KNOWN[Name[iii]]"          ",1,STAR_NAME_LEN)
      CCCN = substr(KCONST[CName[iii]]"          ",1,12)
      printf("   \"\\0%03o\\0%03o%s\"   /*  Star ID:%3d  Constellation:%s */  \\\n",iii,Kid[CName[iii]],SSSN,iii,CCCN)
      req_const[CName[iii]]=1   # Lets define strings only for required Constellations
      count++
      }
   }
print "   };   // This table uses " count*(STAR_NAME_LEN+2) " bytes..."
print "#define STAR_NAME_LEN " STAR_NAME_LEN+2
print "#define STAR_NAME_COUNT " count
print ""
print "PROGMEM const char pgm_const_name[]   =  // format: byte:ConstelationId Constellation Name string"
print "   {                                     // Names are for only a few prefered stars to save memory space"
qqq=asorti(req_const)
count=0
for ( iii=1 ; iii<qqq ; iii++)
   {
   CCCN = substr(KCONST[req_const[iii]]"          ",1,12)
   printf("   \"\\0%03o%s\"   /* Const ID:%3d  */  \\\n",Kid[req_const[iii]],CCCN,Kid[req_const[iii]])
   count++
   }
print "   };   // This table uses " count*(CONSTEL_NAME_LEN+1) " bytes..."
print "#define CONSTEL_NAME_LEN " CONSTEL_NAME_LEN+1
print "#define CONSTEL_NAME_COUNT " count



# This is an example output:
#-
#-PROGMEM const unsigned long pgm_stars_pos[] =  
#-  {                  //   RA                                                DEC
#-  0                                              ,    0                                                     ,     // 0  origin 
#-  ( 5*TICKS_P_HOUR+55*TICKS_P_MIN+10.3*TICKS_P_SEC), (  7*TICKS_P_DEG+24*TICKS_P_DEG_MIN+25  *TICKS_P_DEG_SEC),     // 1  Orion: Betelgeuse (Red)  5h55m10.3  +7;24'25.0
#-  ( 5*TICKS_P_HOUR+14*TICKS_P_MIN+32.3*TICKS_P_SEC), (351*TICKS_P_DEG+47*TICKS_P_DEG_MIN+54.0*TICKS_P_DEG_SEC),     // 2  Orion: Rigel (Blue)      5h14m32.3  -8;12'06.0
#-  ( 5*TICKS_P_HOUR+47*TICKS_P_MIN+45.4*TICKS_P_SEC), (350*TICKS_P_DEG+19*TICKS_P_DEG_MIN+49  *TICKS_P_DEG_SEC),     // 3  Orion: Saiph     7h47m45.4s -9;40;11.0
#-  ( 5*TICKS_P_HOUR+25*TICKS_P_MIN+ 7.9*TICKS_P_SEC), (  6*TICKS_P_DEG+20*TICKS_P_DEG_MIN+59.0*TICKS_P_DEG_SEC),     // 4  Orion: Bellatrix 5h25m7.9s   6;20;59.0
#-  ( 5*TICKS_P_HOUR+40*TICKS_P_MIN+45.5*TICKS_P_SEC), (358*TICKS_P_DEG+3 *TICKS_P_DEG_MIN+27.0*TICKS_P_DEG_SEC),     // 5  Orion: Alnitak   5h40m45.5  -1;56;33.0
#-  }

print ""
print "PROGMEM const unsigned long pgm_stars_pos[] = // The next "NBNB" stars are used for polar alignment : they are reference stars"
print "   {"
for ( iii=0 ; iii<NB ; iii++)
   {
   RR = my_split(RA[iii])
   DD = my_split(DE[iii])
   if ( match (DE[iii],"^-")) DEDE = "    " DE[iii]
   else                       DEDE = ""
   HIGHLIGHT=KCONST[CName[iii]]
   if ( HIGHLIGHT=="" ) HIGHLIGHT=CName[iii]
   NAME = KNOWN[Name[iii]];
   if ( NAME=="" ) NAME = SName[iii];
   if ( iii==NBNB ) print "   // Start of my own points of interest ..."
   if ( iii <NBNB ) print "   ("RR,"),("DD"), // "sprintf("%3d",iii)" " NAME " " HIGHLIGHT "  " DEDE
   else             print "   ("RR,"),("DD"), // "sprintf("%3d",iii)" >"KNOWN[iii]
   }
print "   0,0  // origin and Null terminator"
print "   };   // This table uses " NB*8 " bytes..."
print "#define STARS_COORD_TOTAL " NB
print "#define STARS_COORD_ALIGN " NBNB

}

function my_split(string)
{
split(string,Parts," ")
if ( match(string,"^[+-]") )   # DEC
   {
   if ( match(string,"^[-]") ) 
      {
      Parts[1] = 359-substr(Parts[1],2)
      Parts[2] =  59-Parts[2]
      Parts[3] =  60-Parts[3]
      }
   return sprintf("%3d*TICKS_P_DEG +%2d*TICKS_P_DEG_MIN +%6g*TICKS_P_DEG_SEC", Parts[1],Parts[2],Parts[3]) 
   }
else                           # RA
   {
   return sprintf("%3d*TICKS_P_HOUR +%2d*TICKS_P_MIN +%6g*TICKS_P_SEC", Parts[1],Parts[2],Parts[3])
   }

}

# $Log: stars2atmel.awk,v $
# Revision 1.3  2012/03/05 22:41:13  pmichel
# more is more
#
# Revision 1.2  2012/03/05 22:12:25  pmichel
# Few fixes, almost complete
#
#
