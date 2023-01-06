#include "globals.h"

static int  prevKeyLine(int l);
static int  nextKeyLine(int l);
static void copyKeyLine(int from, int to);
static void drawKeyLineSection(char *from, char *to);
static void clearScreenLine(int l);
static bool copyHistoricCommand(int input_pos);

void rawMode()
{
	static int saved = 0;
	struct termios tio;

	/* Get current settings */
	tcgetattr(0,&tio);
	if (!saved)
	{
		tcgetattr(0,&saved_tio);
		saved = 1;
	}

	/* Echo off */
	tio.c_lflag &= ~ECHO;

	/* Disable canonical mode */
	tio.c_lflag &= ~ICANON;

	/* Don't strip off 8th bit */
	tio.c_iflag &= ~ISTRIP;

	/* Set buffer size to 1 byte and no delay between reads */
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 0;

	tcsetattr(0,TCSANOW,&tio);
}




void saneMode()
{
	tcsetattr(0,TCSANOW,&saved_tio);
}




bool getLine(char **line)
{
	st_keybline *kbl;
	char esc_str[MAX_ESC_SEQ_LEN+1];
	int input_pos;
	int old_pos;
	int esc_cnt;
	int seq_num;
	int c;
	bool insert;

	input_pos = old_pos = next_keyb_line;
	esc_cnt = 0;
	insert = TRUE;

	while((c = getchar()) != EOF)
	{
		/* If ^C reset */
		if (last_signal == SIGINT)
		{
			clearKeyLine(input_pos);
			esc_cnt = 0;
			last_signal = 0;
		}

		/* Deal with escape codes */
		if (esc_cnt)
		{
			esc_str[esc_cnt++] = c;
			esc_str[esc_cnt] = 0;
			if ((seq_num = getEscapeSeq(esc_str+1,esc_cnt-1)) < 0)
			{
				if (seq_num == ESC_SEQ_INVALID) esc_cnt = 0;
				continue;
			}
			esc_cnt = 0;

			switch(seq_num)
			{
			case ESC_K:
			case ESC_UP_ARROW:
				/* Move up the history. Could just update 
				   input_pos here but that messes up the 
				   command history */
				clearScreenLine(input_pos);
				old_pos = prevKeyLine(old_pos);
				copyKeyLine(old_pos,input_pos);
				drawKeyLine(input_pos);
				continue;
				
			case ESC_J:
			case ESC_DOWN_ARROW:
				/* Move down the history */
				clearScreenLine(input_pos);
				old_pos = nextKeyLine(old_pos);
				copyKeyLine(old_pos,input_pos);
				drawKeyLine(input_pos);
				continue;

			case ESC_LEFT_ARROW:
				leftCursor(&keyb_line[input_pos]);
				break;

			case ESC_RIGHT_ARROW:
				rightCursor(&keyb_line[input_pos]);
				break;

			case ESC_DELETE:
				delCharFromKeyLine(&keyb_line[input_pos],FALSE);
				break;
				
			case ESC_INSERT:
				insert = !insert;
				break;

			case ESC_PAGE_UP:
			case ESC_PAGE_DOWN:
				/* Do nothing with them for now */
				break;

			case ESC_CON_F1:
			case ESC_CON_F2:
			case ESC_CON_F3:
			case ESC_CON_F4:
			case ESC_CON_F5:
				c = 256 + (seq_num - ESC_CON_F1);
				if (defmod[(int)c])
				{
					addDefModStrToKeyLine(
						&keyb_line[input_pos],
						c,TRUE,insert);
				}
				break;
			case ESC_TERM_F1:
			case ESC_TERM_F2:
			case ESC_TERM_F3:
			case ESC_TERM_F4:
			case ESC_TERM_F5:
				c = 256 + (seq_num - ESC_TERM_F1);
				if (defmod[(int)c])
				{
					addDefModStrToKeyLine(
						&keyb_line[input_pos],
						c,TRUE,insert);
				}
				break;
			default:
				assert(0);
			}
			continue;
		}

		switch(c)
		{
		case CONTROL_D:
			doExit(0);
			assert(0);
		case ESC:
			esc_str[0] = ESC;
			esc_cnt = 1;
			continue;

		case '\n':
			kbl = &keyb_line[input_pos];
			*line = NULL;

			if (kbl->len)
			{
				if (kbl->str[0] == '!' &&
				    !copyHistoricCommand(input_pos))
				{
					errno = 0;
					PRINT("\n",1);
					doError(ERR_INVALID_HISTORY_LINE,NULL);
					clearKeyLine(input_pos);
					return TRUE;
				}
				if (keyb_lines_free > 1) --keyb_lines_free;

				*line = kbl->str;
				kbl->cursor_pos = kbl->len;
				next_keyb_line = (input_pos + 1) % num_keyb_lines;
				clearKeyLine(next_keyb_line);
			}
			PRINT("\n",1);
			return TRUE;

		case DEL1:
		case DEL2:
			delCharFromKeyLine(&keyb_line[input_pos],TRUE);
			break;

		default:
			if (defmod[(int)c])
				addDefModStrToKeyLine(&keyb_line[input_pos],c,TRUE,insert);
			else
				addCharToKeyLine(&keyb_line[input_pos],c,TRUE,insert);
		}
	}
	PRINT("\n",1);
	return FALSE;
}




