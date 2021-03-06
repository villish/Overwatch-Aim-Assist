/*
 *Overwatch Aim Assitance
 *Copyright (C) 2016  Juan Xuereb
 *
 *This program is free software: you can redistribute it and/or modify
 *it under the terms of the GNU General Public License as published by
 *the Free Software Foundation, either version 3 of the License, or
 *(at your option) any later version.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.

 *You should have received a copy of the GNU General Public License
 *along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Screenshot.h"
#include "Time.h"

/*
		Screenshot Implementation Code
*/

Screenshot::Screenshot(int sWidth, int sHeight, int sLength, RGBQUAD *sPixels, bool pointToSPixels)
{
	width = sWidth;
	height = sHeight;
	length = sLength;

	if(pointToSPixels)
		pixels = sPixels;
	else
	{
		pixels = new RGBQUAD[length];
		memcpy(pixels, sPixels, length*(sizeof(RGBQUAD)));
	}
}

Screenshot::Screenshot(Screenshot sc, bool pointToSPixels)
{
	width = sc.width;
	height = sc.height;
	length = sc.length;

	if (pointToSPixels)
		pixels = sc.pixels;
	else
	{
		pixels = new RGBQUAD[length];
		memcpy(pixels, sc.pixels, length*(sizeof(RGBQUAD)));
	}
}

Screenshot::Screenshot()
{
}

Screenshot::~Screenshot()
{
}

void Screenshot::FreeMemory()
{
	delete[] pixels; //don't put this in destructor
}

bool Screenshot::isHealth(RGBQUAD &pixel) //pass by reference so to avoid re-allocating data that we already have in for loop
{
	if (pixel.rgbRed == HEALTH.r && pixel.rgbBlue == HEALTH.b && pixel.rgbGreen == HEALTH.g)
		return true;

	return false;
}

bool Screenshot::isRed(RGBQUAD &pixel)
{
	BYTE minR = 150;//175
	BYTE maxR = 255;//255
	BYTE minG = 0;//0
	BYTE maxG = 75;//75
	BYTE minB = 0;//0
	BYTE maxB = 90;//50
	if (pixel.rgbRed >= minR && pixel.rgbRed <= maxR &&
		pixel.rgbGreen >= minG && pixel.rgbGreen <= maxG &&
		pixel.rgbBlue >= minB && pixel.rgbBlue <= maxB)
	{
		return true;
	}
	else
		return false;
}

bool Screenshot::triggerBot()
{
	int centreWidth = width / 2;
	int centreHeight = height / 2;

	//CALCULATE THESE ONCE! not everytime tbot is called
	float percentWidth = (0.3f * (float)width) / 100.0f; //you can use int 5 pixels as default too...
	float percentHeight = (0.5f * (float)height) / 100.0f; // ^

	int startingRow = (centreHeight - (int)percentHeight)*width;
	int endRow = (centreHeight + (int)percentHeight)*width;

	for (int row = startingRow; row < endRow; row += width)
	{
		for (int col = centreWidth - percentWidth; col < centreWidth + percentWidth; col++)
		{
			int index = row  + col;
			//if (index < length && index > 0)
			//{
				//drawPixel(col, (index/1920), RGB((int)pixels[index].rgbRed, (int)pixels[index].rgbGreen, (int)pixels[index].rgbBlue));
				if (isRed(pixels[index]))
					return true;
			//}
		}
	}

	//cin.ignore();

	return false;
	
}

