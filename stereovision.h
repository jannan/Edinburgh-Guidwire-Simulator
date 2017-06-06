#pragma once

#include <iostream>
#include <string>   // for strings
#include <iomanip>  // for controlling float print precision
#include <sstream>  // string to number conversion
#include <time.h>

#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\calib3d\calib3d.hpp>
#include <opencv2\imgproc\imgproc.hpp>

#include "util.h"
#include "GeometryClass.h"

#include "d3dclass.h"
#include <directxmath.h>

using namespace std;
using namespace cv;

//#define CHESSBOARD_SIZE 24.4	//size of calibration chessboard size in mm
#define CHESSBOARD_SIZE 12.76	//size of calibration chessboard size in mm

#define MARKER_ID_WIRE 1
#define MARKER_ID_FEMUR 2

struct StereoPoint
{
	Point2f left;
	Point2f right;
	float disparity;
	Point3f p3d;
	int objectID;		//keep track of which marker this belongs to
	int pointID;		//which point of marker
};

struct Interconnect
{
	int p1;
	int p2;
	float distance;
	int objectID;
	Point3f pnt3_1;
	Point3f pnt3_2;
};


class StereoVision
{
public:
	StereoVision();
	StereoVision(const StereoVision&);
	~StereoVision();

	bool ison = false;

	bool Initialize(int c1, int c2, float scale, bool flipLeft, bool flipRight);
	bool Capture(XMMATRIX & wire_rot, XMMATRIX & wire_pos, XMMATRIX & model_rot, XMMATRIX & model_pos, bool & wire_found, bool & model_found, int obj);
	void GetPositions(XMFLOAT3 & l1, XMFLOAT3 & l2, XMFLOAT3 & l3, XMFLOAT3 & pos, XMFLOAT3 & dir);
	void Shutdown();
	void CVDrawText(Mat & mt, Point pt, char* chr);
	bool capture_wire(vector<Point3f>, int&, int&, int&, XMMATRIX&);	//capture wire marker

	void cal1_start();
	bool cal1_isdone();
	XMMATRIX get_dir();
	XMMATRIX get_pos();
	XMMATRIX rot_from_dir(XMFLOAT3 dir);
	void cal2_start();
	bool cal2_isdone();

	void SetAverageOn();
	void SetAverageOff();
	void ToggleAverage();

	void setGeom(GeometryClass* g);

private:
	float m_chessboard_size = 0.f;
	bool m_flipLeft = false;	//flip the image
	bool m_flipRight = false;

	void AverageLEDs();
	void SetupAverageFilter();

	void cal1_finish();
	void cal2_finish();
	bool cal1_done = false;
	bool cal2_done = false;


	const char* w_left = "wind1";
	const char* w_right = "wind2";

	VideoCapture m_cam1;
	VideoCapture m_cam2;	//two video capture devices, one for each camera
	
	Mat cameraMatrix[2], distCoeffs[2];
	Mat R, T, E, F;
	Mat R1, R2, P1, P2, Q;
	Mat rmap[2][2];

	bool initialised = false;

	XMFLOAT3 led1, led2, led3, m_pos, m_dir;

	//#define AVERAGE_NUM 6
#define AVERAGE_NUM 12
	XMFLOAT3 led1_av[AVERAGE_NUM];
	XMFLOAT3 led2_av[AVERAGE_NUM];
	XMFLOAT3 led3_av[AVERAGE_NUM];
	float led_av[AVERAGE_NUM][3];
	bool use_average = false;

#define COLLECT_DATA_NONE 0
#define COLLECT_DATA_CIRCLE 1
#define COLLECT_DATA_SPHERE 2

	char collect_data = COLLECT_DATA_NONE;	//flag set to start collecting data for something
	int datanum = 100;			//number of points to collect
	vector<XMFLOAT3> led1_dataset;
	vector<XMFLOAT3> led2_dataset;
	vector<XMFLOAT3> led3_dataset;

	XMFLOAT3 calib_offset;		//saves offset position of centre rotation
	XMFLOAT3 calib_dir;			//and calibrated direction offset


	GeometryClass* geom;
};

void bubblesort_interconnect(vector<Interconnect>& arr);