void clearKeyLine(int l)
{
	st_keybline *line = &keyb_line[l];

	/* Don't free the memory - can re-use it next time without bothering
	   realloc() if its big enough */
	if (line->len)
	{
		line->str[0] = 0;
		line->len = 0;
		line->cursor_pos = 0;
	}
}




void addDefModStrToKeyLine(
	st_keybline *line, int index, bool write_stdout, bool insert)
{
	char *ptr;

	/* Loop as its easier than writing an entire new word add function to 
	   account for cursor control */
	for(ptr=defmod[index];*ptr;++ptr)
		addCharToKeyLine(line,*ptr,TRUE,insert);
}




void addCharToKeyLine(st_keybline *line, char c, bool write_stdout, bool insert)
{
	char *ptr;
	char *s;
	char *e;

	/* Ignore non printing characters */
	if (c < 32) return;

	assert(line->len <= line->alloced);

	if (line->len >= line->alloced)
	{
		line->alloced += CHAR_ALLOC;
		line->str = (char *)realloc(line->str,line->alloced+1);
		assert(line->str);
	}

	/* Do an insert, not overwrite. More useful, but more complex */
	if (insert && line->cursor_pos < line->len)
	{
		/* Shift everything up one to make space for inserted char */
		s = line->str + line->cursor_pos;
		e = line->str + line->len;
		for(ptr=e;ptr > s;--ptr) *ptr = *(ptr-1);
		*ptr = c;

		if (write_stdout) drawKeyLineSection(s,e);
		++line->cursor_pos;
		++line->len;
		return;
	}

	assert(line->cursor_pos <= line->len);

	/* Add/overwrite character into the string at the cursor position */
	line->str[line->cursor_pos++] = c;
	if (line->cursor_pos > line->len) line->str[++line->len] = 0;
	if (write_stdout) PRINT(&c,1);
}




/*** Only used by EDIT command and always uses next_keyb_line ***/
void addWordToKeyLine(char *word)
{
	st_keybline *line = &keyb_line[next_keyb_line];
	int llen;
	int slen;

	if (!(slen = strlen(word))) return;

	llen = line->len + slen;

	if (llen > line->alloced)
	{
		line->alloced = llen;
		line->str = (char *)realloc(line->str,line->alloced+1);
		assert(line->str);
		line->str[line->len] = 0;  /* In case this is first alloc */
	}
	strcat(line->str,word);
	line->len = line->cursor_pos = llen;
}




void delCharFromKeyLine(st_keybline *line, bool move_cursor)
{
	char *ptr;
	char *s;
	char *e;

	if (!line->len) return;

	/* Delete at cursor position if its not at the end */
	if (line->cursor_pos < line->len)
	{
		if (move_cursor && !line->cursor_pos) return;

		/* Shift everything down 1 character from the point of
		   deletion */
		s = line->str + line->cursor_pos - move_cursor;
		e = line->str + line->len;
		for(ptr=s;ptr < e;++ptr) *ptr = *(ptr+1);
		
		if (move_cursor) PRINT("\b",1);
		drawKeyLineSection(s,e-2);
		PRINT("\b",1);
		if (move_cursor) --line->cursor_pos;
		--line->len;
	}
	else
	{
		line->str[--line->len] = 0;
		--line->cursor_pos;
		PRINT("\b \b",3);
	}
}




