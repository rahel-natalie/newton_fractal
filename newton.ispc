struct color
{
	uint8 r,g,b,a;
};

struct root_info
{
	const uniform float<2>* roots;
	uniform size_t number_of_roots;
	const uniform color* colors;
};

struct area
{
	float lower_x, upper_x, lower_y, upper_y;
};


float<2> complex_multiply(float<2> lhs, float<2> rhs)
{
	float<2> result = {lhs.x*rhs.x - lhs.y*rhs.y, lhs.x*rhs.y + rhs.x*lhs.y};
	return result;
}

float<2> complex_divide(float<2> lhs, float<2> rhs)
{
	float<2> result = {(lhs.x*rhs.x + lhs.y*rhs.y)/(rhs.x*rhs.x + rhs.y*rhs.y), (lhs.y*rhs.x - lhs.x*rhs.y)/(rhs.x*rhs.x + rhs.y*rhs.y)};
	return result;
}

float<2> pow(float<2> base, uint exponent)
{
	float<2> result = {1,0};
	while(exponent>0)
	{
		if((exponent%2)==0)
		{
			base = complex_multiply(base,base);
			exponent /= 2;
		}
		else
		{
			result = complex_multiply(result,base);
			exponent -= 1;
		}
	}
	return result;
}

float hypot(float<2> number)
{
	return sqrt(number.x*number.x + number.y*number.y);
}

float<2> function(float<2> z, size_t n)
{
	float<2> rhs = {1,0};
	return pow(z,n) - rhs;
}

float<2> derivative(float<2> z, size_t n)
{
	float<2> lhs = {n,0};
	return complex_multiply(lhs,pow(z,n-1));
}


color color_tone(color clr, float brightness_factor)
{
	if(brightness_factor >= 0.0f)
	{
		color brighter = {(255-clr.r)*brightness_factor + clr.r, (255-clr.g)*brightness_factor + clr.g,  (255-clr.b)*brightness_factor + clr.b, 255};
		return brighter;
	}
	
	color darker = {clr.r * (1.0f+brightness_factor), clr.g * (1.0f+brightness_factor), clr.b * (1.0f+brightness_factor), 255};
	return darker;
}

color calculate_single_pixel(const uniform root_info& info, uniform uint8 pixels[], float<2> z)
{
	const uniform size_t max_iteration = 42;
	for(uniform size_t iteration=0; iteration<max_iteration; ++iteration)
	{
		z -= complex_divide(function(z, info.number_of_roots), derivative(z, info.number_of_roots));
		for(uniform size_t i=0; i<info.number_of_roots; i++)
		{
			const float<2> difference = z - info.roots[i];
			const uniform float tolerance = 0.000001;
			if(hypot(difference)<tolerance)
			{
				const float brightness_factor = (-2.0*i)/max_iteration + 0.5;
				return color_tone(info.colors[i], brightness_factor);
			}
		}	
	}
	color background = {0, 117, 44, 255};
	return background;
}

export void calculate_pixels(const uniform root_info& info, uniform uint8 pixels[], const uniform area& target_area, uniform uint width, uniform uint height)
{
	foreach (y = 0 ... height, x = 0 ... width)
	{
		const float fraction_x = (float)x/width;
		const float range_x = target_area.upper_x-target_area.lower_x;
		const float zx = fraction_x*range_x + target_area.lower_x;
		
		const float fraction_y = (float)y/height;
		const float range_y = target_area.upper_y-target_area.lower_y;
		const float zy = fraction_y*range_y + target_area.lower_y;
		
		float<2> z = {zx, zy};
		
		const color pixel_color = calculate_single_pixel(info, pixels, z);
		pixels[(y*width+x) * 4] = pixel_color.r;
		pixels[(y*width+x) * 4+1] = pixel_color.g;
		pixels[(y*width+x) * 4+2] = pixel_color.b;
		pixels[(y*width+x) * 4+3] = pixel_color.a;
	}
}
