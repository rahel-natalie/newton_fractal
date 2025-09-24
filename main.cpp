#include "newton_ispc.h"

#include <complex>
#include <fstream>
#include <iostream>
#include <numbers>
#include <string_view>
#include <vector>

#include <cmath>
#include <cstdint>

#include <raylib.h>


using namespace std::complex_literals;


struct image_data
{
	image_data(unsigned width, unsigned height):
		width{width}, height{height}
	{
		pixels.resize(width*height*4);
	}
	
	unsigned width{};
	unsigned height{};
	
	std::vector<std::uint8_t> pixels{};
};

void zoom(ispc::area& a, float factor)
{
	const auto x_range = a.upper_x - a.lower_x;
	const auto x_center = a.lower_x+x_range/2.0;

	a.lower_x = x_center-x_range*factor/2.0;
	a.upper_x = x_center+x_range*factor/2.0;
	
	const auto y_range = a.upper_y - a.lower_y;
	const auto y_center = a.lower_y+y_range/2.0;

	a.lower_y = y_center-y_range*factor/2.0;
	a.upper_y = y_center+y_range*factor/2.0;
};

std::complex<float> function(const std::complex<float>& z, std::size_t n)
{
	return std::complex<float>{std::pow(z,static_cast<float>(n)) - std::complex<float>{1}};
}

std::complex<float> derivative(const std::complex<float>& z, std::size_t n)
{
	return std::complex<float>{std::complex<float>(n, 0) * std::pow(z,static_cast<float>(n-1))};
}

Color to_raylib(ispc::color clr)
{
	return {clr.r, clr.g, clr.b, 255};
}

Color calculate_single_pixel(std::complex<float> z, const std::vector<ispc::float2>& roots, const std::vector<ispc::color>& colors)
{
	constexpr auto max_iteration = 42;
	for(std::size_t i=0; i<max_iteration; ++i)
	{
		constexpr auto tolerance = 0.000001;
		const auto deriv = derivative(z,roots.size());
		if(abs(deriv)<=tolerance) break;
		z -= function(z,roots.size()) / deriv;
		
		for (std::size_t i=0; i<roots.size(); i++)
		{
			const auto difference = z - std::complex<float>{roots[i].v[0], roots[i].v[1]};
			if(abs(difference)<tolerance)
			{
				const auto brightness_factor = (-2.0*i)/max_iteration + 0.5;
				return ColorBrightness(to_raylib(colors[i]), brightness_factor);
			}
		}	
	}
	return DARKGREEN;
}

void calculate_pixels(image_data& img_data, const ispc::area& target_area, const std::vector<ispc::float2>& roots, const std::vector<ispc::color>& colors)
{
	for(std::size_t y=0; y<img_data.height; ++y)
	{
		for(std::size_t x=0; x<img_data.width; ++x)
		{
			const auto fraction_x = static_cast<float>(x)/img_data.width;
			const auto range_x = target_area.upper_x-target_area.lower_x;
			const auto zx = fraction_x*range_x + target_area.lower_x;
			
			const auto fraction_y = static_cast<float>(y)/img_data.height;
			const auto range_y = target_area.upper_y-target_area.lower_y;
			const auto zy = fraction_y*range_y + target_area.lower_y;
			
			std::complex<float> z = zx + zy*1if;
			
			const auto pixel_color = calculate_single_pixel(z, roots, colors);
			
			img_data.pixels[(y*img_data.width+x) * 4] = pixel_color.r;
			img_data.pixels[(y*img_data.width+x) * 4+1] = pixel_color.g;
			img_data.pixels[(y*img_data.width+x) * 4+2] = pixel_color.b;
			img_data.pixels[(y*img_data.width+x) * 4+3] = pixel_color.a;
		}
	}
}

std::vector<ispc::float2> calculate_roots(std::size_t n)
{
	std::vector<ispc::float2> result(n);
	for(int i=0; i<n; ++i)
	{
		const auto root = std::exp(1if*((2.0f*std::numbers::pi_v<float>*i)/n));
		result[i] = ispc::float2{root.real(), root.imag()};
	}
	return result;
}

