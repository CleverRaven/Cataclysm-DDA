#include "lighting.h"
#include <math.h>

Lighting::Lighting(const int w, const int h)
{
    this->w = w;
    this->h = h;

    light.resize(w);
    for (int i = 0; i < w; i++) {
        light[i].resize(h);
    }

    tmp.resize(w * 3);
    tmp2.resize(w * 3);
    tmpNew.resize(w);
}

void Lighting::recalculateLighting(const vvInt &objects, const double startIntensity)
{
    light.clear();
    light.resize(w);
    for (int i = 0; i < w; i++) {
        light[i].resize(h);
    }

    tmp.clear();
    tmp.resize(w * 3);
    tmp2.clear();
    tmp2.resize(w * 3);
    tmpNew.clear();
    tmpNew.resize(w);

    tmp2IntN = 0;

    int x, y;

    int raysN = w * 2;
    for (int a = -raysN / 2; a < raysN / 2; a++) { // slope
        int tmpN = 0; // n of intervals
        tmp2IntN = 0; // clear tmp2
        tmpNewIntN = 0; // clear tmpNew

        double alpha = M_PI / 4 * (2.0 * a / raysN);
        double slp = tan(alpha);
        double td = sqrt((slp * slp) + 1);

        for (y = 0; y < h; y++) {
            tmpN = merge();

            for (int i = 0; i < tmpN; i++) {
                T &t = tmp[i];
                t.b += slp;

                if (t.b < 0 || t.b + 1 >= w || t.i <= 0.0000001) {
                    t.d = true;
                }
            }

            tmp2IntN = 0; // clear tmp2
            tmpNewIntN = 0; // clear tmpNew

            for (int i = 0; i < tmpN; i++) {
                T &t = tmp[i];
                if (t.d) continue;
                int fb = (int) (t.b);
                int fe = fb + 1;
                applyLight(t, fb, y, td);
                applyLight(t, fe, y, td);
                if (objects[fb][y] == wall) {
                    cutInterval(t, fb);
                }
                if (objects[fe][y] == wall) {
                    cutInterval(t, fe);
                }
            }
            if (y < h / 2 || a % 2 == 0) {
                for (x = 0; x < w; x++) {
                    if (objects[x][y] == lightsource) { // light
                        light[x][y] += startIntensity;
                        tmpNew[tmpNewIntN++] = new T(x, (y < h / 2 ? startIntensity : startIntensity * 2));
                    }
                }
            }

            std::vector<T> tmpTmp = tmp; // swap tmp and tmp2
            tmp = tmp2;
            tmp2 = tmpTmp;
            tmp2IntN = tmpN;
        }
    }
}

void Lighting::applyLight(const T &t, const int x, const int y, const double td)
{
    light[x][y] += max(min(t.b + t.l, x + 1) - max(t.b, x), 0) * t.i * td;
}

void Lighting::cutInterval(T &t, const int x)
{
    double e = t.b + t.l;
    if (e <= x + 1 && e > x) {
        t.l = x - t.b;
    }
    if (t.b >= x && t.b < x + 1) {
        double b = x + 1;
        t.l -= b - t.b;
        t.b = b;
    }
    if (t.l <= 0) {
        t.d = true;
    }
}

int Lighting::merge()
{
    int i = 0;
    int j = 0;
    int n = 0;

    j = 0;
    T tj;
    for (i = 0; i < tmp2IntN; i++) {
        T ti = tmp2[i];
        if (ti.d) continue;
        while (j < tmpNewIntN && (tj = tmpNew[j]).b <= ti.b) {
            n = merge1(tj, n);
            j++;
        }
        n = merge1(tmp2[i], n);
    }

    while (j < tmpNewIntN) {
        n = merge1(tmpNew[j], n);
        j++;
    }
    return n;
}

int Lighting::merge1(T &t, int n)
{
    if (n > 0) {
        T &t2 = tmp[n - 1];
        double b = min(t.b, t2.b);
        double e = max(t.b + t.l, t2.b + t2.l);
        double l = e - b;
        if (l <= 1) {
            t2.b = b;
            t2.l = l;
            t2.i = (t.i * t.l + t2.i * t2.l) / l;
            return n;
        } else if (l <= 2 && t.b - t2.b - t2.l <= 0.5) {
            double l1 = l / 2;
            double i = (t2.i * t2.l + t.i * t.l) / l;
            t2.b = b;
            t2.l = l1;
            t2.i = i;

            t.b = t2.b + l1;
            t.l = l1;
            t.i = i;
        }
    }
    tmp[n++] = t;
    return n;
}

double Lighting::getLight(const int x, const int y) const
{
    return light[x][y];
}

double Lighting::max(const double a, const double b)
{
    return a > b ? a : b;
}

double Lighting::min(const double a, const double b)
{
    return a < b ? a : b;
}
