/*
	This file is part of EqualizerAPO, a system-wide equalizer.
	Copyright (C) 2013  Jonas Thedering

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include "BiQuad.h"

using namespace std;

BiQuad::BiQuad(Type type, double dbGain, double freq, double srate, double bandwidthOrQOrS, bool isBandwidth)
{
    double A;
	if(type == PEAKING || type == LOW_SHELF || type == HIGH_SHELF)
		A = pow(10, dbGain / 40);
	else
		A = pow(10, dbGain / 20);
    double omega = 2 * M_PI * freq / srate;
    double sn = sin(omega);
    double cs = cos(omega);
	double alpha;
	double beta = -1;

	if (type == LOW_SHELF || type == HIGH_SHELF) // S
	{
		alpha = sn/2 * sqrt((A + 1/A) * (1/bandwidthOrQOrS - 1) + 2);
		beta = 2 * sqrt(A) * alpha;
	}
	else if(isBandwidth) // BW
		alpha = sn * sinh(M_LN2/2 * bandwidthOrQOrS * omega / sn);
	else // Q
		alpha = sn / (2 * bandwidthOrQOrS);

	double b0, b1, b2, a0, a1, a2;

	switch(type)
	{
	case LOW_PASS:
        b0 = (1 - cs) /2;
        b1 = 1 - cs;
        b2 = (1 - cs) /2;
        a0 = 1 + alpha;
        a1 = -2 * cs;
        a2 = 1 - alpha;
		break;
	case HIGH_PASS:
        b0 = (1 + cs) /2;
        b1 = -(1 + cs);
        b2 = (1 + cs) /2;
        a0 = 1 + alpha;
        a1 = -2 * cs;
        a2 = 1 - alpha;
		break;
	case BAND_PASS:
        b0 = alpha;
        b1 = 0;
        b2 = -alpha;
        a0 = 1 + alpha;
        a1 = -2 * cs;
        a2 = 1 - alpha;
		break;
	case NOTCH:
        b0 = 1;
        b1 = -2 * cs;
        b2 = 1;
        a0 = 1 + alpha;
        a1 = -2 * cs;
        a2 = 1 - alpha;
		break;
	case ALL_PASS:
		b0 = 1 - alpha;
		b1 = -2 * cs;
		b2 = 1 + alpha;
		a0 = 1 + alpha;
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case PEAKING:
        b0 = 1 + (alpha * A);
        b1 = -2 * cs;
        b2 = 1 - (alpha * A);
        a0 = 1 + (alpha / A);
        a1 = -2 * cs;
        a2 = 1 - (alpha / A);
		break;
	case LOW_SHELF:
		b0 = A * ((A + 1) - (A - 1) * cs + beta);
		b1 = 2 * A * ((A - 1) - (A + 1) * cs);
		b2 = A * ((A + 1) - (A - 1) * cs - beta);
		a0 = (A + 1) + (A - 1) * cs + beta;
		a1 = -2 * ((A - 1) + (A + 1) * cs);
		a2 = (A + 1) + (A - 1) * cs - beta;
		break;
	case HIGH_SHELF:
        b0 = A * ((A + 1) + (A - 1) * cs + beta);
        b1 = -2 * A * ((A - 1) + (A + 1) * cs);
        b2 = A * ((A + 1) + (A - 1) * cs - beta);
        a0 = (A + 1) - (A - 1) * cs + beta;
        a1 = 2 * ((A - 1) - (A + 1) * cs);
        a2 = (A + 1) - (A - 1) * cs - beta;
		break;
	}

    this->a0 = float(b0 / a0);
    this->a[0] = float(b1 / a0);
    this->a[1] = float(b2 / a0);
    this->a[2] = float(a1 / a0);
    this->a[3] = float(a2 / a0);

	x1 = 0;
	x2 = 0;
	y1 = 0;
	y2 = 0;
}

float BiQuad::gainAt(float freq, float srate)
{
    float omega = 2 * (float)M_PI * freq / srate;
    float sn = sin(omega/2.0f);
	float phi = sn * sn;
	float b0 = this->a0;
	float b1 = this->a[0];
	float b2 = this->a[1];
	float a0 = 1.0f;
	float a1 = -this->a[2];
	float a2 = -this->a[3];

	float dbGain = 10*log10( pow(b0+b1+b2, 2) - 4*(b0*b1 + 4*b0*b2 + b1*b2)*phi + 16*b0*b2*phi*phi )
		-10*log10( pow(a0+a1+a2, 2) - 4*(a0*a1 + 4*a0*a2 + a1*a2)*phi + 16*a0*a2*phi*phi );

	return dbGain;
}