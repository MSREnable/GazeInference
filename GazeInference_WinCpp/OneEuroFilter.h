#pragma once
/* -*- coding: utf-8 -*-
 *
 * OneEuroFilter.h -
 *
 * Author: Nicolas Roussel (nicolas.roussel@inria.fr)
 *
 * Copyright 2019 Inria
 *
 * BSD License https://opensource.org/licenses/BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions
 * and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 * and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
 * promote products derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <iostream>
#include <stdexcept>
#include <cmath>
#include <ctime>
#include <corecrt_math_defines.h>

 // -----------------------------------------------------------------
 // Utilities

void
randSeed(void) {
    srand(time(0));
}

double
unifRand(void) {
    return rand() / double(RAND_MAX);
}

typedef double TimeStamp; // in seconds

static const TimeStamp UndefinedTime = -1.0;

// -----------------------------------------------------------------

class LowPassFilter {

    double y, a, s;
    bool initialized;

    void setAlpha(double alpha) {
        if (alpha <= 0.0 || alpha > 1.0)
            throw std::range_error("alpha should be in (0.0., 1.0]");
        a = alpha;
    }

public:

    LowPassFilter(double alpha, double initval = 0.0) {
        y = s = initval;
        setAlpha(alpha);
        initialized = false;
    }

    double filter(double value) {
        double result;
        if (initialized)
            result = a * value + (1.0 - a) * s;
        else {
            result = value;
            initialized = true;
        }
        y = value;
        s = result;
        return result;
    }

    double filterWithAlpha(double value, double alpha) {
        setAlpha(alpha);
        return filter(value);
    }

    bool hasLastRawValue(void) {
        return initialized;
    }

    double lastRawValue(void) {
        return y;
    }

};

// -----------------------------------------------------------------

class OneEuroFilter {
    double freq;
    double mincutoff;
    double beta;
    double dcutoff;
    LowPassFilter* x;
    LowPassFilter* dx;
    TimeStamp lasttime;

    double alpha(double cutoff) {
        double te = 1.0 / freq;
        double tau = 1.0 / (2 * M_PI * cutoff);
        return 1.0 / (1.0 + tau / te);
    }

    void setFrequency(double f) {
        if (f <= 0) throw std::range_error("freq should be >0");
        freq = f;
    }

    void setMinCutoff(double mc) {
        if (mc <= 0) throw std::range_error("mincutoff should be >0");
        mincutoff = mc;
    }

    void setBeta(double b) {
        beta = b;
    }

    void setDerivateCutoff(double dc) {
        if (dc <= 0) throw std::range_error("dcutoff should be >0");
        dcutoff = dc;
    }

public:
    OneEuroFilter(double freq=120.0, double mincutoff = 1.0, double beta = 0.0, double dcutoff = 1.0) {
        setFrequency(freq);
        setMinCutoff(mincutoff);
        setBeta(beta);
        setDerivateCutoff(dcutoff);
        x = new LowPassFilter(alpha(mincutoff));
        dx = new LowPassFilter(alpha(dcutoff));
        lasttime = UndefinedTime;
    }

    double filter(double value, TimeStamp timestamp = UndefinedTime) {
        // update the sampling frequency based on timestamps
        if (lasttime != UndefinedTime && timestamp != UndefinedTime)
            freq = 1.0 / (timestamp - lasttime);
        lasttime = timestamp;
        // estimate the current variation per second 
        double dvalue = x->hasLastRawValue() ? (value - x->lastRawValue()) * freq : 0.0; // FIXME: 0.0 or value?
        double edvalue = dx->filterWithAlpha(dvalue, alpha(dcutoff));
        // use it to update the cutoff frequency
        double cutoff = mincutoff + beta * fabs(edvalue);
        // filter the given value
        return x->filterWithAlpha(value, alpha(cutoff));
    }

    ~OneEuroFilter(void) {
        delete x;
        delete dx;
    }
};

//----------------------------------------------------------------

class GazeFilter {
    OneEuroFilter* filterX;
    OneEuroFilter* filterY;

public:
    GazeFilter(double freq = 60.0, double mincutoff = 0.001, double beta = 0.003, double dcutoff = 0.4) {
       /* beta // Low value => low jitter, dcutoff // Higher value => low lag */
        filterX = new OneEuroFilter(freq, mincutoff, beta, dcutoff);
        filterY = new OneEuroFilter(freq, mincutoff, beta, dcutoff);
    }

    ~GazeFilter(void) {
        delete filterX;
        delete filterY;
    }

    cv::Point filter(cv::Point point, TimeStamp timestamp = UndefinedTime) {
        return cv::Point(filterX->filter(point.x), filterX->filter(point.y));
    }
};