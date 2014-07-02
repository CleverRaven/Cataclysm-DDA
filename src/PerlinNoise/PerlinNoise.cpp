# include <cmath>
# include <array>
# include <numeric>
# include <random>
# include <algorithm>
# include "PerlinNoise.hpp"

PerlinNoise::PerlinNoise( unsigned seed )
{
	// workaround for VS2012
	if(seed==0)
	{
		seed = std::mt19937::default_seed;
	}

	// p[0]..p[255] contains all numbers in [0..255] in random order		
	std::iota(std::begin(p),std::begin(p)+256,0);
	
	std::shuffle(std::begin(p),std::begin(p)+256,std::mt19937(seed));

	for(int i=0; i<256; ++i)
	{
		p[256+i] = p[i];
	}
}

double PerlinNoise::noise( double x ) const
{
	return noise(x,0.0,0.0);
}

double PerlinNoise::noise( double x, double y ) const
{
	return noise(x,y,0.0);
}

double PerlinNoise::noise( double x, double y, double z ) const
{
	const int X = static_cast<int>(::floor(x)) & 255;
	const int Y = static_cast<int>(::floor(y)) & 255;
	const int Z = static_cast<int>(::floor(z)) & 255;

	x -= ::floor(x);
	y -= ::floor(y);
	z -= ::floor(z);

	const double u = fade(x);
	const double v = fade(y);
	const double w = fade(z);

	const int A = p[X  ]+Y, AA = p[A]+Z, AB = p[A+1]+Z;
	const int B = p[X+1]+Y, BA = p[B]+Z, BB = p[B+1]+Z;

	return lerp(w, lerp(v, lerp(u, grad(p[AA  ], x  , y  , z   ),
			grad(p[BA  ], x-1, y  , z   )),
			lerp(u, grad(p[AB  ], x  , y-1, z   ),
			grad(p[BB  ], x-1, y-1, z   ))),
			lerp(v, lerp(u, grad(p[AA+1], x  , y  , z-1 ),
			grad(p[BA+1], x-1, y  , z-1 )),
			lerp(u, grad(p[AB+1], x  , y-1, z-1 ),
			grad(p[BB+1], x-1, y-1, z-1 ))));
}

double PerlinNoise::octaveNoise( double x, int octaves ) const
{
	double result = 0.0;
	double amp = 1.0;

	for(int i=0; i<octaves; ++i)
	{
		result += noise(x) * amp;
		x   *= 2.0;
		amp *= 0.5;
	}

	return result;
}

double PerlinNoise::octaveNoise( double x, double y, int octaves ) const
{
	double result = 0.0;
	double amp = 1.0;

	for(int i=0; i<octaves; ++i)
	{
		result += noise(x,y) * amp;
		x   *= 2.0;
		y   *= 2.0;
		amp *= 0.5;
	}

	return result;
}

double PerlinNoise::octaveNoise( double x, double y, double z, int octaves ) const
{
	double result = 0.0;
	double amp = 1.0;

	for(int i=0; i<octaves; ++i)
	{
		result += noise(x,y,z) * amp;
		x   *= 2.0;
		y   *= 2.0;
		z   *= 2.0;
		amp *= 0.5;
	}

	return result;
}

double PerlinNoise::fade( double t ) const
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

double PerlinNoise::lerp( double t, double a, double b ) const
{
	return a + t * (b - a);
}

double PerlinNoise::grad( int hash, double x, double y, double z ) const
{
	const int h = hash & 15;
	const double u = h<8 ? x : y, v = h<4 ? y : h==12||h==14 ? x : z;
	return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}
