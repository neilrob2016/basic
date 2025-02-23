/* Must set build OS */
#ifdef NO_OS_SET
#error "Please edit the Makefile for your OS"
#endif

/* Fail with a compile error on mismatched build options */
#if defined(NO_CRYPT) && defined(DES_ONLY)
#error "The NO_CRYPT and DES_ONLY build options are mutually exclusive"
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <math.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#if !defined(NO_CRYPT) && defined(__linux__)
#include <crypt.h>
#endif

#include "build_date.h"

/********************************* MACROS ********************************/

/* Hopefully they'll sort this one day */
#ifdef __APPLE__
#define DES_ONLY
#endif

#ifdef MAINFILE
#define EXTERN 
#else
#define EXTERN extern
#endif

#define INTERPRETER "NRJ-BASIC"
#define COPYRIGHT   "Copyright (C) Neil Robertson 2016-2025"
#define VERSION     "1.12.1"

#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define IS_OP(TOK)     ((TOK)->type == TOK_OP)
#define IS_COM(TOK)    ((TOK)->type == TOK_COM)
#define IS_NUM(TOK)    ((TOK)->type == TOK_NUM)
#define IS_STR(TOK)    ((TOK)->type == TOK_STR)
#define IS_VAR(TOK)    ((TOK)->type == TOK_VAR)
#define IS_DEFEXP(TOK) ((TOK)->type == TOK_DEFEXP)

#define IS_OP_TYPE(TOK,SUBT)  ((TOK)->type == TOK_OP && (TOK)->subtype == SUBT)
#define IS_COM_TYPE(TOK,SUBT) ((TOK)->type == TOK_COM && (TOK)->subtype == SUBT)
#define IS_NUM_TYPE(TOK,SUBT) ((TOK)->type == TOK_NUM && (TOK)->subtype == SUBT)

#define IS_FLOAT(NUM) ((long)(NUM) != NUM)
#define FREE(M)       { if (M) free(M); M = NULL; }
#define SGN(X)        ((X) < 0 ? -1 : ((X) > 0 ? 1 : 0))
#define NOT_COMMA(PC) ((PC) >= runline->num_tokens || !IS_OP_TYPE(&runline->tokens[(PC)],OP_COMMA))
#define PRINT(S,L)    write(STDOUT,S,L)

#define CHECK_STREAM(S) \
	if ((S) < 0 || (S) >= MAX_STREAMS) return ERR_INVALID_STREAM; \
	if (!stream[S]) return ERR_STREAM_NOT_OPEN;

#define FALSE  0
#define TRUE   1
#define EITHER 2

#define CONTROL_D 4
#define DEL1      8
#define DEL2      127
#define ESC       27

#define TERM_ROWS         25
#define TERM_COLS         80
#define MAX_UCHAR         0xFF
#define MAX_RETURN_STACK  1000
#define MAX_INDEXES       5
#define MAX_FUNC_PARAMS   10
#define MAX_STREAMS       20
#define MAX_DIR_STREAMS   10
#define VAL_FIXED_STR_LEN 20
#define CHAR_ALLOC        50
#define NUM_F_KEYS        5
#define DEFMOD_SIZE       (255 + NUM_F_KEYS)
#define MAX_ESC_SEQ_LEN   4
#define ESC_SEQ_PARTIAL  -1
#define ESC_SEQ_INVALID  -2

#ifndef PATH_MAX
#define PATH_MAX 1024 /* Err on the side of caution & keep it small */
#endif

#define DEGS_PER_RADIAN 57.2957795

#define S_IFANY 0

/* Only 1 stream flags for now */
#define SFLAG_NO_WAIT_NL 1

/********************************* STRUCTURES ********************************/

typedef u_char bool;
struct st_runline;

typedef struct 
{
	int type;
	int shmid;
	int shmlen;
	int slen;
	char str[VAL_FIXED_STR_LEN+1];
	char *sval;
	double dval;
} st_value;


typedef struct st_keyval
{
	char *key;
	int key_len;
	st_value value;
	struct st_keyval *prev;
	struct st_keyval *next;
} st_keyval;


typedef struct st_var
{
	char *name;
	int name_len;
	int type;

	/* Arrays */
	int index_cnt;
	int index[MAX_INDEXES];
	int arrsize;
	st_value *value;

	/* Maps */
	int map_cnt;
	st_keyval **first_keyval;
	st_keyval **last_keyval;

	struct st_var *prev;
	struct st_var *next;
} st_var;


typedef struct st_defexp
{
	char *name;
	int len;
	struct st_runline *runline;
	struct st_defexp *prev;
	struct st_defexp *next;
} st_defexp;


/* Used by FOR/NEXT and LOOP/LEND */
typedef struct 
{
	bool looped;
	double from;
	double to;
	double step;
} st_for_loop;


typedef struct
{
	bool looped;
	int map_index;
	int map_pos;
} st_foreach_loop;


typedef struct 
{
	int type;
	int subtype;
	int len;
	int lazy_jump; /* Used in AND lazy evaluation */
	int lazy_end;
	bool quoted;
	bool negative;
	bool fp_checked;
	char *str;
	double dval;
	st_var *var;
	st_defexp *exp;
} st_token;


/* A run line is an individual execution line. Eg: PRINT "hello". Which
   can either be terminated by a newline or colon */
typedef struct st_runline
{
	int num_tokens;
	int alloc_tokens;
	struct st_progline *parent;
	st_token *tokens;
	st_for_loop *for_loop;
	st_foreach_loop *foreach_loop;

	/* Used by all loops, IF, GOTO, CHOOSE, CHOSEN, BREAK & DATA */
	struct st_runline *jump;      
	struct st_runline *else_jump; /* Used by IF */
	struct st_runline *next_case; /* Used by CHOOSE */
	struct st_runline *prev;
	struct st_runline *next;
	struct st_defexp *defexp;     /* For when DEFEXP entered direct */
	bool renumbered;
} st_runline;


/* A program line consists of 1 or more run lines which all share the same
   line number */
typedef struct st_progline
{
	u_int linenum;

	/* Pointers into the linked list formed by the runlines */
	st_runline *first_runline;
	st_runline *last_runline;

	/* The linked list of program lines which allows us to find a given 
	   line by its number and delete all its runlines in one go */
	struct st_progline *prev;
	struct st_progline *next;
} st_progline;


typedef struct
{
	char *name;
	int num_params;
	int param_type[MAX_FUNC_PARAMS];
	int (*funcptr)(
		int func, st_var **var, st_value *vallist, st_value *result);
} st_func;


typedef struct
{
	char *name;
	int (*funcptr)(int comnum, st_runline *runline);
} st_com;


typedef struct
{
	char *str;
	char prec;
} st_op;


typedef struct
{
	char *str;
	int alloced;
	int len;
	int cursor_pos;
} st_keybline;


typedef struct st_label
{
	char *name;
	int len;
	st_runline *runline;
	struct st_label *prev;
	struct st_label *next;
} st_label;