std::vector<ispc::color> set_colors(std::size_t n)
{
	std::vector<ispc::color> result
	{
		{255, 109, 194, 255},
		{200, 122, 255, 255},
		{135, 60, 190, 255},
		{112, 31, 126, 255},
		{0, 82, 172, 255}
	};
	result.resize(n);
	
	ispc::color current = {245, 109, 194};
	for(int i=5; i<n; ++i)
	{
		if((i%3)==0) current.r = (current.r+100)%255;
		if((i%3)==1) current.g = (current.g+100)%255;
		if((i%3)==2) current.b = (current.b+100)%255;
		result[i] = current;
	}
	return result;
}

void show_image_on_screen(Image& result, image_data& img_data, ispc::area& target_area, const auto fun)
{
	constexpr auto zoom_in_factor = 0.9;
	constexpr auto zoom_out_factor = 1.1;
	auto movement_step = 0.1;
	
	InitWindow(img_data.width, img_data.height, "Newton Fractal");
	SetTargetFPS(60);
	auto texture = LoadTextureFromImage(result);
	while(!WindowShouldClose())
	{
		bool should_recompute = false;
		if(IsKeyDown(KEY_UP))
		{
			zoom(target_area, zoom_in_factor);
			movement_step *= zoom_in_factor;
			should_recompute = true;
		}
		if(IsKeyDown(KEY_DOWN))
		{
			zoom(target_area, zoom_out_factor);
			movement_step *= zoom_out_factor;
			should_recompute = true;
		}
		if(IsKeyDown(KEY_W))
		{
			target_area.lower_y -= movement_step;
			target_area.upper_y -= movement_step;
			should_recompute = true;
		}
		if(IsKeyDown(KEY_S))
		{
			target_area.lower_y += movement_step;
			target_area.upper_y += movement_step;
			should_recompute = true;
		}
		if(IsKeyDown(KEY_A))
		{
			target_area.lower_x -= movement_step;
			target_area.upper_x -= movement_step;
			should_recompute = true;
		}
		if(IsKeyDown(KEY_D))
		{
			target_area.lower_x += movement_step;
			target_area.upper_x += movement_step;
			should_recompute = true;
		}
		if(should_recompute)
		{
		   fun(target_area);
		   UnloadTexture(texture);
		   texture = LoadTextureFromImage(result);
		}
		
		BeginDrawing();
			ClearBackground(BLACK);
			DrawTexturePro
			(
				texture,
				{0,0,static_cast<float>(texture.width),static_cast<float>(texture.height)},
				{0,0,static_cast<float>(img_data.width),static_cast<float>(img_data.height)},
				{0,0},
				0,
				WHITE
			); 
		EndDrawing();
	}
}

int main(int argc, char* argv[])
{
	if(argc<2)
	{
		std::cerr << "Please enter a natural number when executing this program. For example: ./newton 5\n";
		return -1;
	}
	const int n = std::atoi(argv[1]);
	if(n<=0) return -1;
	
	const auto roots = calculate_roots(n);
	const auto colors = set_colors(n);
	
	constexpr auto width = 512;
	constexpr auto height = 512;
	image_data img_data(width, height);
	ispc::area target_area{-2.0f,2.0f,-2.0f,2.0f};
	
	Image result
	{
		.data = img_data.pixels.data(),
		.width = static_cast<int>(width), 
		.height = static_cast<int>(height),
		.mipmaps = 1,
		.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
	};
	
	
	const auto ispc_pixel_calculation_fun = [&](ispc::area target_area)
	{
		ispc::calculate_pixels(ispc::root_info{roots.data(),static_cast<uint32_t>(n),colors.data()}, img_data.pixels.data(), target_area, width, height);
	};
	const auto cpp_pixel_calculation_fun = [&](ispc::area target_area)
	{
		calculate_pixels(img_data, target_area, roots, colors);
	};
	
	if(argc==3 && std::string_view{argv[2]}=="C++")
	{
		cpp_pixel_calculation_fun(target_area);
		ExportImage(result, "newton_fractal.png");
		show_image_on_screen(result, img_data, target_area, cpp_pixel_calculation_fun);
	}
	else
	{
		ispc_pixel_calculation_fun(target_area);
		ExportImage(result, "newton_fractal.png");
		show_image_on_screen(result, img_data, target_area, ispc_pixel_calculation_fun);
	}
}
