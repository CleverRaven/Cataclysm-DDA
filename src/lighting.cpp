/*
The MIT License (MIT)
Copyright (c) 2018 Ivan Zaitsev (https://github.com/Aivean/efficient-2d-raycasting)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "lighting.h"
#include "game_constants.h"
#include <math.h>
#include <algorithm>

void Lighting::recalculateLighting(const float startIntensity)
{
    /* cleaning lighting */
    std::for_each(brightness.begin(), brightness.end(), [](std::array<float, N_LIGHTING> &in){
        std::fill(in.begin(), in.end(), 0.0);
    });

    bool firstPass = true;
    std::fill(lightPresent.begin(), lightPresent.end(), false);

    for (int a = -beamsN / 2; a < beamsN / 2; a++) { // slope
        int currentBeamsBufferN = 0; // n of intervals

        /* angle of the currently processed beams */
        double alpha = M_PI / 4 * (2.0 * a / beamsN);

        /*       l
                /--------/
            /| beam  /td
            b/-|------/
            slp
            If beam is a parallelogram with height=1, angle `alpha`, and base `l`,
            then `slp` is "left base", i.e. distance between the vertex and height projected on base,
            and `td` is the length of the sides of the parallelogram
        */
        float slp = (float) tan(alpha);
        float td = (float) sqrt(slp * slp + 1);

        /* "y" coordinate of the current row, assuming input is in row-first order, i.e. [y,x] */
        for (int y = 0; y < fieldSize; y++) {
            auto inputRow = input[y];
            auto &brightnessRow = brightness[y];

            /*
                * Step 1
                * move beams horizontally by slp, remove beams that are outside the field
                * */
            for (int i = 0; i < currentBeamsBufferN; i++) {
                Beam &t = currentBeamsBuffer[i];
                t.b += slp;

                if (t.b < 0 || t.b + 1 >= fieldSize || t.i <= 0.0000001) {
                    t.d = true;
                }
            }

            newlyAddedBeamsBufferN = 0; // clear newlyAddedBeamsBuffer

            /*
                * Step 2 and 3
                *
                * Add beams intensity to the corresponding tiles of the current row
                * (see {@link #applyLight(Beam, int, int, float)})
                *
                * If beam intersects with the obstacle, cut the beam accordingly.
                * */
            for (int i = 0; i < currentBeamsBufferN; i++) {
                if (currentBeamsBuffer[i].d) {
                    continue;
                }
                Beam &t = currentBeamsBuffer[i];
                int fb = (int) (t.b);
                int fe = fb + 1;
                brightnessRow[fb] += applyLight(t, fb, y, td);
                brightnessRow[fe] += applyLight(t, fe, y, td);

                if (inputRow[fb] == OBSTACLE) {
                    cutBeam(t, fb);

                } else if ( inputRow[fb] < EMPTY ) {
                    t.i *= ( 10 - inputRow[fb] * -1 ) / 10.0;
                }

                if (inputRow[fe] == OBSTACLE) {
                    cutBeam(t, fe);

                } else if ( inputRow[fe] < EMPTY ) {
                    t.i *= (10 - inputRow[fe] * -1) / 10.0;
                }
            }

            /*
                * Step 4
                *
                * If current row contains any light sources, add them as new beams
                * to the #newlyAddedBeamsBuffer
                *
                * Several optimizations here:
                * 1. `lightPresent[y]` may be precomputed for this row from the previous passes.
                *         It allows us to skip the pass entirely.
                * 2. `y < fieldSize / 2 || a % 2 == 0` for the bottom part of the fields we can
                *      cast only half of the beams without noticeable lose in precision
                *      (but intensity of each beam should be doubled to compensate)
                * */
            if ((firstPass || lightPresent[y]) && (y < fieldSize / 2 || a % 2 == 0)) {
                for (int x = 0; x < fieldSize; x++) {
                    if (inputRow[x] > EMPTY) {
                        float modIntensity = startIntensity * (inputRow[x] / 10.0);
                        brightnessRow[x] += modIntensity;
                        lightPresent[y] = true;
                        newlyAddedBeamsBuffer[newlyAddedBeamsBufferN++].set(x, (y < fieldSize / 2 ? modIntensity : modIntensity * 2));
                    }
                }
            }

            auto tmpBuffer = currentBeamsBuffer; // swap currentBeamsBuffer and nextBeamsBuffer
            currentBeamsBuffer = nextBeamsBuffer;
            nextBeamsBuffer = tmpBuffer;

            currentBeamsBufferN = merge(currentBeamsBufferN);
        }
        firstPass = false;
    }
}