typedef struct
{
	unsigned on_break_clear:1;
	unsigned on_break_cont:1;
	unsigned on_error_cont:1;
	unsigned on_termsize_cont:1;
	unsigned autorun:1;
	unsigned executing:1;
	unsigned child_process:1;
	unsigned prog_new_runline_set:1;
	unsigned listing_line_wrap:1;
	unsigned angle_in_degrees:1;
	unsigned draw_prompt:1;
} st_flags;

/********************************** ERRORS ********************************/

enum
{
	/* 0 */
	OK,
	ERR_SYNTAX,
	ERR_NO_SUCH_LINE,
	ERR_INVALID_LINENUM,
	ERR_INVALID_ARG,

	/* 5 */
	ERR_MISSING_PARAMS,
	ERR_TOO_MANY_PARAMS,
	ERR_UNDEFINED_VAR_OR_FUNC,
	ERR_VAR_ALREADY_DEF,
	ERR_VAR_IS_NOT_ARRAY,

	/* 10 */
	ERR_VAR_READ_ONLY,
	ERR_MAP_ARRAY,
	ERR_INVALID_VAR_NAME,
	ERR_VAR_INDEX_OOB,
	ERR_VAR_MAX_INDEX,

	/* 15 */
	ERR_VAR_IS_NOT_MAP,
	ERR_VAR_IS_MAP,
	ERR_KEY_NOT_FOUND,
	ERR_CANT_REDIM,
	ERR_INVALID_NEG,

	/* 20 */
	ERR_INCOMPLETE_EXPR,
	ERR_STACK_OVERFLOW,
	ERR_DIVIDE_BY_ZERO,
	ERR_MISSING_BRACKET,
	ERR_MISSING_END_QUOTES,

	/* 25 */
	ERR_MAX_RECURSION,
	ERR_OUT_OF_RANGE,
	ERR_UNEXPECTED_RETURN,
	ERR_UNEXPECTED_WEND,
	ERR_UNEXPECTED_UNTIL,

	/* 30 */
	ERR_UNEXPECTED_ELSE,
	ERR_UNEXPECTED_FI,
	ERR_UNEXPECTED_NEXT,
	ERR_UNEXPECTED_NEXTEACH,
	ERR_UNEXPECTED_LEND,

	/* 35 */
	ERR_UNEXPECTED_READ,
	ERR_MISSING_WEND,
	ERR_MISSING_NEXT,
	ERR_MISSING_NEXTEACH,
	ERR_MISSING_UNTIL,

	/* 40 */
	ERR_MISSING_LEND,
	ERR_MISSING_THEN,
	ERR_MISSING_FI,
	ERR_DATA_EXHAUSTED,
	ERR_CANT_RESTORE,

	/* 45 */
	ERR_CANT_OPEN_FILE,
	ERR_CANT_OPEN_DIR,
	ERR_READ,
	ERR_WRITE,
	ERR_SEEK,

	/* 50 */
	ERR_CANT_DEL_FILE,
	ERR_NOTHING_TO_SAVE,
	ERR_NO_SUCH_FILE,
	ERR_PATH_TOO_LONG,
	ERR_MAX_STREAMS,

	/* 55 */
	ERR_MAX_DIR_STREAMS,
	ERR_INVALID_STREAM,
	ERR_STREAM_NOT_OPEN,
	ERR_DIR_STREAM,
	ERR_DIR_SEEK,

	/* 60 */
	ERR_DIR_INPUT_COM,
	ERR_NOT_ALLOWED_IN_PROG,
	ERR_CANT_MERGE,
	ERR_SLEEP,
	ERR_DUPLICATE_DEFEXP,

	/* 65 */
	ERR_UNDEFINED_DEFEXP,
	ERR_INVALID_DEFEXP_NAME,
	ERR_CANT_CONTINUE,
	ERR_MISSING_CHOSEN,
	ERR_UNEXPECTED_CHOSEN,

	/* 70 */
	ERR_INVALID_ARRAY_TYPE,
	ERR_VAR_NO_MEM_SIZE,
	ERR_SHARMEM,
	ERR_INVALID_HISTORY_LINE,
	ERR_INVALID_PORT,

	/* 75 */
	ERR_SOCKET,
	ERR_ENCRYPTION_NOT_SUPPORTED,
	ERR_UNAVAILABLE,
	ERR_VAR_ALREADY_WATCHED,
	ERR_VAR_NOT_WATCHED,

	/* 80 */
	ERR_OVERFLOW,
	ERR_LINE_EXISTS,
	ERR_REGEX,
	ERR_ARR_SIZE,
	ERR_UNEXPECTED_BREAK,

	/* 85 */
	ERR_UNEXPECTED_CONTLOOP,
	ERR_VAR_ALREADY_HAS_NAME,
	ERR_DEFEXP_ALREADY_HAS_NAME,
	ERR_RENAME_SAME,
	ERR_RENAME_TYPE,

	/* 90 */
	ERR_INVALID_PATH,
	ERR_INVALID_FILE_PERMS,
	ERR_LP,
	ERR_LP_INPUT,
	ERR_INVALID_LABEL_NAME,

	/* 95 */
	ERR_INVALID_LABEL_LOC,
	ERR_DUPLICATE_LABEL,
	ERR_UNDEFINED_LABEL,

	NUM_ERRORS
};


#ifdef MAINFILE
char *error_str[NUM_ERRORS] =
{
	/* 0 */
	"OK",
	"Syntax error",
	"No such line(s)",
	"Invalid line number",
	"Invalid argument",

	/* 5 */
	"Missing function parameters",
	"Too many function parameters",
	"Undefined variable or function",
	"Variable already defined",
	"Variable is not an array",

	/* 10 */
	"Variable is read only",
	"Map variables cannot be arrays",
	"Invalid variable name",
	"Array indexing invalid or out of bounds",
	"Array indexing limit exceeded",

	/* 15 */
	"Variable is not a map",
	"Variable is a map",
	"Key not found",
	"Cannot REDIM to smaller size",
	"Invalid negative",

	/* 20 */
	"Incomplete expression",
	"Stack overflow",
	"Divide by zero",
	"Missing bracket(s)",
	"Missing end quotes",

	/* 25 */
	"Recursion limit exceeded",
	"Value out of range",
	"Unexpected RETURN",
	"Unexpected WEND",
	"Unexpected UNTIL",

	/* 30 */
	"Unexpected ELSE",
	"Unexpected FI",
	"Unexpected NEXT",
	"Unexpected NEXTEACH",
	"Unexpected LEND",

	/* 35 */
	"Unexpected READ",
	"Missing WEND",
	"Missing NEXT",
	"Missing NEXTEACH",
	"Missing UNTIL",

	/* 40 */
	"Missing LEND",
	"Missing THEN",
	"Missing FI",
	"DATA exhausted",
	"Cannot RESTORE to non DATA line",

	/* 45 */
	"Cannot open file",
	"Cannot open directory",
	"Read failure",
	"Write failure",
	"Seek failure",

	/* 50 */
	"Cannot delete file",
	"No program to save",
	"No such path or file",
	"Path too long",
	"Maximum open streams",

	/* 55 */
	"Maximum open directory streams",
	"Invalid stream",
	"Stream not open",
	"Directory streams are read only",
	"Cannot seek in directory stream",

	/* 60 */
	"Cannot read a directory stream with CINPUT or NINPUT",
	"Command not allowed in a program",
	"Cannot merge due to line overwrite",
	"Sleep failure",
	"Duplicate DEFEXP",

	/* 65 */
	"Undefined DEFEXP",
	"Invalid DEFEXP name",
	"Cannot CONTinue",
	"Missing CHOSEN",
	"Unexpected CHOSEN",

	/* 70 */
	"Invalid array type",
	"No size given for shared memory variable",
	"Shared memory error",
	"Invalid history line",
	"Invalid port number",

	/* 75 */
	"Socket error",
	"Encryption type not supported",
	"Function unavailable in this build",
	"Variable is already being watched",
	"Variable is not being watched",

	/* 80 */
	"Overflow",
	"Line already exists",
	"Invalid regular expression",
	"Invalid array size",
	"Unexpected BREAK",

	/* 85 */
	"Unexpected CONTLOOP",
	"A variable already has the same name",
	"A DEFEXP already has the same name",
	"RENAME requires arguments differ",
	"RENAME can only rename variables and DEFEXPs",

	/* 90 */
	"Invalid path or path not found",
	"Invalid file/directory permissions",
	"Cannot connect to the printer",
	"Cannot input from the printer",
	"Invalid label name",

	/* 95 */
	"Invalid label location",
	"Duplicate label",
	"Undefined label"
};
#else
extern char *error_str[NUM_ERRORS];
#endif