void drawKeyLine(int l)
{
	st_keybline *line = &keyb_line[l];
	int i;

	if (line->len)
	{
		PRINT(line->str,line->len);
		for(i=line->len;i > line->cursor_pos;--i) PRINT("\b",1);
	}
}




void leftCursor(st_keybline *line)
{
	if (line->cursor_pos)
	{
		--line->cursor_pos;
		PRINT("\b",1);
	}
}




void rightCursor(st_keybline *line)
{
	if (line->cursor_pos < line->len)
	{
		PRINT(&line->str[line->cursor_pos],1);
		++line->cursor_pos;
	}
}




/*** Returns escape seq number or result code ***/
int getEscapeSeq(char *seq, int len)
{
	static char *esc_seq[NUM_ESC_SEQS] =
	{
		/* 0 */
		"k",
		"j",
		"[A",
		"[B",
		"[D",

		/* 5 */
		"[C",
		"[2~",
		"[3~",
		"[5~",

		/* 10 */
		"[6~",
		"[[A",
		"[[B",
		"[[C",
		"[[D",

		/* 15 */
		"[[E",
		"OP",
		"OQ",
		"OR",
		"OS",

		/* 20 */
		"[15~"
	};
	int i;

	for(i=0;i < NUM_ESC_SEQS;++i)
	{
		/* Check for full match */
		if (!strcmp(esc_seq[i],seq)) return i;

		/* Check for partial match. If we match we have an incomplete
		   but potential code. Return -1 */
		if (!strncmp(esc_seq[i],seq,len)) return ESC_SEQ_PARTIAL;
	}
	return ESC_SEQ_INVALID;
}


/********************************* STATICS **********************************/

int prevKeyLine(int l)
{
	int i;
	int pos;

	for(i=1;i < num_keyb_lines;++i)
	{
		pos = l - i;
		if (pos < 0) pos += num_keyb_lines;
		if (keyb_line[pos].len) return pos;
	}
	return l;
}




int nextKeyLine(int l)
{
	int i;
	int pos;

	for(i=1;i < num_keyb_lines;++i)
	{
		pos = (l + i) % num_keyb_lines;
		if (keyb_line[pos].len) return pos;
	}
	return l;
}




void copyKeyLine(int from, int to)
{
	st_keybline *fromline = &keyb_line[from];
	st_keybline *toline = &keyb_line[to];

	clearKeyLine(to);
	if (fromline->len)
	{
		toline->len = fromline->len;
		toline->cursor_pos = fromline->cursor_pos;
		toline->alloced = strlen(fromline->str) + 1;
		toline->str = strdup(fromline->str);
		assert(toline->str);
	}
}




void drawKeyLineSection(char *from, char *to)
{
	char *s;
	for(s=from;s <= to;++s) PRINT(s,1);
	PRINT(" ",1);  /* Overwrite last char for delete */
	for(;s > from;--s) PRINT("\b",1);
}




void clearScreenLine(int l)
{
	int i;
	PRINT("\r",1);
	prompt();
	for(i=0;i < keyb_line[l].len;++i) PRINT(" ",1);
	PRINT("\r",1);
	prompt();
}




/*** If a user does !<number> we copy the numbered historic command into the
     keyboard buffer to run ***/
bool copyHistoricCommand(int input_pos)
{
	int pos;
	int histnum;

	histnum = atoi(keyb_line[input_pos].str+1);
	pos = (next_keyb_line + keyb_lines_free + histnum - 1) % num_keyb_lines;

	if (pos < 0 || pos == input_pos || !keyb_line[pos].len) return FALSE;

	clearScreenLine(input_pos);
	copyKeyLine(pos,input_pos);
	drawKeyLine(input_pos);
	return TRUE;
}
