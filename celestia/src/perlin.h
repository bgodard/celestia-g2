// perlin.h

#ifndef _PERLIN_H_
#define _PERLIN_H_

extern float noise(float vec[], int len);

extern float noise1(float arg);
extern float noise2(float vec[]);
extern float noise3(float vec[]);

extern float turbulence(float v[], float freq);
extern float turbulence(Point2f& p, float freq);
extern float turbulence(Point3f& p, float freq);
extern float fractalsum(float v[], float freq);
extern float fractalsum(Point2f& p, float freq);
extern float fractalsum(Point3f& p, float freq);

#endif // _PERLIN_H_