/**
    * Add Beam's brightness to given tile
    *
    * @param t  current beam
    * @param x  coordinate of the tile
    * @param y  coordinate of the tile
    * @param td length of the "side" of the beam
    * @return amount of light to add
    */
float Lighting::applyLight(const Beam &t, const int x, const int, const float td)
{
    /* calculating the intersection area between current beam and tile */
    return fmax(fmin(t.b + t.l, x + 1) - fmax(t.b, x), 0) * t.i * td;
}

/**
    * Cuts the `b` and `l` of the beam
    *
    * @param t beam to cut
    * @param x coordinate of the obstacle
    */
void Lighting::cutBeam(Beam &t, const int x)
{
    float e = t.b + t.l;
    if (e <= x + 1 && e > x) {
        t.l = x - t.b;
    }
    if (t.b >= x && t.b < x + 1) {
        float b = x + 1;
        t.l -= b - t.b;
        t.b = b;
    }
    if (t.l <= 0) {
        t.d = true;
    }
}

/**
    * Merges the beams from {@link #nextBeamsBuffer} and {@link #newlyAddedBeamsBuffer}
    * into the {@link #currentBeamsBuffer}, returning the size of the resulting buffer.
    * <p>
    * As all buffers are ordered by their `b` field, merge can be done in O(N),
    * where N is the number of beams.
    * <p>
    * Also, as the number of beams is bounded when following invariants are preserved:
    * <ol>
    * <li> width of all beams is  within (0..1] interval</li>
    * <li> if the combined width of two beams and the gap between them is less than 1 they are merged into one</li>
    * <li> intersecting beams are rearranged so that they satisfy items above</li>
    * </ol>
    * Invariants listed above guarantee that the number of beams is bounded at every time.
    * Depending on concrete implementation of the merge rules there cannot be more than 2N beams.
    *
    * @param nextBeamsBufferN size of the nextBeamsBuffer
    * @return size of the resulting {@link #currentBeamsBuffer}
    */
int Lighting::merge(const int nextBeamsBufferN)
{
    int j = 0;

    /* size of the `currentBeamsBuffer` as it is being filled */
    int n = 0;

    Beam tj;

    /*
        * Takes the next beam with the smallest `b` from either `nextBeamsBuffer` or `newlyAddedBeamsBuffer`
        * and `merges` it with the last beam in the `currentBeamsBuffer`.
        */
    for (int i = 0; i < nextBeamsBufferN; i++) {
        Beam &ti = nextBeamsBuffer[i];
        if (ti.d) continue;
        while (j < newlyAddedBeamsBufferN && (tj = newlyAddedBeamsBuffer[j]).b <= ti.b) {
            n = merge1(tj, n);
            j++;
        }
        n = merge1(nextBeamsBuffer[i], n);
    }

    while (j < newlyAddedBeamsBufferN) {
        n = merge1(newlyAddedBeamsBuffer[j], n);
        j++;
    }
    return n;
}

/**
    * Implements merge rules for two beams.
    * Adds the result of the merge to {@link #currentBeamsBuffer} and
    * returns new size of the currentBeamsBuffer
    *
    * @param t beam to merge into currentBeamsBuffer
    * @param n current size of the currentBeamsBuffer
    * @return new size of the currentBeamsBuffer
    */
