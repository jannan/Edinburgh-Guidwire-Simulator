#include "util.h"


float distance3f(cv::Point3f p1, cv::Point3f p2)
{

	float d1;

	float dx, dy, dz;
	dx = p1.x - p2.x;
	dy = p1.y - p2.y;
	dz = p1.z - p2.z;
	d1 = sqrt((dx * dx) + (dy * dy) + (dz * dz));

	return d1;
}


void bubblesort_float(float arr[], int n)
{
	bool swapped = true;
	int j = 0;
	float tmp;
	while (swapped) {
		swapped = false;
		j++;
		for (int i = 0; i < n - j; i++) {
			if (arr[i] > arr[i + 1]) {
				tmp = arr[i];
				arr[i] = arr[i + 1];
				arr[i + 1] = tmp;
				swapped = true;
			}
		}
	}
}


void bubblesort_lineseg(lineseg arr[], int n)
{
	bool swapped = true;
	int j = 0;
	lineseg tmp;
	while (swapped) {
		swapped = false;
		j++;
		for (int i = 0; i < n - j; i++) {
			if (arr[i].dist > arr[i + 1].dist) {
				tmp = arr[i];
				arr[i] = arr[i + 1];
				arr[i + 1] = tmp;
				swapped = true;
			}
		}
	}
}



//these functions will be required:

//compute the centre and radius of a sphere fit to 3d point data

//compute centroid from point data
XMFLOAT3 find_centroid(vector<XMFLOAT3>& points)
{
	XMFLOAT3 cent = { 0,0,0 };
	for each (XMFLOAT3 point in points)
	{
		cent.x += point.x;
		cent.y += point.y;
		cent.z += point.z;
	}
	cent.x /= points.size();
	cent.y /= points.size();
	cent.z /= points.size();

	return cent;
}

//compute plane nomral from point data
bool fitplane(vector<XMFLOAT3> points, XMFLOAT3& dir, XMFLOAT3& center)
{
	if (points.size() < 3) return false;

	center = find_centroid(points);

	//matrix
	//let mut xx = 0.0; let mut xy = 0.0; let mut xz = 0.0;
	//let mut yy = 0.0; let mut yz = 0.0; let mut zz = 0.0;

	double xx = 0.f, xy = 0.f, xz = 0.f;
	double yy = 0.f, yz = 0.f, zz = 0.f;

	for each (XMFLOAT3 point in points)
	{
		XMFLOAT3 r = { point.x - center.x, point.y - center.y, point.z - center.z };
		xx += (double)r.x * (double)r.x;
		xy += (double)r.x * (double)r.y;
		xz += (double)r.x * (double)r.z;
		yy += (double)r.y * (double)r.y;
		yz += (double)r.y * (double)r.z;
		zz += (double)r.z * (double)r.z;
	}

	double det_x = yy*zz - yz*yz;
	double det_y = xx*zz - xz*xz;
	double det_z = xx*yy - xy*xy;

	double det_max = max(det_x, max(det_y, det_z));
	if (det_max == 0.0) return false;	// , "The points don't span a plane");


	// Pick path with best conditioning:
	if (det_max == det_x)
	{
		double a = (xz*yz - xy*zz) / det_x;
		double b = (xy*yz - xz*yy) / det_x;
		dir = { 1.f, (float)a, (float)b };
	}
	else if (det_max == det_y)
	{
		double a = (yz*xz - xy*zz) / det_y;
		double b = (xy*xz - yz*xx) / det_y;
		dir = { (float)a, 1.f,(float)b };
	}
	else
	{
		double a = (yz*xy - xz*yy) / det_z;
		double b = (xz*xy - yz*xx) / det_z;
		dir = { (float)a, (float)b, 1.f };
	};

	//normalise dir
	XMVECTOR out = XMVector3Normalize(XMLoadFloat3(&dir));
	XMStoreFloat3(&dir, out);

	return true;
}



//compute an ortho projection of point q onto
//plane defined by point p and normal n
XMFLOAT3 pointtoplane(XMFLOAT3 q, XMFLOAT3 p, XMFLOAT3 n)
{
	XMVECTOR normal = XMVector3Normalize(XMLoadFloat3(&n));		//make sure normal is unit value and load
	XMVECTOR Q = XMLoadFloat3(&q);
	XMVECTOR P = XMLoadFloat3(&p);

	XMVECTOR result = Q - (XMVector3Dot((Q - P), normal) * normal);

	XMFLOAT3 output = { 0,0,0 };
	XMStoreFloat3(&output, result);
	return output;
}