/********************************** COMMANDS *********************************/

enum
{
	/* 0 */
	COM_REM,
	COM_REM_SHORTCUT,
	COM_LIST,
	COM_PLIST,
	COM_LLIST,

	/* 5 */
	COM_NEW,
	COM_CLEAR,
	COM_CLEARKEYS,
	COM_DELETE,
	COM_RUN,

	/* 10 */
	COM_DIM,
	COM_CDIM,
	COM_LET,
	COM_REDIM,
	COM_PRINT,

	/* 15 */
	COM_IF,
	COM_THEN,
	COM_ELSE,
	COM_FI,
	COM_FIALL,

	/* 20 */
	COM_GOTO,
	COM_GOSUB,
	COM_RETURN,
	COM_WHILE,
	COM_WEND,

	/* 25 */
	COM_REPEAT,
	COM_UNTIL,
	COM_FOR,
	COM_TO,
	COM_STEP,

	/* 30 */
	COM_NEXT,
	COM_FOREACH,
	COM_IN,
	COM_NEXTEACH,
	COM_LOOP,

	/* 35 */
	COM_LEND,
	COM_RENUM,
	COM_DATA,
	COM_READ,
	COM_RESTORE,

	/* 40 */
	COM_AUTORESTORE,
	COM_EXIT,
	COM_STOP,
	COM_LOAD,
	COM_CHAIN,

	/* 45 */
	COM_MERGE,
	COM_SAVE,
	COM_DIR,
	COM_DIRL,
	COM_PDIR,

	/* 50 */
	COM_PDIRL,
	COM_RM,
	COM_INPUT,
	COM_CINPUT,
	COM_NINPUT,

	/* 55 */
	COM_CLOSE,
	COM_CLOSEDIR,
	COM_DELKEY,
	COM_ON,
	COM_ERROR,

	/* 60 */
	COM_BREAK,
	COM_CONT,
	COM_CLS,
	COM_LOCATE,
	COM_PEN,

	/* 65 */
	COM_PAPER,
	COM_ATTR,
	COM_SCROLL,
	COM_CURSOR,
	COM_PLOT,

	/* 70 */
	COM_LINE,
	COM_RECT,
	COM_CIRCLE,
	COM_PAUSE,
	COM_TRON,

	/* 75 */
	COM_TRONS,
	COM_TROFF,
	COM_WRON,
	COM_WROFF,
	COM_INDENT,

	/* 80 */
	COM_SEED,
	COM_DEFEXP,
	COM_DEFMOD,
	COM_DUMP,
	COM_LDUMP,

	/* 85 */
	COM_FULL,
	COM_HISTORY,
	COM_HELP,
	COM_EDIT,
	COM_EVAL,

	/* 90 */
	COM_CHOOSE,
	COM_CASE,
	COM_DEFAULT,
	COM_CHOSEN,
	COM_WATCH,

	/* 95 */
	COM_UNWATCH,
	COM_DEG,
	COM_RAD,
	COM_KILLALL,
	COM_END,

	/* 100 */
	COM_FROM,
	COM_MOVE,
	COM_CONTLOOP,
	COM_STON,
	COM_STOFF,

	/* 105 */
	COM_RENAME,
	COM_TERMSIZE,
	COM_LABEL,
	COM_AUTO,

	NUM_COMMANDS
};


/* Declare the command functions */
#define DECL_COM(F) int com##F(int comnum, struct st_runline *runline);

DECL_COM(Default)
DECL_COM(Unexpected)
DECL_COM(DoNothing)
DECL_COM(ListHistory)
DECL_COM(New)
DECL_COM(Clear)
DECL_COM(ClearDefMods)
DECL_COM(DeleteMove)
DECL_COM(Run)
DECL_COM(DimLet)
DECL_COM(Redim)
DECL_COM(Print)
DECL_COM(If)
DECL_COM(Else)
DECL_COM(GotoGosub)
DECL_COM(Return)
DECL_COM(While)
DECL_COM(Wend)
DECL_COM(Repeat)
DECL_COM(Until)
DECL_COM(For)
DECL_COM(Next)
DECL_COM(ForEach)
DECL_COM(NextEach)
DECL_COM(Loop)
DECL_COM(Lend)
DECL_COM(RenumAuto)
DECL_COM(Read)
DECL_COM(Restore)
DECL_COM(Exit)
DECL_COM(Stop)
DECL_COM(Break)
DECL_COM(Cont)
DECL_COM(ContLoop)
DECL_COM(Load)
DECL_COM(Save)
DECL_COM(Dir)
DECL_COM(Rm)
DECL_COM(Input)
DECL_COM(Close)
DECL_COM(CloseDir)
DECL_COM(Seek)
DECL_COM(DelKey)
DECL_COM(On)
DECL_COM(Cls)
DECL_COM(LocatePlot)
DECL_COM(Colour)
DECL_COM(Attr)
DECL_COM(Scroll)
DECL_COM(Cursor)
DECL_COM(LineRect)
DECL_COM(Circle)
DECL_COM(Sleep)
DECL_COM(Trace)
DECL_COM(Wrap)
DECL_COM(Indent)
DECL_COM(Seed)
DECL_COM(DefExp)
DECL_COM(DefMod)
DECL_COM(Dump)
DECL_COM(Help)
DECL_COM(Edit)
DECL_COM(Eval)
DECL_COM(Choose)
DECL_COM(Chosen)
DECL_COM(Watch)
DECL_COM(Unwatch)
DECL_COM(AngleType)
DECL_COM(KillAll)
DECL_COM(Strict)
DECL_COM(Rename)

