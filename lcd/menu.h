// This file was auto generated from menu.def and menu.awk
// cat menu.def | awk -f menu.awk | tee menu.h
#define M_ROOT_COUNT                    1
#define M_ROOT_FIRST                    1
#define M_ROOT_LAST                     1
#define M_MAIN_COUNT                    9
#define M_MAIN_FIRST                    2
#define M_MAIN_LAST                     10
#define M_MOZAIC_PAUSE_COUNT            10
#define M_MOZAIC_PAUSE_FIRST            11
#define M_MOZAIC_PAUSE_LAST             20
#define M__MOZ_RA_COUNT                 1
#define M__MOZ_RA_FIRST                 21
#define M__MOZ_RA_LAST                  21
#define M__MOZ_DE_COUNT                 1
#define M__MOZ_DE_FIRST                 22
#define M__MOZ_DE_LAST                  22
#define M_MOZAIC_COUNT                  8
#define M_MOZAIC_FIRST                  23
#define M_MOZAIC_LAST                   30
#define M__MOZ_EXP_COUNT                1
#define M__MOZ_EXP_FIRST                31
#define M__MOZ_EXP_LAST                 31
#define M__MOZ_SPAN1_COUNT              1
#define M__MOZ_SPAN1_FIRST              32
#define M__MOZ_SPAN1_LAST               32
#define M__MOZ_SPAN2_COUNT              1
#define M__MOZ_SPAN2_FIRST              33
#define M__MOZ_SPAN2_LAST               33
#define M__MOZ_SPAN3_COUNT              1
#define M__MOZ_SPAN3_FIRST              34
#define M__MOZ_SPAN3_LAST               34
#define M_CATALOG_COUNT                 5
#define M_CATALOG_FIRST                 35
#define M_CATALOG_LAST                  39
#define M_STORE_COUNT                   5
#define M_STORE_FIRST                   40
#define M_STORE_LAST                    44
#define M_RECALL_COUNT                  6
#define M_RECALL_FIRST                  45
#define M_RECALL_LAST                   50
#define M_TIMELAP_COUNT                 3
#define M_TIMELAP_FIRST                 51
#define M_TIMELAP_LAST                  53
#define M__TIMELAP_EXP_COUNT            1
#define M__TIMELAP_EXP_FIRST            54
#define M__TIMELAP_EXP_LAST             54
#define M__TIMELAP_PERIOD1_COUNT        1
#define M__TIMELAP_PERIOD1_FIRST        55
#define M__TIMELAP_PERIOD1_LAST         55
#define M__TIMELAP_PERIOD2_COUNT        1
#define M__TIMELAP_PERIOD2_FIRST        56
#define M__TIMELAP_PERIOD2_LAST         56
#define M_CGEM_STATUS_COUNT             6
#define M_CGEM_STATUS_FIRST             57
#define M_CGEM_STATUS_LAST              62
#define M__MOZ_RUN_COUNT                3
#define M__MOZ_RUN_FIRST                63
#define M__MOZ_RUN_LAST                 65
#define M__MOZ_PAUSE_RUN_COUNT          3
#define M__MOZ_PAUSE_RUN_FIRST          66
#define M__MOZ_PAUSE_RUN_LAST           68
#define M__TIMELAP_RUN_COUNT            2
#define M__TIMELAP_RUN_FIRST            69
#define M__TIMELAP_RUN_LAST             70
#define M_PAUSE_CANCEL_COUNT            1
#define M_PAUSE_CANCEL_FIRST            71
#define M_PAUSE_CANCEL_LAST             71
#define M_PAUSE_CANCELING_COUNT         1
#define M_PAUSE_CANCELING_FIRST         72
#define M_PAUSE_CANCELING_LAST          72
#define M_CANCEL_COUNT                  1
#define M_CANCEL_FIRST                  73
#define M_CANCEL_LAST                   73
#define M_CANCELING_COUNT               1
#define M_CANCELING_FIRST               74
#define M_CANCELING_LAST                74
#define M_GOTO_COUNT                    1
#define M_GOTO_FIRST                    75
#define M_GOTO_LAST                     75
#define M_EXECUTE_COUNT                 1
#define M_EXECUTE_FIRST                 76
#define M_EXECUTE_LAST                  76
#define M_PRESET_COUNT                  1
#define M_PRESET_FIRST                  77
#define M_PRESET_LAST                   77
#define M_DEBUG_COUNT                   9
#define M_DEBUG_FIRST                   78
#define M_DEBUG_LAST                    86

