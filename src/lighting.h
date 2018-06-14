#pragma once
#ifndef LIGHTING_H
#define LIGHTING_H

#include <array>
#include <algorithm>

#define N_LIGHTING 100

class Lighting
{
    public:
        typedef std::array< std::array<int, N_LIGHTING>, N_LIGHTING> aaInt;
        typedef std::array< std::array<float, N_LIGHTING>, N_LIGHTING> aaFloat;

        enum en_types : int {
            OBSTACLE = -10,
            TRANSP_10,
            TRANSP_20,
            TRANSP_30,
            TRANSP_40,
            TRANSP_50,
            TRANSP_60,
            TRANSP_70,
            TRANSP_80,
            TRANSP_90,
            EMPTY = 0,
            LIGHT_10,
            LIGHT_20,
            LIGHT_30,
            LIGHT_40,
            LIGHT_50,
            LIGHT_60,
            LIGHT_70,
            LIGHT_80,
            LIGHT_90,
            LIGHT_SOURCE,
        };

        enum en_rot : int {
            ROT_NO = 0,
            ROT_CCW,
            ROT_CW,
            ROT_PI
        };

        Lighting() {};

        void setInputRotated( aaInt &input, const en_rot rotation );
        void accumulateLightRotated( Lighting &dst, const en_rot rotation );

        void recalculateLighting( const float startIntensity );

        float getLight( const int y, const int x );

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
                * (size()) width of the beam
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
        aaInt input;

        /**
            * Resulting brightness of the tiles, bigger is brighter
            * 0 - no illumination
            */
        aaFloat brightness;

        float applyLight( const Beam &t, const int x, const int y, const float td );

        void cutBeam( Beam &t, const int x );

        int merge( const int nextBeamsBufferN );
        int merge1( Beam &t, int n );

        void rotateInt_no( aaInt &dst, const aaInt &src );
        void rotateAndAddFloat_no( aaFloat &dst, const aaFloat &src );
        void rotateInt_ccw( aaInt &dst, const aaInt &src );
        void rotateAndAddFloat_ccw( aaFloat &dst, const aaFloat &src );
        void rotateInt_cw( aaInt &dst, const aaInt &src );
        void rotateAndAddFloat_cw( aaFloat &dst, const aaFloat &src );
        void rotateInt_pi( aaInt &dst, const aaInt &a );
        void rotateAndAddFloat_pi( aaFloat &dst, const aaFloat &src );
};

#endif