#ifdef MAINFILE
st_com command[NUM_COMMANDS] =
{
	/* 0 */
	{ "REM",        comDoNothing },
	{ "'",          comDoNothing },  /* Shortcut for REM */
	{ "LIST",       comListHistory },
	{ "PLIST",      comListHistory },
	{ "LLIST",      comListHistory },

	/* 5 */
	{ "NEW",        comNew  },
	{ "CLEAR",      comClear },
	{ "CLEARMODS",  comClearDefMods },
	{ "DELETE",     comDeleteMove },
	{ "RUN",        comRun },

	/* 10 */
	{ "DIM",        comDimLet },
	{ "CDIM",       comDimLet },
	{ "LET",        comDimLet },
	{ "REDIM",      comRedim },
	{ "PRINT",      comPrint },

	/* 15 */
	{ "IF",         comIf },
	{ "THEN",       comUnexpected },
	{ "ELSE",       comElse },
	{ "FI",         comDefault },
	{ "FIALL",      comDefault },

	/* 20 */
	{ "GOTO",       comGotoGosub },
	{ "GOSUB",      comGotoGosub },
	{ "RETURN",     comReturn },
	{ "WHILE",      comWhile },
	{ "WEND",       comWend },

	/* 25 */
	{ "REPEAT",     comRepeat },
	{ "UNTIL",      comUntil },
	{ "FOR",        comFor },
	{ "TO",         comUnexpected },
	{ "STEP",       comUnexpected },

	/* 30 */
	{ "NEXT",       comNext },
	{ "FOREACH",    comForEach },
	{ "IN",         comUnexpected },
	{ "NEXTEACH",   comNextEach },
	{ "LOOP",       comLoop },

	/* 35 */
	{ "LEND",       comLend },
	{ "RENUM",      comRenumAuto },
	{ "DATA",       comDoNothing },
	{ "READ",       comRead },
	{ "RESTORE",    comRestore },

	/* 40 */
	{ "AUTORESTORE",comRestore },
	{ "EXIT",       comExit },
	{ "STOP",       comStop },
	{ "LOAD",       comLoad },
	{ "CHAIN",      comLoad },

	/* 45 */
	{ "MERGE",      comLoad },
	{ "SAVE",       comSave },
	{ "DIR",        comDir },
	{ "DIRL",       comDir },
	{ "PDIR",       comDir },

	/* 50 */
	{ "PDIRL",      comDir },
	{ "RM",         comRm },
	{ "INPUT",      comInput },
	{ "CINPUT",     comInput },
	{ "NINPUT",     comInput },

	/* 55 */
	{ "CLOSE",      comClose },
	{ "CLOSEDIR",   comCloseDir },
	{ "DELKEY",     comDelKey },
	{ "ON",         comOn },
	{ "ERROR",      comUnexpected },

	/* 60 */
	{ "BREAK",      comBreak },
	{ "CONT",       comCont },
	{ "CLS",        comCls },
	{ "LOCATE",     comLocatePlot },
	{ "PEN",        comColour },

	/* 65 */
	{ "PAPER",      comColour },
	{ "ATTR",       comAttr },
	{ "SCROLL",     comScroll },
	{ "CURSOR",     comCursor },
	{ "PLOT",       comLocatePlot },

	/* 70 */
	{ "LINE",       comLineRect },
	{ "RECT",       comLineRect },
	{ "CIRCLE",     comCircle },
	{ "SLEEP",      comSleep },
	{ "TRON",       comTrace },

	/* 75 */
	{ "TRONS",      comTrace },
	{ "TROFF",      comTrace },
	{ "WRON",       comWrap },
	{ "WROFF",      comWrap },
	{ "INDENT",     comIndent },

	/* 80 */
	{ "SEED",       comSeed },
	{ "DEFEXP",     comDefExp },
	{ "DEFMOD",     comDefMod },
	{ "DUMP",       comDump },
	{ "LDUMP",      comDump },

	/* 85 */
	{ "FULL",       comUnexpected },
	{ "HISTORY",    comListHistory },
	{ "HELP",       comHelp },
	{ "EDIT",       comEdit },
	{ "EVAL",       comEval },

	/* 90 */
	{ "CHOOSE",     comChoose },
	{ "CASE",       comDoNothing },
	{ "DEFAULT",    comDoNothing },
	{ "CHOSEN",     comChosen },
	{ "WATCH",      comWatch },

	/* 95 */
	{ "UNWATCH",    comUnwatch },
	{ "DEG",        comAngleType },
	{ "RAD",        comAngleType },
	{ "KILLALL",    comKillAll },
	{ "END",        comUnexpected },

	/* 100 */
	{ "FROM",       comUnexpected },
	{ "MOVE",       comDeleteMove },
	{ "CONTLOOP",   comContLoop },
	{ "STON",       comStrict },
	{ "STOFF",      comStrict },

	/* 105 */
	{ "RENAME",     comRename },
	{ "TERMSIZE",   comUnexpected },
	{ "LABEL",      comDoNothing },
	{ "AUTO",       comRenumAuto }
};
#else
extern st_com command[NUM_COMMANDS];
#endif

/************************* VALUE TYPES & FUNCTIONS ***************************/

enum
{
	VAL_UNDEF,
	VAL_NUM,
	VAL_STR,
	VAL_BOTH  /* Only used for functions */
};


enum
{
	/* 0 */
	FUNC_ABS,
	FUNC_SGN,
	FUNC_ROUND,
	FUNC_FLOOR,
	FUNC_CEIL,

	/* 5 */
	FUNC_SQRT,
	FUNC_POW,
	FUNC_SIN,
	FUNC_COS,
	FUNC_TAN,

	/* 10 */
	FUNC_ASIN,
	FUNC_ACOS,
	FUNC_ATAN,
	FUNC_LOG2,
	FUNC_LOG10,

	/* 15 */
	FUNC_LOG,
	FUNC_HYPOT,
	FUNC_PARITY,
	FUNC_MAX,
	FUNC_MIN,

	/* 20 */
	FUNC_ASC,
	FUNC_CHR,
	FUNC_INSTR,
	FUNC_SUB,
	FUNC_LEFT,

	/* 25 */
	FUNC_RIGHT,
	FUNC_STRIP,
	FUNC_LSTRIP,
	FUNC_RSTRIP,
	FUNC_STRLEN,

	/* 30 */
	FUNC_ISNUM,
	FUNC_ISSTR,
	FUNC_ISNUMSTR,
	FUNC_HASKEY,
	FUNC_GETKEY,

	/* 35 */
	FUNC_BIN,
	FUNC_OCT,
	FUNC_HEX,
	FUNC_MATCH,
	FUNC_ERROR,

	/* 40 */
	FUNC_SYSERROR,
	FUNC_RESERROR,
	FUNC_UPPER,
	FUNC_LOWER,
	FUNC_ELEMENTSTR,

	/* 45 */
	FUNC_ELEMENTCNT,
	FUNC_REPLACE,
	FUNC_REPLACEFR,
	FUNC_INSERT,
	FUNC_FORMAT,

	/* 50 */
	FUNC_MAXSTR,
	FUNC_MINSTR,
	FUNC_ARRSIZE,
	FUNC_MAPSIZE,
	FUNC_TONUM,