bool Screenshot::findPlayer(int &posX, int &posY, bool headshot)
{
	int x = 0;
	int y = 0;
	bool foundHandle = false;

	//Scans from top -> down. Move right -> reset
	for (int i = length-width; x<width; i -= width) //x is changed inside
	{
		if (isHealth(pixels[i])) //if pixel = health bar
		{
			//If adjacent pixels in the lower right direction are red => Hande of cluster Found
			int xx = i + 1; //next x
			int yy = i + width; //underlying y
			if (length >(int)xx && length > (int)yy) //is NOT out of bounds
			{
				if (isHealth(pixels[xx]) && isHealth(pixels[yy]))
					foundHandle = true;
			}

			//If adjacent pixels in upper left are NOT red => Hande of cluster Found
			xx = i - 1;
			yy = i - width;
			if (xx > 0 && yy >0) //is NOT out of bounds
			{
				if (isHealth(pixels[xx]) && isHealth(pixels[yy]))
					foundHandle = false; //THERE ARE REDS SO IT CAN'T BE HANDLE CORNER
			}

		}

		int barWidth = 0;
		const int maxTol = 15;
		int tolerance = maxTol; //tolerance till we quit searching for red bars. Is reset if a red is found to maxTol
		if (foundHandle)
		{

			//Measure health bar width
			vector<int> barWidths;
			for (int _x = i; _x < length; _x++)
			{
				if (isRed(pixels[_x]))
				{
					tolerance = maxTol;
					barWidth++;
				}
				else
				{
					if (barWidth > 0)
						barWidths.push_back(barWidth);

					barWidth = 0;
					tolerance--;
					
					if(tolerance==0)
						break;
				}
			}

			if (barWidths.size() == 0) //If no bars found, rekt shrekt and eeeerekt
			{
				return false;
			}

			int numberOfBars = 8; //IMP! FIX THIS FOR WINSTON TA RAS ZOBBI u ekk...
			if (barWidths.size() == 1) //estimation based on 1 bar is inaccurate if that 1 bar is not @ full HP
				barWidth = 150;
			if (barWidths.size() > 3)
				barWidth = calculateMedian(barWidths);
			else
				barWidth = barWidths[0];
			int healthBarWidth = (barWidth * numberOfBars) + ((numberOfBars-1)*2); //health bar area + empty pixel areas in between 
			//

			//Obtain all border points lying under the health bar
			vector<POINT> border;

			int skipYs = 40;
			int startIndex = i - (width*skipYs);
			int actualRow = y + skipYs;

			bool foundAnyRedsBefore = false;
			bool foundRedInRow = false;	
			int toleranceNoReds = (height * 5) / 100; //4% of image height

			//Assign these to 0 before we add to them.
			posX = 0;
			posY = 0;

			for (int i = startIndex; i < length; i-=width) //scan down y
			{
				foundRedInRow = false;

				for (int column = -(healthBarWidth/3); column <= healthBarWidth + (healthBarWidth/3); column++) //scan in x
				{
					int index = i + column;
					if (index < length && index > 0)
					{

						if (isRed(pixels[index]))
						{
							foundAnyRedsBefore = true;
							foundRedInRow = true;

							POINT pt;
							pt.x = x + column;
							pt.y = actualRow;
							border.push_back(pt);

							posX += pt.x; //add all x's -> for avg
							posY += pt.y; //add all y's -> for avg
						}
					}
				}

				actualRow++; //Need to dynamically update this because to move y we are using -=width; altough we can (index/width) to tabulate actualY
				if (foundRedInRow == false)
					toleranceNoReds--;

				if (toleranceNoReds == 0)
					break;
			}
			//

			if (foundAnyRedsBefore)
			{
				posX /= border.size();
				if (!headshot)
					posY /= border.size();
				else
				{
					posX = border[0].x;
					posY = border[0].y + 5; //
				}

				if (DRAW)
					debugDraw(x, y, healthBarWidth, border, posX, posY);

				return true;
			}
			else
			{
				posX = x + (healthBarWidth / 2);
				posY = y + 75;

				foundAnyRedsBefore = true;
				
				if (DRAW)
					debugDraw(x, y, healthBarWidth, border, posX, posY);

				return true;
			}
				
		}
		

		y++;
		if (i < width) //if next value is going to be OutOfRange (on looping: i-=width ==> below 0)
		{
			x++; //Move 1 pixel right
			i = length-width+x;
			y = 0; //Reset to top
		}
	}

	return false;
}

int Screenshot::calculateMedian(vector<int> &values)
{
	int median = 0;
	size_t size = values.size();

	sort(values.begin(), values.end());

	if (size % 2 == 0)
		median = (values[size / 2 - 1] + values[size / 2]) / 2;
	else
		median = values[size / 2];

	return median;
}


void Screenshot::debugDraw(int handleX, int handleY, int healthBarWidth, vector<POINT> border, int aimX, int aimY)
{
	Beep(500, 500); //beep indicates that drawing is taking place in background

	HWND hwnd = GetConsoleWindow();
	HDC hdc = GetDC(hwnd);

	if (hwnd = NULL)
		return;

	ShowWindow(GetConsoleWindow(), SW_MAXIMIZE);
	SwitchToThisWindow(hwnd, true);
	cout << "Press ENTER to draw the 'debug' image....." << endl;
	cin.ignore();

	drawReds(true);
	cin.ignore();

	drawScreenshot(true, false);
	cin.ignore();

	for (int i = 0; i <= healthBarWidth; i++)
	{
		SetPixel(hdc, handleX + i, handleY-1, RGB(0, 255, 255));
		SetPixel(hdc, handleX + i, handleY, RGB(0, 255, 255));
		SetPixel(hdc, handleX + i, handleY+1, RGB(0, 255, 255));
	}

	////Draw Border
	//for (UINT i = 0; i < border.size(); i++)
	//	SetPixel(hdc, border[i].x, border[i].y, RGB(255, 0, 0));

	////Draw Midline
	//for(int y = 0; y < 100; y++)
		//SetPixel(hdc, handleX+(healthBarWidth/2), handleY+y, RGB(100, 100,100));

	//Draw Crosshair
	for (int t = -5; t < 6; t++)
	{
		SetPixel(hdc, aimX + t, aimY, RGB(255, 255, 0));
		SetPixel(hdc, aimX, aimY + t, RGB(255, 255, 0));
	}

	cin.ignore();
	ReleaseDC(hwnd, hdc);
}

