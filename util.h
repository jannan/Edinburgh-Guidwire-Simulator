#pragma once

#include "stereovision.h"
#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

using namespace std;


struct lineseg
{
	float dist;
	int index;
	int p1;
	int p2;
};

float distance3f(cv::Point3f, cv::Point3f);
void bubblesort_float(float arr[], int n);
void bubblesort_lineseg(lineseg arr[], int n);

XMFLOAT3 find_centroid(vector<XMFLOAT3>& points);

bool fitplane(vector<XMFLOAT3> points, XMFLOAT3 & dir, XMFLOAT3 & center);

XMFLOAT3 pointtoplane(XMFLOAT3 q, XMFLOAT3 p, XMFLOAT3 n);

bool fitCircle(vector<XMFLOAT3> points, XMFLOAT3 & center, XMFLOAT3 & dir, float & radius);
void leastSquares(vector<XMFLOAT3>& points_transformed, XMFLOAT2 & c, float & radius, float search_width, float steps);
void LeastSquaresSphere(vector<XMFLOAT3>& points, float & radius, XMFLOAT3 & c, float search_width, float steps);

bool intersect2Rays(XMFLOAT3 line1_dir, XMFLOAT3 line1_pos, XMFLOAT3 line2_dir, XMFLOAT3 line2_pos, XMFLOAT3 &intersection);

int intersectRaySphere(XMFLOAT3 sphere_org, FLOAT sphere_rad, XMFLOAT3 ray_dir, XMFLOAT3 ray_org, XMFLOAT3 & p1, XMFLOAT3 & p2);
