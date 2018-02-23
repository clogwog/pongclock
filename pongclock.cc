#include "led-matrix.h"
#include "graphics.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <functional>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <ctime>
#include <signal.h>
#include <syslog.h>
#include <sys/time.h>

using namespace std;

using rgb_matrix::GPIO;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;


volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
	 syslog( LOG_NOTICE, "interrupt handler ");
	  interrupt_received = true;
}

//canvas->Fill(0, 0, 255); // R G B
// usleep(1 * 1000);  // wait a little to slow down things.


static void DrawLine(Canvas *c, int x0, int y0, int x1, int y1, int r, int g, int b)
{
	int dy = y1 - y0, dx = x1 - x0, gradient, x, y, shift = 0x10;
  	if (abs(dx) > abs(dy)) 
	{
		// x variation is bigger than y variation
		if (x1 < x0) 
		{
	      		std::swap(x0, x1);
	      		std::swap(y0, y1);
	      	}
	      	gradient = (dy << shift) / dx ;
	      
	        for (x = x0 , y = 0x8000 + (y0 << shift); x <= x1; ++x, y += gradient) {
	      		c->SetPixel(x, y >> shift, r, g, b);
	      	}
	} 
	else if (dy != 0) 
	{
		// y variation is bigger than x variation
	      	if (y1 < y0) 
		{
	      		std::swap(x0, x1);
	      		std::swap(y0, y1);
	      	}
	      	gradient = (dx << shift) / dy;
	      	for (y = y0 , x = 0x8000 + (x0 << shift); y <= y1; ++y, x += gradient) 
		{
	      		c->SetPixel(x >> shift, y, r, g, b);
	      	}
	} 
	else 
	{
		c->SetPixel(x0, y0, r, g, b);
	}
}
class Ball;
class Paddle
{
	public:
		Paddle(Canvas* canvas, int x, int y, int width, int height, int bright)
		{
			this->x = x;
			this->y = y;
			this->width = width;
			this->height = height;
			this->canvas = canvas;
			this->bright = bright;
		}
		int x;
		int y;
		int width;
		int height;
		int bright = 180;
		int currbright = bright;
		Canvas* canvas;

		bool CheckIfTouchBall(int xball, int yball)
		{
			bool retValue = false;

			if( (xball >= x && xball <= ( x + width)) &&
		   	    (yball >= y && yball <= ( y + height)))
				retValue = true;	


			return retValue;
		}
		void Flash()
		{
			currbright = 255; 
		}

		void Write()
		{
			//DrawLine(canvas, x,y,x+width, y			, bright,bright,bright);
			DrawLine(canvas, x+width,y, x+width, y+height	, currbright,currbright,currbright);	
			//DrawLine(canvas, x+width, y+height, x, y+height	, bright,bright,bright);
			DrawLine(canvas, x, y+height, x,y 		, currbright, currbright, currbright);

			currbright = bright; // reset
		}

		void Up(int steps = 1)
		{
			for( int t=0 ; t < steps; t++)
			{
				if( y > 0 )
				{
					DrawLine(canvas, x+width,y+height,x,y+height, 0,0,0);
					y--;
					Write();
				}
			}
		}
		void Down(int steps = 1)
		{
			for(int t=0; t < steps; t++)
			{
				if ( y + height < 31 )
				{
					DrawLine(canvas, x,y, x+width,y, 0,0,0);
					y++;
					Write();
				}
			}
		}
};
struct BallBounced
{
  bool leftBounce = false;
  bool rightBounce = false;
};
class Ball
{
	public:
		Ball(Canvas* canvas, int size)
		{
			this->canvas = canvas;
			this->size = size;
			x = rand() % 28 + 3;
			y = rand() % 31;

			dir_x = rand_FloatRange( -0.5, 0.5);
			while( abs( dir_x) < 0.1)
				dir_x = rand_FloatRange( -0.5, 0.5);
			dir_y = rand_FloatRange( -0.5, 0.5);
			while ( abs( dir_y) < 0.1)
				dir_y = rand_FloatRange( -0.5, 0.5);
			speed = 2.0;
		}


		
		Canvas* canvas;
		float x;
		float y;
		int size;

		float dir_x;
		float dir_y;
		float speed;
		int bright = 200;

		float addRandomnessToBounce(float oldDirection)
		{
			float retValue = oldDirection;
			if( oldDirection < 0.0 )
			{
				retValue += rand_FloatRange( -0.1, 0.1);
				while ( retValue >= -0.1 || retValue <= -0.7)
			    		retValue += rand_FloatRange( -0.1, 0.1);
			}
			return retValue;
		}

