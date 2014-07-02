
# pragma once

class PerlinNoise
{
public:

	PerlinNoise( unsigned seed = 1 );

	double noise( double x ) const;

	double noise( double x, double y ) const;

	double noise( double x, double y, double z ) const;

	double octaveNoise( double x, int octaves ) const;

	double octaveNoise( double x, double y, int octaves ) const;

	double octaveNoise( double x, double y, double z, int octaves ) const;

private:

	double fade( double t ) const;

	double lerp( double t, double a, double b ) const;

	double grad( int hash, double x, double y, double z ) const;

	int p[512];
};