	/* 55 */
	FUNC_TOSTR,
	FUNC_OPEN,
	FUNC_OPENDIR,
	FUNC_GETDIR,
	FUNC_CHDIR,

	/* 60 */
	FUNC_MKDIR,
	FUNC_SEEK,
	FUNC_RMFILE,
	FUNC_RMDIR,
	FUNC_RMFSO,

	/* 65 */
	FUNC_STAT,
	FUNC_LSTAT,
	FUNC_CHMOD,
	FUNC_UMASK,
	FUNC_CANREAD,

	/* 70 */
	FUNC_CANWRITE,
	FUNC_SELECT,
	FUNC_RAND,
	FUNC_RANDOM,
	FUNC_TIME,

	/* 75 */
	FUNC_DATE,
	FUNC_DATETOSECS,
	FUNC_POPEN,
	FUNC_FORK,
	FUNC_EXEC,

	/* 80 */
	FUNC_WAITPID,
	FUNC_CHECKPID,
	FUNC_KILL,
	FUNC_PIPE,
	FUNC_CONNECT,

	/* 85 */
	FUNC_LISTEN,
	FUNC_ACCEPT,
	FUNC_GETIP,
	FUNC_IP2HOST,
	FUNC_HOST2IP,

	/* 90 */
	FUNC_GETUSERBYID,
	FUNC_GETGROUPBYID,
	FUNC_GETUSERBYNAME,
	FUNC_GETGROUPBYNAME,
	FUNC_GETENV,

	/* 95 */
	FUNC_SETENV,
	FUNC_SYSTEM,
	FUNC_SYSINFO,
	FUNC_CRYPT,
	FUNC_LPAD,

	/* 100 */
	FUNC_RPAD,
	FUNC_NUMSTRBASE,
	FUNC_PATH,
	FUNC_HAVEDATA,
	FUNC_REGMATCH,

	/* 105 */
	FUNC_EXP,
	FUNC_EXP2,
	FUNC_EXP10,

	NUM_FUNCTIONS
};


/* Declare the C functions for the BASIC functions */
#define DECL_FUNC(F) \
	int func##F(int func, st_var **var, st_value *vallist, st_value *result);

DECL_FUNC(Abs)
DECL_FUNC(Sgn)
DECL_FUNC(Round)
DECL_FUNC(Floor)
DECL_FUNC(Ceil)
DECL_FUNC(Sqrt)
DECL_FUNC(Pow)
DECL_FUNC(Trig1)
DECL_FUNC(Trig2)
DECL_FUNC(Log)
DECL_FUNC(Hypot)
DECL_FUNC(Exp)
DECL_FUNC(Parity)
DECL_FUNC(MaxMin)
DECL_FUNC(Asc)
DECL_FUNC(ChrStr)
DECL_FUNC(InStr)
DECL_FUNC(StrLen)
DECL_FUNC(SubStr)
DECL_FUNC(IsStr)
DECL_FUNC(IsType)
DECL_FUNC(IsNumStr)
DECL_FUNC(SubStr)
DECL_FUNC(LeftStr)
DECL_FUNC(RightStr)
DECL_FUNC(StripStr)
DECL_FUNC(HasKey)
DECL_FUNC(GetKeyStr)
DECL_FUNC(BinStr)
DECL_FUNC(OctStr)
DECL_FUNC(HexStr)
DECL_FUNC(Match)
DECL_FUNC(RegMatch)
DECL_FUNC(ErrorStr)
DECL_FUNC(SysErrorStr)
DECL_FUNC(ResErrorStr)
DECL_FUNC(UpperLowerStr)
DECL_FUNC(ElementStr)
DECL_FUNC(ReplaceStr)
DECL_FUNC(InsertStr)
DECL_FUNC(FormatStr)
DECL_FUNC(MaxMinStr)
DECL_FUNC(ArrSize)
DECL_FUNC(MapSize)
DECL_FUNC(ToNum)
DECL_FUNC(ToStr)
DECL_FUNC(Open)
DECL_FUNC(OpenDir)
DECL_FUNC(GetDirStr)
DECL_FUNC(ChDirStr)
DECL_FUNC(MkDirStr)
DECL_FUNC(Seek)
DECL_FUNC(RmFSOStr)
DECL_FUNC(StatStr)
DECL_FUNC(ChModStr)
DECL_FUNC(Umask)
DECL_FUNC(CanRW)
DECL_FUNC(Select)
DECL_FUNC(Rand)
DECL_FUNC(Random)
DECL_FUNC(Time)
DECL_FUNC(DateStr)
DECL_FUNC(DateToSecs)
DECL_FUNC(Fork)
DECL_FUNC(Exec)
DECL_FUNC(Popen)
DECL_FUNC(WaitCheckStr)
DECL_FUNC(Kill)
DECL_FUNC(Pipe)
DECL_FUNC(Connect)
DECL_FUNC(Listen)
DECL_FUNC(Accept)
DECL_FUNC(GetIPStr)
DECL_FUNC(IP2HostStr)
DECL_FUNC(Host2IPStr)
DECL_FUNC(GetUserStr)
DECL_FUNC(GetGroupStr)
DECL_FUNC(GetEnvStr)
DECL_FUNC(SetEnv)
DECL_FUNC(System)
DECL_FUNC(SysInfoStr)
DECL_FUNC(CryptStr)
DECL_FUNC(PadStr)
DECL_FUNC(NumStrBase)
DECL_FUNC(PathStr)
DECL_FUNC(HaveData)