		BallBounced CalculateNextStep(bool changeInHourDue, bool changeInMinuteDue)
		{
			BallBounced retValue;

			//clear old ball
			canvas->SetPixel(x,y,0,0,0);

			x += dir_x * speed;
			y += dir_y * speed;

			// bounce at top and bottom
			if( y >=31 || y <= 0)
			{
				dir_y = -dir_y; // pure bounce

				if( dir_y < 0.0)
					dir_y = addRandomnessToBounce(dir_y);
			}

			// bounce at left and right
			int minLeft = changeInMinuteDue ? 1 : 2;
			if( x <= minLeft )
			{
				dir_x = -dir_x; // bounce
				if( changeInMinuteDue)
				{
					retValue.leftBounce = true;
					x = 16; // start from the middle line
				}
			}	
			int maxRight = changeInHourDue ? 31 : 30;
			if( x >= maxRight )
			{
				dir_x = -dir_x;
				dir_x = addRandomnessToBounce(dir_x);

				if( changeInHourDue )
				{
					retValue.rightBounce = true;
					x = 16; // start from the middle line
				}
			}

			// write new ball position
			canvas->SetPixel(x,y,bright, bright, bright);

			return retValue;
		}


	private:
		float rand_FloatRange(float a, float b)
		{
			    return ((b - a) * ((float)rand() / RAND_MAX)) + a;
		}

};

void DrawCenterLine(Canvas* canvas)
{
    bool on=true;
    int count=0;
    for(int t=0; t < 32; t++)
    {
	if( on)
	{
		canvas->SetPixel(15,t, 10,10,10);
		count++;
		if( count == 2)
			on=false;
	}
	else
	{
		count--;
		if( count == 0)
			on=true;
	}
    }

}
void CalculateNextMove(Paddle& lpaddle, Paddle& rpaddle, Ball& ball, bool changeInHourDue, bool changeInMinuteDue)
{
	int numOfSteps = 1;
	if( ball.x > 16 && ball.dir_x > 0)
	{
		// move the right paddle but only if the ball is moving towards it
		if( (int)(rpaddle.y + ( 0.5 * rpaddle.height)) > (int)ball.y )
		{
			if( ( (rpaddle.y + (0.5 * rpaddle.height)) - ball.y) > rpaddle.height) numOfSteps = 2;
			changeInHourDue ? rpaddle.Down() : rpaddle.Up(numOfSteps);
		}
		else if ( (int)(rpaddle.y + (0.5 * rpaddle.height)) < (int)ball.y)
		{
			if( (ball.y - (rpaddle.y + (0.5 * rpaddle.height) )) > rpaddle.height) numOfSteps = 2;
			changeInHourDue ? rpaddle.Up() : rpaddle.Down(numOfSteps);
		}
	}
	else if ( ball.x < 16 && ball.dir_x < 0)
	{
		// move the left paddle
		if ( (int)(lpaddle.y + ( 0.5 * lpaddle.height)) > (int)ball.y)
		{
			if( ( ( lpaddle.y + (0.5 * lpaddle.height)) - ball.y ) > lpaddle.height) numOfSteps = 2;
			changeInMinuteDue ? lpaddle.Down() : lpaddle.Up(numOfSteps);
		}
		else if ( (int)(lpaddle.y + ( 0.5 * lpaddle.height)) < (int)ball.y)
		{
			if( ( ball.y - (lpaddle.y + ( 0.5* lpaddle.height))) > lpaddle.height) numOfSteps = 2;
			changeInMinuteDue ? lpaddle.Up() : lpaddle.Down(numOfSteps);
		}
			
	}
}

float normal_dist(float x, float m, float width)
{
    static const float inv_sqrt_2pi = 0.3989422804014327;
    float a = (x - m) / width;

   return inv_sqrt_2pi / width * std::exp(-0.5f * a * a);
}

void DrawSecondLine(Canvas* canvas, int milliseconds, int hour)
{
	// slowly change colour from awua to orange over the day
	float r = 0;
	float g = 255;
	float b = 255;
	float rn = 255;
	float gn = 102;
	float bn = 0;    

	float rt = (r-rn)*(hour/23.0);
	float gt = (g-gn)*(hour/23.0);
	float bt = (b-bn)*(hour/23.0);

	float rf = r-rt;
	float gf = g-gt;
	float bf = b-bt;

	// draw a normal distribution curve as the second hand
	// 0 - 59000   -> 0 .. 31
	float steps = 37.0 * milliseconds / 59999.0;

	for( int t = 0; t < 37; t ++)
	{
		float strength = normal_dist( 4.0, steps - t, 1.0) ;
		canvas->SetPixel(t,31, rf * strength  , gf * strength, bf * strength);
	}
}