void Screenshot::drawScreenshot(bool invert, bool pixelated)
{
	HWND hwnd = GetConsoleWindow();
	HDC hdc = GetDC(hwnd);
	
	if (hwnd = NULL)
		return;

	ShowWindow(GetConsoleWindow(), SW_MAXIMIZE);
	SwitchToThisWindow(hwnd, true);

	int x = 0;
	int y = 0;
	int delta = 1;

	int modifier = 1;
	if (pixelated)
		modifier = 2;

	if (invert)
	{
		y = height;
		delta = -1;
	}

	for (int i = 0; i < length; i+= modifier)
	{
		RGBQUAD rgb = pixels[i];
		COLORREF COLOR = RGB((int)rgb.rgbRed, (int)rgb.rgbGreen, (int)rgb.rgbBlue);
		SetPixel(hdc, x, y, COLOR);

		x+= modifier;

		if (x >= width)
		{
			x = 0;
			y+=delta;
		}
	}

	ReleaseDC(hwnd, hdc);
}

void Screenshot::drawPixel(int x, int y, COLORREF col)
{
	HWND hwnd = GetConsoleWindow();
	HDC hdc = GetDC(hwnd);

	if (hwnd = NULL)
		return;

	ShowWindow(GetConsoleWindow(), SW_MAXIMIZE);
	SwitchToThisWindow(hwnd, true);

	SetPixel(hdc, x, y, col);

	ReleaseDC(hwnd, hdc);

}

void Screenshot::drawReds(bool invert)
{
	HWND hwnd = GetConsoleWindow();
	HDC hdc = GetDC(hwnd);

	if (hwnd = NULL)
		return;

	ShowWindow(GetConsoleWindow(), SW_MAXIMIZE);
	SwitchToThisWindow(hwnd, true);

	int x = 0;
	int y = 0;
	int delta = 1;

	if (invert)
	{
		y = height;
		delta = -1;
	}

	for (int i = 0; i < length; i++)
	{
		if (isRed(pixels[i]))
			SetPixel(hdc, x, y, RGB((int)pixels[i].rgbRed, (int)pixels[i].rgbGreen, (int)pixels[i].rgbBlue));
		else
			SetPixel(hdc, x, y, RGB(255,255,255));
		
		x++;
		if (x >= width)
		{
			x = 0;
			y += delta;
		}
	}


	ReleaseDC(hwnd, hdc);
}

void Screenshot::drawBlankScreenshot(bool pixelated)
{
	HWND hwnd = GetConsoleWindow();
	HDC hdc = GetDC(hwnd);

	if (hwnd = NULL)
		return;

	ShowWindow(GetConsoleWindow(), SW_MAXIMIZE);
	SwitchToThisWindow(hwnd, true);
	COLORREF COLOR = RGB(255, 255, 255);

	int modifier = 1;
	if (pixelated)
		modifier = 2;

	for (int x = 0; x < width; x+=modifier)
	{
		for (int y = 0; y < height; y+=modifier)
			SetPixel(hdc, x, y, COLOR);
	}

	ReleaseDC(hwnd, hdc);
}

bool Screenshot::isRGBEqual(RGBQUAD &r1, RGBQUAD &r2)
{
	if (r1.rgbRed == r2.rgbRed && r1.rgbGreen == r2.rgbGreen && r1.rgbBlue == r2.rgbBlue)
		return true;

	return false;
}

bool Screenshot::operator==(const Midline &line)
{
	int start = (height / 2) * width;
	int mIndex = 0;
	for (int sIndex = start; sIndex < start + width; sIndex++)
	{
		if (!isRGBEqual(line.pixels[mIndex], this->pixels[sIndex]))
			return false;

		mIndex++;
	}

	return true;
}
bool Screenshot::operator!=(const Midline &line)
{
	return !(*this == line);
}


Screenshot& Screenshot::operator=(const Screenshot &other)
{
	height = other.height;
	width = other.width;
	length = other.length;
	pixels = other.pixels; //not creating new pixel arr but simply linking value-of,address-of
	return *this;
}

bool Screenshot::operator==(const Screenshot &other)
{
	for (int i = 0; i < length; i++)
	{
		if (!isRGBEqual(other.pixels[i], this->pixels[i]))
			return false;
	}
	return true;
}

bool Screenshot::operator!=(const Screenshot &other)
{
	return !(*this == other);
}


/*  
		Midline Class Implementation Code
*/

Midline::Midline(Screenshot &sc)
{
	length = sc.width;
	pixels = new RGBQUAD[sc.width];

	int index = (sc.height / 2) * sc.width;
	int i = 0;

	for (int ii = index; ii < index + sc.width; ii++)
	{
		pixels[i] = sc.pixels[ii];
		i++;
	}
}

Midline::Midline(int sWidth, int sHeight, RGBQUAD *sPixels)
{
	length = sWidth;
	pixels = new RGBQUAD[sWidth];

	int index = (sHeight / 2) * sWidth;
	int i = 0;

	for (int ii = index; ii < index + sWidth; ii++)
	{
		pixels[i] = sPixels[ii];
		i++;
	}
}

Midline::~Midline()
{
	delete[] pixels;
}