#ifdef MAINFILE
/* VAL_UNDEF in a valid position means its a variable, not a variable value */
st_func function[NUM_FUNCTIONS] =
{
	/* 0 */
	{ "ABS",            1, { VAL_NUM }, funcAbs },
	{ "SGN",            1, { VAL_NUM }, funcSgn },
	{ "ROUND",          1, { VAL_NUM }, funcRound },
	{ "FLOOR",          1, { VAL_NUM }, funcFloor },
	{ "CEIL",           1, { VAL_NUM }, funcCeil },

	/* 5 */
	{ "SQRT",           1, { VAL_NUM }, funcSqrt },
	{ "POW",            2, { VAL_NUM, VAL_NUM }, funcPow },
	{ "SIN",            1, { VAL_NUM }, funcTrig1 },
	{ "COS",            1, { VAL_NUM }, funcTrig1 },
	{ "TAN",            1, { VAL_NUM }, funcTrig1 },

	/* 10 */
	{ "ASIN",           1, { VAL_NUM }, funcTrig2 },
	{ "ACOS",           1, { VAL_NUM }, funcTrig2 },
	{ "ATAN",           1, { VAL_NUM }, funcTrig2 },
	{ "LOG2",           1, { VAL_NUM }, funcLog },
	{ "LOG10",          1, { VAL_NUM }, funcLog },

	/* 15 */
	{ "LOG",            1, { VAL_NUM }, funcLog },
	{ "HYPOT",          2, { VAL_NUM, VAL_NUM }, funcHypot },
	{ "PARITY",         1, { VAL_NUM }, funcParity },
	{ "MAX",           -2, { VAL_NUM, VAL_NUM }, funcMaxMin },
	{ "MIN",           -2, { VAL_NUM, VAL_NUM }, funcMaxMin },

	/* 20 */
	{ "ASC",            1, { VAL_STR }, funcAsc },
	{ "CHR$",           1, { VAL_NUM }, funcChrStr },
	{ "INSTR",          3, { VAL_STR, VAL_STR, VAL_NUM }, funcInStr },
	{ "SUB$",           3, { VAL_STR, VAL_NUM, VAL_NUM }, funcSubStr },
	{ "LEFT$",          2, { VAL_STR, VAL_NUM }, funcLeftStr },

	/* 25 */
	{ "RIGHT$",         2, { VAL_STR, VAL_NUM }, funcRightStr },
	{ "STRIP$",         1, { VAL_STR }, funcStripStr },
	{ "LSTRIP$",        1, { VAL_STR }, funcStripStr },
	{ "RSTRIP$",        1, { VAL_STR }, funcStripStr },
	{ "STRLEN",         1, { VAL_STR }, funcStrLen },

	/* 30 */
	{ "ISNUM",          1, { VAL_BOTH }, funcIsType },
	{ "ISSTR",          1, { VAL_BOTH }, funcIsType },
	{ "ISNUMSTR",       1, { VAL_STR }, funcIsNumStr },
	{ "HASKEY",         2, { VAL_UNDEF, VAL_STR }, funcHasKey },
	{ "GETKEY$",        2, { VAL_UNDEF, VAL_NUM }, funcGetKeyStr },

	/* 35 */
	{ "BIN$",           1, { VAL_NUM }, funcBinStr },
	{ "OCT$",           1, { VAL_NUM }, funcOctStr },
	{ "HEX$",           1, { VAL_NUM }, funcHexStr },
	{ "MATCH",          3, { VAL_STR, VAL_STR, VAL_NUM }, funcMatch },
	{ "ERROR$",         1, { VAL_NUM }, funcErrorStr },

	/* 40 */
	{ "SYSERROR$",      1, { VAL_NUM }, funcSysErrorStr },
	{ "RESERROR$",      1, { VAL_NUM }, funcResErrorStr },
	{ "UPPER$",         1, { VAL_STR }, funcUpperLowerStr },
	{ "LOWER$",         1, { VAL_STR }, funcUpperLowerStr },
	{ "ELEMENT$",       2, { VAL_STR, VAL_NUM }, funcElementStr },

	/* 45 */
	{ "ELEMENTCNT",     1, { VAL_STR }, funcElementStr },
	{ "REPLACE$",       3, { VAL_STR, VAL_STR, VAL_STR }, funcReplaceStr },
	{ "REPLACEFR$",     4, { VAL_STR, VAL_STR, VAL_STR, VAL_NUM }, funcReplaceStr },
	{ "INSERT$",        3, { VAL_STR, VAL_STR, VAL_NUM }, funcInsertStr },
	{ "FORMAT$",        2, { VAL_STR, VAL_NUM }, funcFormatStr },

	/* 50 */
	{ "MAX$",          -2, { VAL_STR, VAL_STR }, funcMaxMinStr },
	{ "MIN$",          -2, { VAL_STR, VAL_STR }, funcMaxMinStr },
	{ "ARRSIZE",        1, { VAL_UNDEF }, funcArrSize },
	{ "MAPSIZE",        1, { VAL_UNDEF }, funcMapSize },
	{ "TONUM",          1, { VAL_STR }, funcToNum },

	/* 55 */
	{ "TOSTR$",         1, { VAL_NUM }, funcToStr },
	{ "OPEN",          -2, { VAL_STR, VAL_STR }, funcOpen },
	{ "OPENDIR",        1, { VAL_STR }, funcOpenDir },
	{ "GETDIR$",        0, { VAL_UNDEF }, funcGetDirStr },
	{ "CHDIR$",         1, { VAL_STR }, funcChDirStr },

	/* 60 */
	{ "MKDIR$",        -1, { VAL_STR }, funcMkDirStr },
	{ "SEEK",           2, { VAL_NUM, VAL_NUM }, funcSeek },
	{ "RMFILE$",        1, { VAL_STR }, funcRmFSOStr },
	{ "RMDIR$",         1, { VAL_STR }, funcRmFSOStr },
	{ "RMFSO$",         1, { VAL_STR }, funcRmFSOStr },

	/* 65 */
	{ "STAT$",          1, { VAL_STR }, funcStatStr },
	{ "LSTAT$",         1, { VAL_STR }, funcStatStr },
	{ "CHMOD$",         2, { VAL_STR, VAL_NUM }, funcChModStr },
	{ "UMASK",          1, { VAL_NUM }, funcUmask },
	{ "CANREAD",        1, { VAL_NUM }, funcCanRW },

	/* 70 */
	{ "CANWRITE",       1, { VAL_NUM }, funcCanRW },
	{ "SELECT",         3, { VAL_UNDEF, VAL_UNDEF, VAL_NUM }, funcSelect },
	{ "RAND",           0, { VAL_UNDEF }, funcRand },
	{ "RANDOM",         1, { VAL_NUM }, funcRandom },
	{ "TIME",           0, { VAL_UNDEF }, funcTime },

	/* 75 */
	{ "DATE$",          2, { VAL_NUM, VAL_STR }, funcDateStr },
	{ "DATETOSECS",     2, { VAL_STR, VAL_STR }, funcDateToSecs },
	{ "POPEN",          2, { VAL_STR, VAL_STR }, funcPopen },
	{ "FORK",           0, { VAL_UNDEF }, funcFork },
	{ "EXEC",           2, { VAL_STR, VAL_UNDEF }, funcExec },

	/* 80 */
	{ "WAITPID$",       1, { VAL_NUM }, funcWaitCheckStr },
	{ "CHECKPID$",      1, { VAL_NUM }, funcWaitCheckStr },
	{ "KILL",           2, { VAL_NUM, VAL_NUM }, funcKill },
	{ "PIPE",          -1, { VAL_UNDEF }, funcPipe },
	{ "CONNECT",       -1, { VAL_STR }, funcConnect },

	/* 85 */
	{ "LISTEN",         2, { VAL_NUM, VAL_NUM }, funcListen },
	{ "ACCEPT",         1, { VAL_NUM }, funcAccept },
	{ "GETIP$",         1, { VAL_NUM }, funcGetIPStr },
	{ "IP2HOST$",       1, { VAL_STR }, funcIP2HostStr },
	{ "HOST2IP$",       1, { VAL_STR }, funcHost2IPStr },

	/* 90 */
	{ "GETUSERBYID$",   1, { VAL_NUM }, funcGetUserStr },
	{ "GETGROUPBYID$",  1, { VAL_NUM }, funcGetGroupStr },
	{ "GETUSERBYNAME$", 1, { VAL_STR }, funcGetUserStr }, 
	{ "GETGROUPBYNAME$",1, { VAL_STR }, funcGetGroupStr },
	{ "GETENV$",        1, { VAL_STR }, funcGetEnvStr },

	/* 95 */
	{ "SETENV",         2, { VAL_STR, VAL_STR }, funcSetEnv },
	{ "SYSTEM",         1, { VAL_STR }, funcSystem },
	{ "SYSINFO$",       1, { VAL_STR }, funcSysInfoStr },
	{ "CRYPT$",         3, { VAL_STR, VAL_STR, VAL_STR }, funcCryptStr },
	{ "LPAD$",          3, { VAL_STR, VAL_STR, VAL_NUM }, funcPadStr },

	/* 100 */
	{ "RPAD$",          3, { VAL_STR, VAL_STR, VAL_NUM }, funcPadStr },
	{ "NUMSTRBASE",     1, { VAL_STR }, funcNumStrBase },
	{ "PATH$",          1, { VAL_STR }, funcPathStr },
	{ "HAVEDATA",       0, { VAL_UNDEF }, funcHaveData },
	{ "REGMATCH",       2, { VAL_STR,VAL_STR }, funcRegMatch },

	/* 105 */
	{ "EXP",            1, { VAL_NUM }, funcExp },
	{ "EXP2",           1, { VAL_NUM }, funcExp },
	{ "EXP10",          1, { VAL_NUM }, funcExp }
};
#else
extern st_func function[NUM_FUNCTIONS];
#endif