//compute circle from point data - radius, normal, offset
//this function takes a vector of points and returns best fit circle
//center, radius and direction vector
bool fitCircle(vector<XMFLOAT3> points, XMFLOAT3& center, XMFLOAT3& dir, float& radius)
{
	//1 -> find best fit plane and align all points onto that plane - this is direction sorted
	XMFLOAT3 centroid = { 0,0,0 };
	fitplane(points, dir, centroid);	//find the least squares best fit plane
	//if (display)geom->AddSphere(centroid, 0.2f, COLOUR_PURPLE);	//show centroid

																//test the fit by drawing a line
																// now find plane normal and centroid.
	XMFLOAT3 dir_line_vector = { dir.x * 50, dir.y * 50, dir.z * 50 };
	dir_line_vector.x += centroid.x;
	dir_line_vector.y += centroid.y;
	dir_line_vector.z += centroid.z;
	//if (display)geom->AddLine(centroid, dir_line_vector, COLOUR_PURPLE);

	//translate to plane facing camera to drop z-axis
	//XMVECTOR at = XMVectorSet(0.f, 0.f, 0.f, 0.f);
	XMVECTOR at = XMLoadFloat3(&centroid);
	XMVECTOR to = XMVectorSet(dir.x + centroid.x, dir.y + centroid.y, dir.z + centroid.z, 0.f);
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);	//for now just y axis but will need modified to avoid gimbal lock
	XMMATRIX m = XMMatrixLookAtLH(at, to, up);
	m *= XMMatrixTranslation(0.f, 0.f, 200.f);
	XMFLOAT3 newpt = { 0.f, 0.f, 0.f };
	vector<XMFLOAT3> points_transformed;
	for each (XMFLOAT3 point in points)
	{
		XMVECTOR pt = XMVector3Transform(XMLoadFloat3(&point), m);
		XMStoreFloat3(&newpt, pt);
		newpt.z = 200.f;	//drop the z component to ensure coplanar
		point = newpt;		//overwrite point
							//if(display)geom->AddSphere(newpt, 0.1f, COLOUR_YELLOW);
		points_transformed.push_back(newpt);
	}

	//try the openCV fit ellipse
	Mat pointsf;	//array of points
	vector<XMFLOAT3> pt;
	for each(XMFLOAT3 p in points_transformed)
	{
		XMFLOAT3 t;
		t.x = p.x;
		t.y = p.y;
		pt.push_back(t);
	}
	//Mat(pt).convertTo(pointsf, CV_32F);
	RotatedRect box = fitEllipse(pt);
	radius = (box.size.width + box.size.height) / 4;
	//if (display)geom->AddSphere(XMFLOAT3(box.center.x, box.center.y, 200.f), 0.1f, COLOUR_YELLOW);


	//the above opencv fit ellipse function gives us an approximate staring location
	//from here need to use minimise least squares to reduce the error
	//problem is opencv is fitting an elipse not a circle, so only works well for a full circle


	XMFLOAT2 cent = { box.center.x, box.center.y };
	leastSquares(points_transformed, cent, radius, 300.f, 60.f);
	//and again
	leastSquares(points_transformed, cent, radius, 10.f, 20.f);
	//and one last time to get really accurate
	leastSquares(points_transformed, cent, radius, 1.f, 10.f);
	//another time??

	//if (display) geom->AddSphere(XMFLOAT3(cent.x, cent.y, 200.f), 0.1f, COLOUR_BLUE);


	//draw the circle to illustrate
	vector<XMFLOAT3> circle;
	float angle = 0.f;
	float x = cent.x + (radius * cos(angle));
	float y = cent.y + (radius * sin(angle));
	circle.push_back(XMFLOAT3(x, y, 0.f));
	for (float angle = 0.1f; angle < (3.14159f * 2.f); angle += 0.2f)
	{
		float xold = x;
		float yold = y;
		x = cent.x + (radius * cos(angle));
		y = cent.y + (radius * sin(angle));
		//		if (display)geom->AddLine(XMFLOAT3(xold, yold, 200.f), XMFLOAT3(x, y, 200.f), COLOUR_YELLOW);
		circle.push_back(XMFLOAT3(x, y, 0.f));
	}

	//transform circle points back to original space
	//build transform

	XMVECTOR up2 = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	XMVECTOR org2 = XMVectorSet(0.f, 0.f, 0.f, 0.f);
	XMVECTOR out2 = XMLoadFloat3(&dir);
	XMMATRIX r2 = XMMatrixTranspose(XMMatrixLookAtLH(org2, out2, up2));
	XMMATRIX trans2 = XMMatrixTranslation(centroid.x, centroid.y, centroid.z);
	XMMATRIX m1 = r2 * trans2;

	vector<XMFLOAT3> circle2;
	for each(XMFLOAT3 c in circle)
	{
		XMVECTOR p = XMVector3Transform(XMLoadFloat3(&c), m1);
		XMFLOAT3 tmp = { 0,0,0 };
		XMStoreFloat3(&tmp, p);
		circle2.push_back(tmp);
		//if (display)geom->AddSphere(tmp, 0.1f, COLOUR_YELLOW);
	}




	//transform the center back to original space
	XMVECTOR c_vec = XMVector3Transform(XMLoadFloat3(&XMFLOAT3(cent.x, cent.y, 0.f)), m1);
	XMStoreFloat3(&center, c_vec);
	//if (display)geom->AddSphere(center, 0.05f, COLOUR_YELLOW);

	return true;
}