int main(int argc, char *argv[]) 
{
  setlogmask( LOG_UPTO (LOG_NOTICE));
  openlog ("pongclock", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

  syslog( LOG_NOTICE, "Pongclock started pid: %d", getuid());
  GPIO io;
  if (!io.Init())
    return 1;

  srand((unsigned int)time(NULL));

  std::string font_type = "./pongnumberfont.bdf";
  rgb_matrix::Font font;
  if (!font.LoadFont(font_type.c_str())) 
  {
	cout <<  "Couldn't load font " << font_type << std::endl;
        return 1;
  }

  Canvas *canvas = new RGBMatrix(&io, 32, 1);

  DrawCenterLine(canvas);

  Paddle lpaddle(canvas,  0,10, 1, 5,  100);
  lpaddle.Write();

  Paddle rpaddle(canvas, 30,15, 1,5 ,100);
  rpaddle.Write();

  Ball ball(canvas, 1);

  char line[6];
  strcpy(line,"12");

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);
	
  rgb_matrix::Color color(100, 100, 100);
  rgb_matrix::Color changeColor(0,0,100);
  rgb_matrix::Color offColor(0,0,0);

 
  string oldMin = "";
  string oldHour = ""; 
 
  time_t t = time(0);
  struct tm* now = localtime(&t);

  oldHour = std::to_string(now->tm_hour);
  if( oldHour.length() == 1) oldHour.insert(0, 1, '0');
  oldMin = std::to_string(now->tm_min); 
  if( oldMin.length() == 1) oldMin.insert(0,1,'0');
  
  
  //canvas->Fill(255,2,2);
  bool cont = true;

  bool changeInMinuteDue = false;
  bool changeInHourDue   = false;
  int xPosHour = 4;
  int xPosMin = 16;
  
  while(cont)
  {
	//canvas->Fill(0, 0, 0);
	//canvas->Clear();
	DrawCenterLine(canvas);

	// draw ball
	BallBounced test = ball.CalculateNextStep(changeInHourDue, changeInMinuteDue); // move the ball a bit

	// draw paddles
	CalculateNextMove(lpaddle,rpaddle, ball, changeInHourDue, changeInMinuteDue);
	if( test.leftBounce )
		rpaddle.Flash();
	if ( test.rightBounce )
		lpaddle.Flash();
	lpaddle.Write();
	rpaddle.Write();

	// draw time
	t = time(0);
	now = localtime(&t);

	timeval tv;
	gettimeofday(&tv, NULL);

	int ms = tv.tv_usec / 1000;
	ms += now->tm_sec * 1000;

	DrawSecondLine(canvas, ms, now->tm_hour);



	std::string strHour = std::to_string(now->tm_hour);
	if( strHour.length() == 1) strHour.insert(0, 1, '0');
	std::string strMinute = std::to_string(now->tm_min);
	if( strMinute.length() == 1) strMinute.insert(0,1,'0');


	if( oldHour != strHour )
	       changeInHourDue = true;
	if (oldMin != strMinute )
		changeInMinuteDue = true;

	// hour
        if ( changeInHourDue)
        {
	  if(  test.rightBounce )
	  {
	      rgb_matrix::DrawText(canvas, font, xPosHour, -1 + font.baseline(), offColor, oldHour.c_str());
	      oldHour = strHour;
	      changeInHourDue = false;
	  }
	  else
	  {
	      // still write the old hour for now
	      rgb_matrix::DrawText(canvas, font, xPosHour, -1 + font.baseline(), color, oldHour.c_str());
	  }
	}
	else
	{
	  // write the current time
	  rgb_matrix::DrawText(canvas, font, xPosHour, -1 + font.baseline(), color, strHour.c_str());
	}
													    //
	// minutes
	if ( changeInMinuteDue)
	{
	     if(  test.leftBounce )
	     {
		syslog( LOG_NOTICE, " clearing ol time");
		rgb_matrix::DrawText(canvas, font, xPosMin, -1 + font.baseline(), offColor, oldMin.c_str());
		oldMin = strMinute;
		changeInMinuteDue = false;
	     }
	     else
	     {
		     // still write the old minute for now
		     rgb_matrix::DrawText(canvas, font, xPosMin, -1 + font.baseline(), color, oldMin.c_str());
	     }
	}
	else
	{
		// write the current time
		rgb_matrix::DrawText(canvas, font, xPosMin, -1 + font.baseline(), color, strMinute.c_str());
	}

	// wait a bit
	usleep(25000);
	if ( interrupt_received )
		cont = false;
  }

  syslog(LOG_NOTICE, "end of pongclock,natural end");


  canvas->Clear();
  delete canvas;
  return 0;
}

