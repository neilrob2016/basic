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
			drawString(x1,y1,str,slen);
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
		drawString(x1,y1,str,slen);
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
