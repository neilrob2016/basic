#include "globals.h"


void locate(int x, int y)
{
	char ansi[31];
	snprintf(ansi,30,"\033[%d;%dH",y,x);
	PRINT(ansi,strlen(ansi));
}




/*** slen passed seperately instead of using strlen() for efficiency ***/
void drawString(int x, int y, char *str, int slen)
{
	if (x > 0 && y > 0 && x <= term_cols && y <= term_rows)
	{
		locate(x,y);
		PRINT(str,slen);
	}
}




/*** Includes floating point and integer drawing because I'm not sure which
     is faster. FP code is simpler but less accurate. ***/
void drawLine(int x1, int y1, int x2, int y2, char *str, int slen)
{
	int cnt;
	int adx;
	int ady;
	int dx;
	int dy;
	int i;
#ifdef LINE_FLOAT_ALGO
	double x;
	double y;
	double xadd;
	double yadd;
#else
	int xadd;
	int yadd;
	int x;
	int y;
#endif
	dx = x2 - x1;
	dy = y2 - y1;
	adx = abs(dx);
	ady = abs(dy);

	/* Avoid divide by zero */
	if (!adx && !ady) return;

	x = x1;
	y = y1;
#ifdef LINE_FLOAT_ALGO
	if (adx > ady)
	{
		xadd = SGN(dx);
		yadd = (double)dy / adx;
		cnt = adx;
	}
	else
	{
		yadd = SGN(dy);
		xadd = (double)dx / ady;
		cnt = ady;
	}
	for(i=0;i < cnt;++i)
	{
		drawString(x,y,str,slen);
		x += xadd;
		y += yadd;
	}
#else
	xadd = SGN(dx);
	yadd = SGN(dy);
	cnt = 0;
	if (adx > ady)
	{
		for(i=0;i <= adx;++i)
		{
			/* A one line way of doing the same thing which
			   is less efficient hence not used:
			   y = y1 + dy * abs(x - x1) / adx;
			 */
			drawString(x,y,str,slen);
			x += xadd;
			if ((cnt += ady) >= adx)
			{
				cnt %= adx;
				y += yadd;
			}
		}
	}
	else for(i=0;i <= ady;++i)
	{
		/* As above: x = x1 + dx * abs(y - y1) / ady; */
		drawString(x,y,str,slen);
		y += yadd;
		if ((cnt += adx) >= ady)
		{
			cnt %= ady;
			x += xadd;
		}
	}
#endif
}




void drawRect(
	int x, int y, int width, int height, int fill, char *str, int slen)
{
	int xw;
	int yh;

	xw = x + width;
	yh = y + height;

	if (fill)
	{
		for(;y < yh;++y) drawLine(x,y,xw,y,str,slen);
		return;
	}
	drawLine(x,y,xw,y,str,slen);
	drawLine(xw,y,xw,yh,str,slen);
	drawLine(xw,yh,x,yh,str,slen);
	drawLine(x,yh,x,y,str,slen);
}




/*** More complex than the previous algo but writes less to the terminal ***/
void drawCircle(
	int x, int y, int x_radius, int y_radius, int fill, char *str, int slen)
{
	double ang;
	int xp1;
	int xp2;
	int yp;
	int prev_xp = 0;
	int prev_yp = 0;
	int i;

	for(ang=0;ang < 180;++ang)
	{
		xp1 = (double)x + sin((ang + 180)/ DEGS_PER_RADIAN) * x_radius;
		xp2 = (double)x + sin(ang / DEGS_PER_RADIAN) * x_radius;
		yp = (double)y + cos(ang / DEGS_PER_RADIAN) * y_radius;

		if (xp1 == prev_xp && yp == prev_yp) continue;
		prev_xp = xp1;
		prev_yp = yp;

		if (fill)
		{
			if (slen == 1)
			{
				/* Single character fill */
				locate(xp1,yp);
				for(i=xp1;i < xp2;++i) PRINT(str,1);
			}
			else for(i=xp1;i <= xp2 - slen;++i)
			{
				locate(i,yp);
				PRINT(str,slen);
			}
		}
		else
		{
			locate(xp1,yp);
			PRINT(str,slen);
			locate(xp2 - slen,yp);
			PRINT(str,slen);
		}
	}
}