void leastSquares(vector<XMFLOAT3>& points_transformed, XMFLOAT2& c, float& radius, float search_width, float steps)
{
	//try a very basic method to find center to nearest 0.5
	//use the estimated center
	//5search within a pre-defined area for the global cost function minimum
	//float search_height = 50.f;
	float x1 = c.x - search_width / 2;
	float y1 = c.y - search_width / 2;
	float x2 = c.x + search_width / 2;
	float y2 = c.y + search_width / 2;
	float xsteps = steps;// 50;	//set to search width for every square mm
	float ysteps = steps;// 50;
	float xstep = (x2 - x1) / xsteps;
	float ystep = (y2 - y1) / ysteps;

	double err_min = 1000000000.f;
	float x_min = 0.f;
	float y_min = 0.f;
	float r_min = 0.f;

	for (float y = y1; y < y2; y += ystep)
	{
		for (float x = x1; x < x2; x += xstep)
		{
			double r_sample = 0.f;
			for each (XMFLOAT3 p in points_transformed)
			{
				double di = sqrt(((p.x - x)*(p.x - x)) + ((p.y - y)*(p.y - y)));
				r_sample += di;
			}
			r_sample /= points_transformed.size();

			//get error
			double err = 0.f;
			for each (XMFLOAT3 p in points_transformed)
			{
				double di = sqrt(((p.x - x)*(p.x - x)) + ((p.y - y)*(p.y - y)));
				double err_tmp = (di - r_sample) * (di - r_sample);
				err += err_tmp;
			}

			//err is sum of squares of error based on current circle
			if (err < err_min)
			{
				err_min = err;
				x_min = x;
				y_min = y;
				r_min = (float)r_sample;
			}
		}
	}

	c.x = x_min;
	c.y = y_min;
	radius = r_min;

	return;
}

//find least squares sphere within search window specied at step interval
void LeastSquaresSphere(vector<XMFLOAT3>& points, float& radius, XMFLOAT3& c, float search_width, float steps)
{
	//try a very basic method to find center to nearest 0.5
	//use the estimated center
	//5search within a pre-defined area for the global cost function minimum
	//float search_height = 50.f;
	float x1 = c.x - search_width / 2;
	float y1 = c.y - search_width / 2;
	float x2 = c.x + search_width / 2;
	float y2 = c.y + search_width / 2;
	float z1 = c.z - search_width / 2;
	float z2 = c.z + search_width / 2;

	float xsteps = steps;// 50;	//set to search width for every square mm
	float ysteps = steps;// 50;
	float xstep = (x2 - x1) / xsteps;
	float ystep = (y2 - y1) / ysteps;
	float zstep = (z2 - z1) / ysteps;

	double err_min = 1000000000.f;
	float x_min = 0.f;
	float y_min = 0.f;
	float z_min = 0.f;
	float r_min = 0.f;

	for (float z = z1; z < z2; z += zstep)
	{
		for (float y = y1; y < y2; y += ystep)
		{
			for (float x = x1; x < x2; x += xstep)
			{
				double r_sample = 0.f;
				for each (XMFLOAT3 p in points)
				{
					double di = sqrt(((p.x - x)*(p.x - x)) + ((p.y - y)*(p.y - y)) + ((p.z - z) * (p.z - z)));
					r_sample += di;
				}
				r_sample /= points.size();
				//r_sample = 250;	//debug - fix radius
				//get error
				double err = 0.f;
				for each (XMFLOAT3 p in points)
				{
					double di = sqrt(((p.x - x)*(p.x - x)) + ((p.y - y)*(p.y - y)) + ((p.z - z) * (p.z - z)));
					double err_tmp = (di - r_sample) * (di - r_sample);
					err += err_tmp;
				}

				//err is sum of squares of error based on current circle
				if (err < err_min)
				{
					err_min = err;
					x_min = x;
					y_min = y;
					z_min = z;
					r_min = (float)r_sample;
				}
			}
		}
	}

	c.x = x_min;
	c.y = y_min;
	c.z = z_min;
	radius = r_min;

	return;

}




