#include "globals.h"

static void drawMPCircle(
	int cx, int cy, int radius, int fill, char *str, int slen);
static void drawOval(
	int x, int y,
	int x_radius, int y_radius, int fill, char *str, int slen);
static void drawStringAndClip(int x, int len, char *str, int slen);


/*** Move the cursor to the given location ***/
void locate(int x, int y)
{
	char ansi[31];

	/* Clip if OOB */
	if (x < 1) x = 1;
	else if (x > term_cols) x = term_cols;

	if (y < 1) y = 1;
	else if (y > term_rows) y = term_rows;

	snprintf(ansi,30,"\033[%d;%dH",y,x);
	PRINT(ansi,strlen(ansi));
}




/*** My own line drawing algo. Bresenhams seems rather complicated and 
     inefficient with lots of multiplication ***/
void drawLine(int x1, int y1, int x2, int y2, char *str, int slen)
{
	int xadd;
	int yadd;
	int cnt;
	int i;

	int dx = x2 - x1;
	int dy = y2 - y1;
	int adx = abs(dx);
	int ady = abs(dy);

	/* Avoid divide by zero */
	if (!adx && !ady) return;

	xadd = SGN(dx);
	yadd = SGN(dy);
	cnt = 0;
	if (adx > ady)
	{
		/* Every time we increment x1 by 1 add ady to cnt. Then if
		   cnt goes over adx increment y. eg: if adx = 10 and ady = 3
		   then will inc y1 on 4th iteration. Then cnt will = 1 and 
		   next time will only increment y1 on 3rd iter. etc */
		for(i=0;i <= adx;++i)
		{
			drawString(x1,y1,slen,str,slen);
			x1 += xadd;
			if ((cnt += ady) >= adx)
			{
				cnt -= adx;
				y1 += yadd;
			}
		}
	}
	else for(i=0;i <= ady;++i)
	{
		drawString(x1,y1,slen,str,slen);
		y1 += yadd;
		if ((cnt += adx) >= ady)
		{
			cnt -= ady;
			x1 += xadd;
		}
	}
}




void drawRect(
	int x, int y, int width, int height, int fill, char *str, int slen)
{
	int x2;
	int y2;
	int slen2;

	x2 = x + width;
	y2 = y + height;

	if (fill)
	{
		for(;y < y2 && y <= term_rows;++y)
			drawString(x,y,width,str,slen);
		return;
	}
	/* Top */
	drawString(x,y,width,str,slen);

	/* Bottom */
	drawString(x,y2,width,str,slen);

	/* Left */
	if (x + slen > x2)
		slen2 = slen - (x2 - x);
	else
		slen2 = slen;
	drawLine(x,y2,x,y,str,slen2);

	/* Right */
	x2 -= slen;
	if (x2 < x)
	{
		slen -= (x - x2);
		x2 = x;
	}
	drawLine(x2,y,x2,y2,str,slen);
}




/*** Draw a circle on the terminal ***/
void drawCircle(
	int x, int y, int x_radius, int y_radius, int fill, char *str, int slen)
{
	if (x_radius == y_radius)
		drawMPCircle(x,y,x_radius,fill,str,slen);
	else
		drawOval(x,y,x_radius,y_radius,fill,str,slen);
}




/*** Use the mid point algo to draw a circle which is more efficient than
     using trig functions ***/
void drawMPCircle(int cx, int cy, int radius, int fill, char *str, int slen)
{
	double relative;
	int radius2 = radius * radius;
	int x = 0;
	int y = -radius;
	int xp1;
	int xp2;
	int yp1;
	int yp2;
	int len;
	int slen2 = slen * 2;
	
	/* Loop for a quarter circle */
	while(y <= 0)
	{
		xp1 = cx - x;
		xp2 = cx + x;
		yp1 = cy - y - 1;
		yp2 = cy + y;

		if (fill || xp2 - xp1 < slen2)
		{
			/* Fill from one side to the other */
			len = xp2 - xp1;
			drawString(xp1,yp1,len,str,slen);
			drawString(xp1,yp2,len,str,slen);
		}
		else
		{
			/* Print the character in each of the 4 quandrants */
			drawString(xp1,yp1,slen,str,slen);
			drawString(xp1,yp2,slen,str,slen);

			xp2 -= slen;
			drawString(xp2,yp1,slen,str,slen);
			drawString(xp2,yp2,slen,str,slen);
		}

		/* Find where the halfway to the next potential point is 
		   relative to the circles perimeter */
		relative = pow(x + 0.5,2) + pow(y + 1,2) - radius2;

		/* if < 0 then point is inside the circle, if its 0 then its 
		   on the perimeter and if its > 0 then its outside */
		if (relative < 0) ++x;
		else if (!relative) 
		{
			++x;
			++y;
		}
		else ++y;
	}
}




/*** Use trigonometry to draw an oval which has varying X and Y radii ***/
void drawOval(
	int x, int y, int x_radius, int y_radius, int fill, char *str, int slen)
{
	double ang;
	double radang;
	int xp1;
	int xp2;
	int yp;
	int prev_xp = 0;
	int prev_yp = 0;
	int slen2 = slen * 2;

	for(ang=0;ang < 180;++ang)
	{
		radang = ang / DEGS_PER_RADIAN;

		xp1 = (double)x + sin((ang + 180) / DEGS_PER_RADIAN) * x_radius;
		xp2 = (double)x + sin(radang) * x_radius;
		yp = (double)y + cos(radang) * y_radius;

		if (xp1 == prev_xp && yp == prev_yp) continue;
		prev_xp = xp1;
		prev_yp = yp;

		if (fill || xp2 - xp1 < slen2)
			drawString(xp1,yp,xp2-xp1,str,slen);
		else
		{
			drawString(xp1,yp,slen,str,slen);
			xp2 -= slen;
			drawString(xp2,yp,slen,str,slen);
		}
	}
}




/*** Draw/print a string at the given location for the given length ***/
void drawString(int x, int y, int len, char *str, int slen)
{
	if (x <= term_cols && y > 0 && y <= term_rows)
	{
		locate(x,y);
		drawStringAndClip(x,len,str,slen);
	}
}




/*** Draw/print a string for len bytes repeating the string as much as is
     necessary and clipping if necessary ***/
void drawStringAndClip(int x, int len, char *str, int slen)
{
	int i;
	int p;
	for(i=p=0;i < len && x <= term_cols;++i,++x,p=(p+1)%slen)
		if (x > 0) write(STDOUT,str+p,1);
}