int Lighting::merge1(Beam &t, int n)
{
    if (n > 0) {
        Beam &t2 = currentBeamsBuffer[n - 1];
        float b = fmin(t.b, t2.b);
        float e = fmax(t.b + t.l, t2.b + t2.l);
        float l = e - b;
        if (l <= 1) {
            t2.b = b;
            t2.l = l;
            t2.i = (t.i * t.l + t2.i * t2.l) / l;
            return n;
        } else if (l < 2 && t.b - t2.b - t2.l <= 0.5) {
            float l1 = l / 2;
            float i = (t2.i * t2.l + t.i * t.l) / l;
            t2.b = b;
            t2.l = l1;
            t2.i = i;

            t.b = t2.b + l1;
            t.l = l1;
            t.i = i;
        }
    }
    currentBeamsBuffer[n++].setFrom(t);
    return n;
}

/**
    * Data class representing single Beam
    */
float Lighting::getLight(const int y, const int x)
{
    return brightness[y][x];
}

/**
    * Copies input data from input, applying the given rotation.
    * <p>
    * Input is one of the:
    * * {@link #EMPTY}
    * * {@link #LIGHT_SOURCE}
    * * {@link #OBSTACLE}
    *
    * @param input
    * @param rotation
    */
void Lighting::setInputRotated(aaInt &input, const en_rot rotation)
{
    switch((int)rotation) {
        case ROT_NO:
            rotateInt_no(this->input, input);
            break;
        case ROT_CCW:
            rotateInt_ccw(this->input, input);
            break;
        case ROT_CW:
            rotateInt_cw(this->input, input);
            break;
        case ROT_PI:
            rotateInt_pi(this->input, input);
            break;
    }
}

void Lighting::accumulateLightRotated(Lighting &dst, const en_rot rotation)
{
    switch((int)rotation) {
        case ROT_NO:
            rotateAndAddFloat_no(dst.brightness, brightness);
            break;
        case ROT_CCW:
            rotateAndAddFloat_ccw(dst.brightness, brightness);
            break;
        case ROT_CW:
            rotateAndAddFloat_cw(dst.brightness, brightness);
            break;
        case ROT_PI:
            rotateAndAddFloat_pi(dst.brightness, brightness);
            break;
    }
}

void Lighting::rotateInt_no( aaInt &dst, const aaInt &src )
{
    dst = src;
}

void Lighting::rotateAndAddFloat_no( aaFloat &dst, const aaFloat &src )
{
    int size = dst.size();
    for( int i = 0; i < size; i++ ) {
        for( int j = 0; j < size; j++ ) {
            dst[i][j] += src[i][j];
        }
    }
}

void Lighting::rotateInt_ccw( aaInt &dst, const aaInt &src )
{
    int size = src.size();
    for( int i = 0; i < size; i++ ) {
        for( int j = 0; j < size; j++ ) {
            dst[size - j - 1][i] = src[i][j];
        }
    }
}

void Lighting::rotateAndAddFloat_ccw( aaFloat &dst, const aaFloat &src )
{
    int size = dst.size();
    for( int i = 0; i < size; i++ ) {
        for( int j = 0; j < size; j++ ) {
            dst[i][j] += src[j][size - i - 1];
        }
    }
}

void Lighting::rotateInt_cw( aaInt &dst, const aaInt &src )
{
    int size = src.size();
    for( int i = 0; i < size; i++ ) {
        for( int j = 0; j < size; j++ ) {
            dst[j][size - i - 1] = src[i][j];
        }
    }
}

void Lighting::rotateAndAddFloat_cw( aaFloat &dst, const aaFloat &src )
{
    int size = dst.size();
    for( int i = 0; i < size; i++ ) {
        for( int j = 0; j < size; j++ ) {
            dst[i][j] += src[size - j - 1][i];
        }
    }
}

void Lighting::rotateInt_pi( aaInt &dst, const aaInt &a )
{
    int size = a.size();
    for( int i = 0; i < size; i++ ) {
        for( int j = 0; j < size; j++ ) {
            dst[size - i - 1][size - j - 1] = a[i][j];
        }
    }
}

void Lighting::rotateAndAddFloat_pi( aaFloat &dst, const aaFloat &src )
{
    int size = dst.size();
    for( int i = 0; i < size; i++ ) {
        for( int j = 0; j < size; j++ ) {
            dst[i][j] += src[size - i - 1][size - j - 1];
        }
    }
}