//try to get minimum distance between 2 lines using directX functions built in
bool intersect2Rays(XMFLOAT3 line1_dir, XMFLOAT3 line1_pos, XMFLOAT3 line2_dir, XMFLOAT3 line2_pos, XMFLOAT3 &intersection)
{
	XMVECTOR n = XMLoadFloat3(&line1_dir);

	XMVECTOR LineDir = XMVector3Normalize(XMLoadFloat3(&line2_dir));
	XMVECTOR plane_dir = XMVector3Normalize(XMLoadFloat3(&line1_dir));

	XMVECTOR angle = XMVector3AngleBetweenNormals(LineDir, plane_dir);
	if (angle.m128_f32[0] < (1 * 3.14159f / 180.f))
	{
		//quit and return false if angle between 2 lines < 1 degree
		return false;
	}

	XMVECTOR LinePoint1 = XMLoadFloat3(&line2_pos);
	XMVECTOR LinePoint2 = LinePoint1 +  (LineDir * 300.f);


	XMVECTOR line_start = XMLoadFloat3(&line1_pos);


	float min_d = 99999 * 99999; // pow(1.f, 10.f);	//set minimum at 1 to power 10

	for (float f = 0.f; f < 200; f += 0.1f)
	{
		//update point
		line_start += (plane_dir)* f;

		//find distance
		XMVECTOR distance = XMVector3LinePointDistance(LinePoint1, LinePoint2, line_start);

		if (distance.m128_f32[0] < min_d)
		{
			//getting smaller
			min_d = distance.m128_f32[0];
		}
		else
		{
			//getting bigger
			break;
		}

	}

	XMStoreFloat3(&intersection, line_start);
	return true;
}



/*

this is from http://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
also has ray-sphere intersection

n - plane normal (dir)
p0 - plane centre - move along 1st line
l0 - 2nd line origin
l - 2nd line dir

*/


//find if and where a line intersects a plane
//true / false and where is in t
bool intersectLineAndPlane(XMFLOAT3 &plane_dir, XMFLOAT3 &plane_pos, XMFLOAT3 &line_pos, XMFLOAT3 &line_dir, float &t)
{
	//load vectors
	XMVECTOR v_n = XMVector3Normalize( XMLoadFloat3(&plane_dir));
	XMVECTOR v_p0 = XMLoadFloat3(&plane_pos);
	XMVECTOR v_l0 = XMLoadFloat3(&line_pos);
	XMVECTOR v_l = XMVector3Normalize(XMLoadFloat3(&line_dir));
	
	// assuming vectors are all normalized
	float denom = XMVector3Dot(v_n, v_l).m128_f32[0];	//all components of returned vector contain the dot product, so just use one of them

	if (denom > 1e-6) {
	XMVECTOR v_p0l0 = v_p0 - v_l0;
	t = XMVector3Dot(v_p0l0, v_n).m128_f32[0] / denom;
	return (t >= 0);
}
return false;
}

/*
//same as intersectPlane, but tests if within a certain distance
bool intersectDisk(const Vec3f &n, const Vec3f &p0, const float &radius, const Vec3f &l0, const Vec3 &l)
{
float t = 0;
if (intersectPlane(n, p0, l0, l, t)) {
Vec3f p = l0 + l * t;
Vec3f v = p - p0;
float d2 = dot(v, v);
return (sqrtf(d2) <= radius);
// or you can use the following optimisation (and precompute radius^2)
// return d2 <= radius2; // where radius2 = radius * radius
}

return false;
}
*/



//get intersections with a ray and sphere, return number of intersects
int intersectRaySphere(XMFLOAT3 sphere_org, FLOAT sphere_rad, XMFLOAT3 ray_dir, XMFLOAT3 ray_org, XMFLOAT3 &p1, XMFLOAT3 &p2)
{
	//load vetors
	XMVECTOR vSphere_org = XMLoadFloat3(&sphere_org);
	XMVECTOR vRay_org = XMLoadFloat3(&ray_org);
	//1, make sure ray_dir is unit vector
	XMVECTOR vRay_dir = XMVector3Normalize(XMLoadFloat3(&ray_dir));
	
	XMVECTOR q = XMVectorSubtract(vSphere_org, vRay_org);
	float c = XMVector3Length(q).m128_f32[0];
	float v = XMVector3Dot(q, vRay_dir).m128_f32[0];
	float d = (sphere_rad * sphere_rad) - (c * c - v * v);

	if (d < 0.f) return 0;		//no intersection

	if (d == 0.f)
	{
		//1 intersection ie tangent
		XMVECTOR intersect = vRay_org + (vRay_dir * v);
		XMStoreFloat3(&p1, intersect);
		return 1;
	}
	else
	{
		//2 intersections
		d = sqrt(d);
		XMVECTOR intersect = vRay_org + (vRay_dir * (v - d));
		XMStoreFloat3(&p1, intersect);
		intersect = vRay_org + (vRay_dir * (v + d));
		XMStoreFloat3(&p2, intersect);
		return 2;
	}

}