#pragma once
#ifndef LIGHTING_H
#define LIGHTING_H

#include <array>
#include <algorithm>

#define N_LIGHTING 24

class Lighting
{
    public:
        typedef std::array< std::array<int, N_LIGHTING>, N_LIGHTING> aaInt;
        typedef std::array< std::array<float, N_LIGHTING>, N_LIGHTING> aaFloat;

        enum en_types {
            EMPTY = 0,
            LIGHT_SOURCE,
            OBSTACLE
        };

        Lighting() {};

        /*void setInputRotated(aaInt &input, Rotation rotation);
        void accumulateLightRotatated(aaFloat &dst, Rotation rotation);*/
        void recalculateLighting( const aaInt &input, const float startIntensity );

        float getLight( const int y, const int x );

        static float max( const float a, const float b );
        static float min( const float a, const float b );

    private:

        struct Beam {
            /**
                * (beginning) start coordinate of the beam
                */
            float b;

            /**
                * (intensity) brightness of the beam
                */
            float i;

            /*
                * (length) width of the beam
                * important invariant:
                *  0 < `l` <= 1
                * */
            float l;

            /**
                * (deleted) whether beam is marked for deletion
                */
            bool d;

            Beam() : Beam( 0, 0 ) {};

            Beam( const float b, const float i ) {
                this->b = b;
                this->l = 1;
                this->i = i;
                this->d = false;
            }

            void set( const float b, const float i ) {
                this->b = b;
                this->l = 1;
                this->i = i;
                this->d = false;
            }

            void setFrom( const Beam &other ) {
                this->b = other.b;
                this->i = other.i;
                this->l = other.l;
                this->d = other.d;
            }
        };

        /**
            * N — field size (field is NxN tiles)
            */
        static const int fieldSize = N_LIGHTING;

        /**
            * Resulting brightness of the tiles, bigger is brighter
            * 0 - no illumination
            */
        aaFloat brightness;

        std::array<Beam, N_LIGHTING * 3> currentBeamsBuffer;
        std::array<Beam, N_LIGHTING * 3> nextBeamsBuffer;
        std::array<Beam, N_LIGHTING> newlyAddedBeamsBuffer;

        int newlyAddedBeamsBufferN;

        /*
            * Number of beams to cast
            * */
        static const int beamsN = N_LIGHTING * 2;

        /**
            *
            */
        std::array<bool, N_LIGHTING> lightPresent;

        /**
            * Input field (fieldSize x fieldSize)
            */
        //aaInt input;

        float applyLight( const Beam &t, const int x, const int y, const float td );

        void cutBeam( Beam &t, const int x );

        int merge( const int nextBeamsBufferN );
        int merge1( Beam &t, int n );
};

#endif