/******************************** OPERATORS ********************************/

enum
{
	/* 0 */
	OP_COMMA,
	OP_COLON,
	OP_SEMI_COLON,
	OP_L_BRACKET,
	OP_R_BRACKET,

	/* 5 */
	OP_NOT,
	OP_AND,
	OP_OR,
	OP_XOR,
	OP_EQUALS,

	/* 10 */
	OP_NOT_EQUALS,
	OP_GREATER_EQUALS,
	OP_LESS_EQUALS,
	OP_GREATER,
	OP_LESS,

	/* 15 */
	OP_ADD,
	OP_SUB,
	OP_MULT,
	OP_DIV,
	OP_INT_DIV,

	/* 20 */
	OP_BIT_AND,
	OP_BIT_OR,
	OP_BIT_XOR,
	OP_BIT_COMPL,
	OP_LEFT_SHIFT,

	/* 25 */
	OP_RIGHT_SHIFT,
	OP_MOD,
	OP_HASH,
	OP_AT,

	NUM_OPS
};


#ifdef MAINFILE
st_op op_info[NUM_OPS] =
{
	/* 0 */
	{ ",",   0 },
	{ ":",   0 },
	{ ";",   0 },
	{ "(",   0 },
	{ ")",   0 },

	/* 5 */
	{ "NOT", 2 },
	{ "AND", 1 },
	{ "OR",  1 },
	{ "XOR", 1 },
	{ "=",   2 },

	/* 10 */
	{ "<>",  2 },
	{ ">=",  2 },
	{ "<=",  2 },
	{ ">",   2 },
	{ "<",   2 },

	/* 15 */
	{ "+",   3 },
	{ "-",   3 },
	{ "*",   4 },
	{ "/",   4 },
	{ "\\",  4 },

	/* 20 */
	{ "&",   5 },
	{ "|",   5 },
	{ "^",   5 },
	{ "~",   0 },
	{ "<<",  5 },

	/* 25 */
	{ ">>",  5 },
	{ "%",   6 },
	{ "#",   0 },
	{ "@",   0 }
};
#else
extern st_op op_info[NUM_OPS];
#endif

/******************************** MISC ENUMS ******************************/

enum
{
	VAR_STD,
	VAR_MAP,
	VAR_MEM,

	NUM_VAR_TYPES
};


enum
{
	NOT_NUM,
	NUM_BIN,
	NUM_OCT,
	NUM_HEX,
	NUM_INT,
	NUM_FLOAT,

	NUM_NUM_TYPES
};


enum
{
	TOK_UNDEF,
	TOK_VAR,
	TOK_STR,
	TOK_NUM,
	TOK_OP,
	TOK_COM,
	TOK_FUNC,
	TOK_DEFEXP
};


enum
{
	TRACING_OFF,
	TRACING_NOSTEP,
	TRACING_STEP
};


enum
{
	/* 0 */
	ESC_K,
	ESC_J,
	ESC_UP_ARROW,
	ESC_DOWN_ARROW,
	ESC_LEFT_ARROW,

	/* 5 */
	ESC_RIGHT_ARROW,
	ESC_INSERT,
	ESC_DELETE,
	ESC_PAGE_UP,

	/* 10 */
	ESC_PAGE_DOWN,
	ESC_CON_F1,
	ESC_CON_F2,
	ESC_CON_F3,
	ESC_CON_F4,
	
	/* 15 */
	ESC_CON_F5,
	ESC_TERM_F1,
	ESC_TERM_F2,
	ESC_TERM_F3,
	ESC_TERM_F4,

	/* 20 */
	ESC_TERM_F5,

	NUM_ESC_SEQS
};


enum
{
	ERR_GOTO,
	ERR_GOSUB,
	BRK_GOTO,
	BRK_GOSUB,
	TERM_GOTO,
	TERM_GOSUB,

	NUM_ON_JUMPS
};

/*********************************** GLOBALS *********************************/

EXTERN struct termios saved_tio;
EXTERN st_keybline *keyb_line;

EXTERN st_progline *prog_first_line;

EXTERN st_runline *on_jump[NUM_ON_JUMPS];
EXTERN st_runline *return_stack[MAX_RETURN_STACK];
EXTERN st_runline *prog_new_runline;
EXTERN st_runline *interrupted_runline;
EXTERN st_runline *data_runline;
EXTERN st_runline *data_autorestore_runline;

EXTERN st_var *first_var[MAX_UCHAR+1];
EXTERN st_var *last_var[MAX_UCHAR+1];
EXTERN st_var *build_options_var;
EXTERN st_var *error_var;
EXTERN st_var *syserror_var;
EXTERN st_var *reserror_var;
EXTERN st_var *prog_line_var;
EXTERN st_var *error_line_var;
EXTERN st_var *break_line_var;
EXTERN st_var *data_line_var;
EXTERN st_var *eof_var;
EXTERN st_var *pid_var;
EXTERN st_var *ppid_var;
EXTERN st_var *uid_var;
EXTERN st_var *gid_var;
EXTERN st_var *term_cols_var;
EXTERN st_var *term_rows_var;
EXTERN st_var *run_arg_var;
EXTERN st_var *angle_mode_var;
EXTERN st_var *processes_var;
EXTERN st_var *kilobyte_var;
EXTERN st_var *indent_var;
EXTERN st_var *strict_mode_var;
EXTERN st_var *interrupted_var;

EXTERN st_defexp *first_defexp;
EXTERN st_defexp *last_defexp;

EXTERN st_label *first_label[MAX_UCHAR+1];
EXTERN st_label *last_label[MAX_UCHAR+1];

EXTERN pid_t *process_list;

EXTERN char *sorted_commands[NUM_COMMANDS];
EXTERN char *sorted_functions[NUM_FUNCTIONS];
EXTERN char *defmod[DEFMOD_SIZE];
EXTERN char **watch_vars;
EXTERN char *cmdline_run_arg;
EXTERN char build_options[100];
EXTERN char autoline_str[100];