PROGMEM const long edit_increment[] = {
              1               ,                   //  0
              1000            ,                   //  1
              ONE_DEG_CGEM    ,                   //  2
              ONE_MIN_CGEM    ,                   //  3
              ONE_SEC_CGEM    ,                   //  4
              3600            ,                   //  5
              60              };                  //  6

PROGMEM const char linetxt[]=
              "Cam Driver V2.1a"                  //  0   0
              "Ram:\242X\031         "            //  1   2
              "Mozaic Menu...  "                  //  2   0
              "(GOTO Method)   "                  //  3   0
              "(Pause Method)  "                  //  4   0
              "Catalog Goto... "                  //  5   0
              "                "                  //  6   0
              "Store           "                  //  7   0
              "Position...     "                  //  8   0
              "Recall          "                  //  9   0
              "Timelap Menu... "                  //  10   0
              "CGEM Status...  "                  //  11   0
              "RA:\242R\001          "            //  12   2
              "DE:\242D\002          "            //  13   2
              "RA:\242H\001          "            //  14   2
              "Start Mozaic... "                  //  15   0
              "Set Span to:    "                  //  16   0
              "1/10 of EDGE HD "                  //  17   0
              "1/10 of Newton 6"                  //  18   0
              "Set Exposure:   "                  //  19   0
              "1 Second        "                  //  20   0
              "30 Second       "                  //  21   0
              "60 Second       "                  //  22   0
              "Exposure Time:  "                  //  23   0
              "Current:\242s\003 sec "            //  24   2
              "Total Span:     "                  //  25   0
              "Cur:\242d\017         "            //  26   2
              "Nb Pic:         "                  //  27   0
              "Current:\242n\034 (RA)"            //  28   2
              "Current:\242n\035 (DE)"            //  29   2
              "    Set:\243n\034 (RA)"            //  30   2
              "    Set:\243n\035 (DE)"            //  31   2
              "    Set:\243s\003 sec "            //  32   2
              "Set:\243d\017         "            //  33   2
              "Goto:           "                  //  34   0
              "Orion Fire      "                  //  35   0
              "Orion M42       "                  //  36   0
              "M101            "                  //  37   0
              "Triangulum      "                  //  38   0
              "Andromeda       "                  //  39   0
              "Store:          "                  //  40   0
              "Pos 1           "                  //  41   0
              "Pos 2           "                  //  42   0
              "Pos 3           "                  //  43   0
              "Pos 4           "                  //  44   0
              "Pos 5           "                  //  45   0
              "Recall:         "                  //  46   0
              "MozStart        "                  //  47   0
              "Start Timelap..."                  //  48   0
              "Period:         "                  //  49   0
              "Current:\242t\004     "            //  50   2
              "    Set:\243t\004     "            //  51   2
              "RA:\242x\001          "            //  52   2
              "DE:\242x\002          "            //  53   2
              "Aligned:\242y\012     "            //  54   2
              "In Goto:\242y\013     "            //  55   2
              "Tracking:\242q\026    "            //  56   2
              "Moz Err:\242e\014     "            //  57   2
              "Pos Err:\242e\015     "            //  58   2
              "Latency:\242E\016     "            //  59   2
              "Time-Out:\242e\011    "            //  60   2
              "Histogram:  (x8)"                  //  61   0
              "\242h\023             "            //  62   2
              "Mozaic:\242o\005      "            //  63   2
              "Time:\242m\020    sec "            //  64   2
              "Pos Id:\242p\007      "            //  65   2
              "State:\242z\027       "            //  66   2
              "MozPoz:\242o\005      "            //  67   2
              "Timelap progres:"                  //  68   0
              "Shot no:\242s\032     "            //  69   2
              "Period:\242T\033      "            //  70   2
              "Really Cancel ? "                  //  71   0
              "Cancelling...   "                  //  72   0
              "Goto            "                  //  73   0
              "in Progress...  "                  //  74   0
              "Executing...    "                  //  75   0
              "Done...         "                  //  76   0
              "D1:\242x\036          "            //  77   2
              "D2:\242x\037          "            //  78   2
              "D3:\242x\040          "            //  79   2
              "D4:\242x\041          "            //  80   2
              "D5:\242x\042          "            //  81   2
              "D6:\242x\043          "            //  82   2
              "1 :\242H\044          "            //  83   2
              "DE:\242D\052          "            //  84   2
              "2 :\242H\045          "            //  85   2
              "DE:\242D\053          "            //  86   2
              "3 :\242H\046          "            //  87   2
              "DE:\242D\054          "            //  88   2
              "4 :\242H\047          "            //  89   2
              "DE:\242D\055          "            //  90   2
              "5 :\242H\050          "            //  91   2
              "DE:\242D\056          "            //  92   2
              "M :\242H\051          "            //  93   2
              "DE:\242D\057          ";           //  94   2