EXTERN int kilobyte;
EXTERN int processes_cnt;
EXTERN int tracing_mode;
EXTERN int strict_mode;
EXTERN int num_keyb_lines;
EXTERN int data_pc;
EXTERN int return_stack_cnt;
EXTERN int next_keyb_line;
EXTERN int keyb_lines_free;
EXTERN int basic_argc_start;
EXTERN int term_rows;
EXTERN int term_cols;
EXTERN int indent_spaces;
EXTERN int last_signal;
EXTERN int watch_alloc;
EXTERN int watch_cnt;
EXTERN int autoline_step;
EXTERN u_int autoline_curr;

/* Should have combined these into a struct but too much code to change to 
   make it worth doing now */
EXTERN int stream[MAX_STREAMS];
EXTERN u_char stream_flags[MAX_STREAMS];

EXTERN FILE *popen_fp[MAX_STREAMS];
EXTERN DIR *dir_stream[MAX_DIR_STREAMS];

EXTERN st_flags flags;

/**************************** FORWARD DECLARATIONS **************************/

/* keyboard.c */
void initKeyboard(void);
void saneMode(void);
bool getLine(char **line);
void clearKeyLine(int l);
void addDefModStrToKeyLine(
        st_keybline *line, int index, bool write_stdout, bool insert);
void addCharToKeyLine(
	st_keybline *line, char c, bool write_stdout, bool insert);
void addWordToKeyLine(char *word);
void delCharFromKeyLine(st_keybline *line, bool move_cursor);
void drawKeyLine(int l);
void leftCursor(st_keybline *line);
void rightCursor(st_keybline *line);
int  getEscapeSeq(char *seq, int len);

/* tokeniser.c */
st_progline *tokenise(char *line);
void deleteRunLine(st_runline *runline, bool force);
void deleteTokenFromRunLine(st_runline *runline, int from);

/* program.c */
void subInitProgram(void);
void initOnSettings(void);
void resetProgram(void);
bool processProgLine(st_progline *progline);
st_progline *getProgLine(u_int linenum);
void initProgram(void);
void deleteProgram(void);
void deleteProgLine(st_progline *progline, bool update_ptrs, bool force);
bool deleteProgLineByNum(u_int linenum);
bool deleteProgLines(u_int from, u_int to);
void setNewRunLine(st_runline *runline);
int  loadProgram(char *filename, u_int merge_linenum, int comnum, bool verbose);
int  listProgram(FILE *fp, u_int from, u_int to, bool pause);
int  moveProgLine(u_int from, u_int to);
int  renameProgVarsAndDefExps(st_token *fromtok, char *to, int *cnt);

/* execute.c */
bool execProgLine(st_progline *progline);
bool execRunLine(st_runline *runline);

/* variables.c */
void initVariables(void);
void createSystemVariables(int argc, char **argv, char **env);
void resetSystemVariables(void);
void setTermVariables(void);
int  getOrCreateTokenVariable(st_token *token);
int  reDimArray(st_var *var, int index_cnt, int *index);
st_var *createVariable(char *name, int type, int index_cnt, int *index);
st_var *getVariable(st_token *tok);
int getVarIndexes(
	st_runline *runline, int *pc, st_value *icnt_or_key, int *index);
int getVarValue(
	st_var *var, st_value *icnt_or_key, int *index, st_value *result);
int setVarValue(
	st_var *var,
	st_value *icnt_or_key, int *index, st_value *value, bool force);
int deleteMapKeyValue(st_var *var, char *key);
st_keyval *findKeyValue(st_var *var, char *key);
void deleteVariable(st_var *var, st_runline *runline);
void deleteVariables(st_runline *runline);
void renameVariable(st_var *var, char *new_name);
int  dumpVariables(FILE *fp, char *pat, bool dump_contents);
void dumpVariable(FILE *fp, st_var *var, bool dump_contents);

/* expressions.c */
int evalExpression(st_runline *runline, int *pc, st_value *result);

/* values.c */
void initValue(st_value *val);
bool initMemValue(st_value *val, int size);
void clearValue(st_value *val);
void setValue(st_value *val, int type, char *sval, double dval);
void setDirectStringValue(st_value *val, char *sval);
void copyValue(st_value *to, st_value *from);
void appendStringValue(st_value *val1, st_value *val2);
void subtractStringValue(st_value *val1, st_value *val2);
void multStringValue(st_value *val1, int cnt);
bool trueValue(st_value *val);

/* functions.c */
int callFunction(st_runline *runline, int *pc, st_value *result);

/* path.c */
int  matchPath(int type, char *pat, char *matchpath, bool toplevel);
bool hasWildCards(char *path);
int  appendPath(char *to, char *add);

/* defexp.c */
void initDefExps(void);
int createDefExp(st_runline *runline);
st_defexp *getDefExp(char *name, int len);
void renameDefExp(st_defexp *exp, char *new_name);
void deleteDefExp(st_runline *runline, bool force);
void deleteDefExps(void);
int  dumpDefExps(FILE *fp, char *pat);
void dumpDefExp(FILE *fp, st_defexp *exp);

/* defkeys.c */
void initDefMods(void);
void clearDefMods(void);
void addDefMod(int c, char *str);
void dumpDefMods(void);

/* watch.c */
void initWatchVars(void);
int  findWatchVar(char *varname);
int  addWatchVar(char *varname);
int  removeWatchVar(char *varname);
void printWatchVars(void);
void clearWatchVars(void);
void printWatchLet(st_var *var, char *key, int arrpos, st_value *value);

/* procs.c */
void initProcessList(void);
void addProcessToList(pid_t pid);
void removeProcessFromList(pid_t pid);
void killChildProcesses(void);
void createProcessArray(void);

/* draw.c */
void locate(int x, int y);
void drawLine(int x1, int y1, int x2, int y2, char *str, int slen);
void drawRect(int x, int y, int width, int height, int fill, char *str, int slen);
void drawCircle(
	int x, int y,
	int x_radius, int y_radius, int fill, char *str, int slen);
void drawString(int x, int y, int len, char *str, int slen);

/* argv.c */
int  splitStringIntoArgv(char *str, char ***b_argv);
void freeArgv(int b_argc, char **b_argv);

/* strings.c */
int   numType(char *str);
bool  copyStr(char *to, char *from, int max_len);
void  toUpperStr(char *str);
void  toLowerStr(char *str);
bool  wildMatch(char *str, char *pat, bool case_sensitive);
char *addFileExtension(char *filename);

/* labels.c */
void initLabels(void);
int  createLabels(st_progline *progline);
void deleteProgLineLabels(st_progline *progline);
void deleteAllLabels(void);
void clearLabels(void);
st_label *getLabel(char *name, int len);
int  dumpLabels(FILE *fp, char *pat);

/* misc.c */
int    validName(char *name);
int    qsortCompare(const void *p1, const void *p2);
void   doError(int err, st_progline *progline);
double getCurrentTime(void);
void   printTrace(int linenum, char *type, char *name);
void   ready(void);
void   prompt(void);
char   pressAnyKey(char *msg);
void   setAutoLineStr(void);
void   doExit(int code);