#define MENU_TABLE_WIDTH 8
#define MENU_TABLE_LEN   86
PROGMEM const char menu_state_machine[]= {  0,0,0,0,0,0,0,0,0, // STATE : 0 is not a valid state, it means 'do nothing'  > there is one more element so that the array starts at 1
//            UNDO      ENTER     PREV      NEXT      INCREMENT OFFSET    LINE1     LINE2  
               0,       2,        1,        1,        -1,       -1,       0,        1,         // State: 1    M_ROOT              "Cam Driver V2"          "Ram:\242X\031"          
              -1,       23,       10,       3,        -1,       -1,       2,        3,         // State: 2    M_MAIN              "Mozaic Menu..."         "(GOTO Method)"          
              -1,       11,       2,        4,        -1,       -1,       2,        4,         // State: 3    M_MAIN              "Mozaic Menu..."         "(Pause Method)"         
              -1,       35,       3,        5,        -1,       -1,       5,        6,         // State: 4    M_MAIN              "Catalog Goto..."        ""                       
              -1,       40,       4,        6,        -1,       -1,       7,        8,         // State: 5    M_MAIN              "Store"                  "Position..."            
              -1,       45,       5,        7,        -1,       -1,       9,        8,         // State: 6    M_MAIN              "Recall"                 "Position..."            
              -1,       51,       6,        8,        -1,       -1,       10,       6,         // State: 7    M_MAIN              "Timelap Menu..."        ""                       
              -1,       57,       7,        9,        -1,       -1,       11,       6,         // State: 8    M_MAIN              "CGEM Status..."         ""                       
              -1,       0,        8,        10,       -1,       -1,       12,       13,        // State: 9    M_MAIN              "RA:\242R\001"           "DE:\242D\002"           
              -1,       0,        9,        2,        -1,       -1,       14,       13,        // State: 10   M_MAIN              "RA:\242H\001"           "DE:\242D\002"           
              -1,       66,       20,       12,       -1,       -1,       15,       4,         // State: 11   M_MOZAIC_PAUSE      "Start Mozaic..."        "(Pause Method)"         
              -1,       77,       11,       13,       -1,       -1,       16,       17,        // State: 12   M_MOZAIC_PAUSE      "Set Span to:"           "1/10 of EDGE HD"        
              -1,       77,       12,       14,       -1,       -1,       16,       18,        // State: 13   M_MOZAIC_PAUSE      "Set Span to:"           "1/10 of Newton 6"       
              -1,       77,       13,       15,       -1,       -1,       19,       20,        // State: 14   M_MOZAIC_PAUSE      "Set Exposure:"          "1 Second"               
              -1,       77,       14,       16,       -1,       -1,       19,       21,        // State: 15   M_MOZAIC_PAUSE      "Set Exposure:"          "30 Second"              
              -1,       77,       15,       17,       -1,       -1,       19,       22,        // State: 16   M_MOZAIC_PAUSE      "Set Exposure:"          "60 Second"              
              -1,       31,       16,       18,       -1,       -1,       23,       24,        // State: 17   M_MOZAIC_PAUSE      "Exposure Time:"         "Current:\242s\003 sec"  
              -1,       32,       17,       19,       -1,       -1,       25,       26,        // State: 18   M_MOZAIC_PAUSE      "Total Span:"            "Cur:\242d\017"          
              -1,       21,       18,       20,       -1,       -1,       27,       28,        // State: 19   M_MOZAIC_PAUSE      "Nb Pic:"                "Current:\242n\034 (RA)" 
              -1,       22,       19,       11,       -1,       -1,       27,       29,        // State: 20   M_MOZAIC_PAUSE      "Nb Pic:"                "Current:\242n\035 (DE)" 
              -1,       76,       21,       21,       0,        26,       27,       30,        // State: 21   M__MOZ_RA           "Nb Pic:"                "    Set:\243n\034 (RA)" 
              -1,       76,       22,       22,       0,        26,       27,       31,        // State: 22   M__MOZ_DE           "Nb Pic:"                "    Set:\243n\035 (DE)" 
              -1,       63,       30,       24,       -1,       -1,       15,       3,         // State: 23   M_MOZAIC            "Start Mozaic..."        "(GOTO Method)"          
              -1,       77,       23,       25,       -1,       -1,       16,       17,        // State: 24   M_MOZAIC            "Set Span to:"           "1/10 of EDGE HD"        
              -1,       77,       24,       26,       -1,       -1,       16,       18,        // State: 25   M_MOZAIC            "Set Span to:"           "1/10 of Newton 6"       
              -1,       77,       25,       27,       -1,       -1,       19,       20,        // State: 26   M_MOZAIC            "Set Exposure:"          "1 Second"               
              -1,       77,       26,       28,       -1,       -1,       19,       21,        // State: 27   M_MOZAIC            "Set Exposure:"          "30 Second"              
              -1,       77,       27,       29,       -1,       -1,       19,       22,        // State: 28   M_MOZAIC            "Set Exposure:"          "60 Second"              
              -1,       31,       28,       30,       -1,       -1,       23,       24,        // State: 29   M_MOZAIC            "Exposure Time:"         "Current:\242s\003 sec"  
              -1,       32,       29,       23,       -1,       -1,       25,       26,        // State: 30   M_MOZAIC            "Total Span:"            "Cur:\242d\017"          
              -1,       76,       31,       31,       1,        26,       23,       32,        // State: 31   M__MOZ_EXP          "Exposure Time:"         "    Set:\243s\003 sec"  
              -1,       33,       32,       32,       2,        23,       25,       33,        // State: 32   M__MOZ_SPAN1        "Total Span:"            "Set:\243d\017"          
              -1,       34,       33,       33,       3,        26,       25,       33,        // State: 33   M__MOZ_SPAN2        "Total Span:"            "Set:\243d\017"          
              -1,       76,       34,       34,       4,        29,       25,       33,        // State: 34   M__MOZ_SPAN3        "Total Span:"            "Set:\243d\017"          
              -1,       75,       39,       36,       -1,       -1,       34,       35,        // State: 35   M_CATALOG           "Goto:"                  "Orion Fire"             
              -1,       75,       35,       37,       -1,       -1,       34,       36,        // State: 36   M_CATALOG           "Goto:"                  "Orion M42"              
              -1,       75,       36,       38,       -1,       -1,       34,       37,        // State: 37   M_CATALOG           "Goto:"                  "M101"                   
              -1,       75,       37,       39,       -1,       -1,       34,       38,        // State: 38   M_CATALOG           "Goto:"                  "Triangulum"             
              -1,       75,       38,       35,       -1,       -1,       34,       39,        // State: 39   M_CATALOG           "Goto:"                  "Andromeda"              
              -1,       77,       44,       41,       -1,       -1,       40,       41,        // State: 40   M_STORE             "Store:"                 "Pos 1"                  
              -1,       77,       40,       42,       -1,       -1,       40,       42,        // State: 41   M_STORE             "Store:"                 "Pos 2"                  
              -1,       77,       41,       43,       -1,       -1,       40,       43,        // State: 42   M_STORE             "Store:"                 "Pos 3"                  
              -1,       77,       42,       44,       -1,       -1,       40,       44,        // State: 43   M_STORE             "Store:"                 "Pos 4"                  
              -1,       77,       43,       40,       -1,       -1,       40,       45,        // State: 44   M_STORE             "Store:"                 "Pos 5"                  
              -1,       77,       50,       46,       -1,       -1,       46,       41,        // State: 45   M_RECALL            "Recall:"                "Pos 1"                  
              -1,       77,       45,       47,       -1,       -1,       46,       42,        // State: 46   M_RECALL            "Recall:"                "Pos 2"                  
              -1,       77,       46,       48,       -1,       -1,       46,       43,        // State: 47   M_RECALL            "Recall:"                "Pos 3"                  
              -1,       77,       47,       49,       -1,       -1,       46,       44,        // State: 48   M_RECALL            "Recall:"                "Pos 4"                  
              -1,       77,       48,       50,       -1,       -1,       46,       45,        // State: 49   M_RECALL            "Recall:"                "Pos 5"                  
              -1,       77,       49,       45,       -1,       -1,       46,       47,        // State: 50   M_RECALL            "Recall:"                "MozStart"               
              -1,       69,       53,       52,       -1,       -1,       48,       6,         // State: 51   M_TIMELAP           "Start Timelap..."       ""                       
              -1,       54,       51,       53,       -1,       -1,       23,       24,        // State: 52   M_TIMELAP           "Exposure Time:"         "Current:\242s\003 sec"  
              -1,       55,       52,       51,       -1,       -1,       49,       50,        // State: 53   M_TIMELAP           "Period:"                "Current:\242t\004"      
              -1,       76,       54,       54,       1,        26,       23,       32,        // State: 54   M__TIMELAP_EXP      "Exposure Time:"         "    Set:\243s\003 sec"  
              -1,       56,       55,       55,       5,        25,       49,       51,        // State: 55   M__TIMELAP_PERIOD1  "Period:"                "    Set:\243t\004"      
              -1,       76,       56,       56,       6,        28,       49,       51,        // State: 56   M__TIMELAP_PERIOD2  "Period:"                "    Set:\243t\004"      
              -1,       0,        62,       58,       -1,       -1,       52,       53,        // State: 57   M_CGEM_STATUS       "RA:\242x\001"           "DE:\242x\002"           
              -1,       0,        57,       59,       -1,       -1,       54,       55,        // State: 58   M_CGEM_STATUS       "Aligned:\242y\012"      "In Goto:\242y\013"      
              -1,       0,        58,       60,       -1,       -1,       56,       6,         // State: 59   M_CGEM_STATUS       "Tracking:\242q\026"     ""                       
              -1,       0,        59,       61,       -1,       -1,       57,       58,        // State: 60   M_CGEM_STATUS       "Moz Err:\242e\014"      "Pos Err:\242e\015"      
              -1,       0,        60,       62,       -1,       -1,       59,       60,        // State: 61   M_CGEM_STATUS       "Latency:\242E\016"      "Time-Out:\242e\011"     
              -1,       0,        61,       57,       -1,       -1,       61,       62,        // State: 62   M_CGEM_STATUS       "Histogram:  (x8)"       "\242h\023"              
              73,       0,        65,       64,       -1,       -1,       63,       64,        // State: 63   M__MOZ_RUN          "Mozaic:\242o\005"       "Time:\242m\020    sec"  
              73,       0,        63,       65,       -1,       -1,       63,       65,        // State: 64   M__MOZ_RUN          "Mozaic:\242o\005"       "Pos Id:\242p\007"       
              73,       0,        64,       63,       -1,       -1,       63,       66,        // State: 65   M__MOZ_RUN          "Mozaic:\242o\005"       "State:\242z\027"        
              71,       0,        68,       67,       -1,       -1,       67,       64,        // State: 66   M__MOZ_PAUSE_RUN    "MozPoz:\242o\005"       "Time:\242m\020    sec"  
              71,       0,        66,       68,       -1,       -1,       67,       65,        // State: 67   M__MOZ_PAUSE_RUN    "MozPoz:\242o\005"       "Pos Id:\242p\007"       
              71,       0,        67,       66,       -1,       -1,       67,       66,        // State: 68   M__MOZ_PAUSE_RUN    "MozPoz:\242o\005"       "State:\242z\027"        
              73,       0,        70,       70,       -1,       -1,       68,       69,        // State: 69   M__TIMELAP_RUN      "Timelap progres:"       "Shot no:\242s\032"      
              73,       0,        69,       69,       -1,       -1,       68,       70,        // State: 70   M__TIMELAP_RUN      "Timelap progres:"       "Period:\242T\033"       
              -1,       72,       71,       71,       -1,       -1,       71,       6,         // State: 71   M_PAUSE_CANCEL      "Really Cancel ?"        ""                       
               0,       0,        72,       72,       -1,       -1,       72,       6,         // State: 72   M_PAUSE_CANCELING   "Cancelling..."          ""                       
              -1,       74,       73,       73,       -1,       -1,       71,       6,         // State: 73   M_CANCEL            "Really Cancel ?"        ""                       
               0,       0,        74,       74,       -1,       -1,       72,       6,         // State: 74   M_CANCELING         "Cancelling..."          ""                       
               0,       0,        75,       75,       -1,       -1,       73,       74,        // State: 75   M_GOTO              "Goto"                   "in Progress..."         
               0,       0,        76,       76,       -1,       -1,       75,       6,         // State: 76   M_EXECUTE           "Executing..."           ""                       
               0,       0,        77,       77,       -1,       -1,       76,       6,         // State: 77   M_PRESET            "Done..."                ""                       
               0,       0,        86,       79,       -1,       -1,       77,       78,        // State: 78   M_DEBUG             "D1:\242x\036"           "D2:\242x\037"           
               0,       0,        78,       80,       -1,       -1,       79,       80,        // State: 79   M_DEBUG             "D3:\242x\040"           "D4:\242x\041"           
               0,       0,        79,       81,       -1,       -1,       81,       82,        // State: 80   M_DEBUG             "D5:\242x\042"           "D6:\242x\043"           
               0,       0,        80,       82,       -1,       -1,       83,       84,        // State: 81   M_DEBUG             "1 :\242H\044"           "DE:\242D\052"           
               0,       0,        81,       83,       -1,       -1,       85,       86,        // State: 82   M_DEBUG             "2 :\242H\045"           "DE:\242D\053"           
               0,       0,        82,       84,       -1,       -1,       87,       88,        // State: 83   M_DEBUG             "3 :\242H\046"           "DE:\242D\054"           
               0,       0,        83,       85,       -1,       -1,       89,       90,        // State: 84   M_DEBUG             "4 :\242H\047"           "DE:\242D\055"           
               0,       0,        84,       86,       -1,       -1,       91,       92,        // State: 85   M_DEBUG             "5 :\242H\050"           "DE:\242D\056"           
               0,       0,        85,       78,       -1,       -1,       93,       94};       // State: 86   M_DEBUG             "M :\242H\051"           "DE:\242D\057"           
