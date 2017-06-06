//code to implement stereo vision

#include "stereovision.h"


StereoVision::StereoVision()
{
	//m_cam1 = 0;
	//m_cam2 = 0;
}

StereoVision::StereoVision(const StereoVision& other)
{
}

StereoVision::~StereoVision()
{
}


bool StereoVision::Initialize(int c1, int c2, float scale, bool flipLeft, bool flipRight)
{
	m_flipLeft = flipLeft;
	m_flipRight = flipRight;

	m_chessboard_size = scale;

	//create windows to show raw webcam pictures
	cvNamedWindow(w_left, CV_WINDOW_AUTOSIZE);
	cvNamedWindow(w_right, CV_WINDOW_AUTOSIZE);
	
	m_cam1.open(c1);
	m_cam2.open(c2);

	//set size 1280 x 720:  //cam2.set(CV_CAP_PROP_FRAME_WIDTH, 1280); cam2.set(CV_CAP_PROP_FRAME_HEIGHT, 720);
	m_cam1.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
	m_cam1.set(CV_CAP_PROP_FRAME_HEIGHT, 720);
	m_cam2.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
	m_cam2.set(CV_CAP_PROP_FRAME_HEIGHT, 720);

	if (!m_cam1.isOpened() || !m_cam2.isOpened())
	{
		//cout << "Couldnt open devices" << endl;
		ison = false;
		return false;

	}


	//open parameters
	char charsl = '/';
	string file_intrinsics = "data\\calibration\\intrinsics.yml";
	string file_extrinsics = "data\\calibration\\extrinsics.yml";
	Mat Qx;

	FileStorage fs(file_intrinsics, FileStorage::READ);
	if (!fs.isOpened())
	{
		//cout << "ERROR: Couldn't open intrinsics file";
		return false;
	}
	fs["M1"] >> cameraMatrix[0];
	fs["D1"] >> distCoeffs[0];
	fs["M2"] >> cameraMatrix[1];
	fs["D2"] >> distCoeffs[1];
	fs.release();

	fs.open(file_extrinsics, FileStorage::READ);
	if (fs.isOpened())
	{
		fs["R"] >> R;
		fs["T"] >> T;
		fs["R1"] >> R1;
		fs["R2"] >> R2;
		fs["P1"] >> P1;
		fs["P2"] >> P2;
		fs["Q"] >> Qx;
		fs["F"] >> F;
		fs.release();
	}
	Q = Qx;

	Mat f1;
	m_cam1 >> (f1);

	//work out rectification maps
	Size imageSize(f1.cols, f1.rows);
	initUndistortRectifyMap(cameraMatrix[0], distCoeffs[0], R1, P1, imageSize, CV_16SC2, rmap[0][0], rmap[0][1]);
	initUndistortRectifyMap(cameraMatrix[1], distCoeffs[1], R2, P2, imageSize, CV_16SC2, rmap[1][0], rmap[1][1]);

	use_average = true;
	initialised = true;

	ison = true;

	return true;
}

bool StereoVision::Capture(XMMATRIX& wire_rot, XMMATRIX& wire_pos, XMMATRIX& model_rot, XMMATRIX& model_pos, bool& wire_found, bool& model_found, int obj)
{
	int thresh_level = 210;
	//Size out_size(320, 240);
	float scale = 0.5f;

	Mat f1, f2;
	vector<KeyPoint> keypoints_L, keypoints_R;

	if (!initialised) return false;

	//load camera images and rectify
	{
		m_cam1 >> f1;
		m_cam2 >> f2;

		//flip over vertically
		if (m_flipLeft) flip(f1, f1, -1);
		if (m_flipRight) flip(f2, f2, -1);

		if (!f1.empty())
		{
			remap(f1, f1, rmap[0][0], rmap[0][1], INTER_LINEAR);
		}
		else
		{
			return false;
		}

		if (!f2.empty())
		{
			remap(f2, f2, rmap[1][0], rmap[1][1], INTER_LINEAR);
		}
		else
		{
			return false;
		}
	}

	//threshold blob detect
	{
		Mat blobimg;
		SimpleBlobDetector::Params params;
		params.minDistBetweenBlobs = 10.0f;
		params.minThreshold = thresh_level;
		params.maxThreshold = 255;
		params.thresholdStep = 10;
		params.filterByInertia = false;
		params.filterByColor = true;
		params.blobColor = 255;
		params.filterByConvexity = false;
		params.filterByCircularity = false;
		params.filterByArea = false;
		params.minArea = 1.0f;
		params.maxArea = 50.0f;

		cv::Ptr<cv::SimpleBlobDetector> detector = SimpleBlobDetector::create(params);

		cvtColor(f1, blobimg, COLOR_BGR2GRAY);
		detector->detect(blobimg, keypoints_L);

		//save thresholded image
		threshold(blobimg, f1, thresh_level, 255, 0);
		
		cvtColor(f2, blobimg, COLOR_BGR2GRAY);
		detector->detect(blobimg, keypoints_R);

		threshold(blobimg, f2, thresh_level, 255, 0);
	}


	//resize
	Size out_size(f1.cols * scale, f1.rows * scale);
	resize(f1, f1, out_size);
	resize(f2, f2, out_size);

	//fail if less than 3 points detected in either camera
	if ((keypoints_L.size() < 3) || (keypoints_R.size() < 3) )
	{
		//show the images to the window
		imshow(w_left, f1); waitKey(33);
		imshow(w_right, f2); waitKey(33);
		return false;
	}

	imshow(w_left, f1); waitKey(33);
	imshow(w_right, f2); waitKey(33);

	//Pair up possible point matches
	//discard any that dont have a corresponding point on horizontal epiline (greater than MAX_ERR)
	vector<StereoPoint> kp;
	float MAX_ERR = 10.0f;	//maximum difference in pixels to accept or reject a pairing
	if (keypoints_L.size() > keypoints_R.size())
	{
		//more points on left
		for (int i = 0; i < keypoints_L.size(); i++)
		{
			bool done = false;
			int j = 0;
			//see if has a corresponding point on the right
			while (!done)
			{
				float err = abs(keypoints_R[j].pt.y - keypoints_L[i].pt.y);
				if (err < MAX_ERR)
				{
					//within 2 pixels of each other so a possible match so save in list
					StereoPoint pp;
					pp.left = keypoints_L[i].pt;
					pp.right = keypoints_R[j].pt;
					kp.push_back(pp);
				}
				j++;
				if (j > (keypoints_R.size() - 1)) done = true;
			}
		}
	}
	else
	{
		//more points on right
		for (int i = 0; i < keypoints_R.size(); i++)
		{
			bool done = false;
			int j = 0;
			//see if has a corresponding point on the right
			while (!done)
			{
				float err = abs(keypoints_L[j].pt.y - keypoints_R[i].pt.y);
				if (err < MAX_ERR)
				{
					//within 2 pixels of each other so a possible match so save in list
					StereoPoint pp;
					pp.left = keypoints_L[j].pt;
					pp.right = keypoints_R[i].pt;
					kp.push_back(pp);
				}
				j++;
				if (j > (keypoints_L.size() - 1)) done = true;
			}
		}
	}

	//fail if less than 3 matched points (cannot make a marker)
	if (kp.size() < 3)
	{
		//show the images to the window
		imshow(w_left, f1); waitKey(33);
		imshow(w_right, f2); waitKey(33);
		return false;
	}

	//have a list of possible pairings, excluding any isolated points only visible on one image
	//also have possible incorrect pairings - give incorrect depth values
	//calculate 3d positions of all pairings however then select based on distances
	vector<Point3f> pt_in, pt_out;
	for (auto i = begin(kp); i != end(kp); ++i)
	{
		float x = i->left.x; x += i->right.x; x /= 2;
		float y = i->left.y; y += i->right.y; y /= 2;
		float d = i->left.x - i->right.x;		//disparity
		pt_in.push_back( Point3f(x, y, d));
	}
	//calculate 3d coordinates from disparity and Q matrix from camera calibration
	perspectiveTransform(pt_in, pt_out, Q);	//Q matrix provides disparity to Z transform info

	
	//note - m_chessboard_size is loaded during initialise function (originally loaded by scenarioclass

	//scale 3d coordinates based on camera calibration grid size and invert Y to match directx and stl files
	for (auto i = begin(pt_out); i != end(pt_out); ++i)
	{
		i->x *= m_chessboard_size;
		i->y *= -m_chessboard_size;		//invert y
		i->z *= m_chessboard_size;
	}

	//make a list of all possible interconnections with distances between points
	vector<Interconnect> ic;
	{
		float MIN_DIST = 65.0f;		//setup for large marker
		float MAX_DIST = 110.0f;
		float MIN_Z = 200.f;		//also a filter on z depth to reduce erroneous points
		float MAX_Z = 800.f;

		int j = 0;
		for (int i = 0; i < kp.size(); ++i)
		{
			//for each keypoint pair
			j = i + 1;
			while (j < kp.size())
			{
				//check both points are within z limits
				if ((pt_out[i].z > MIN_Z) && (pt_out[i].z < MAX_Z) && (pt_out[j].z > MIN_Z) && (pt_out[j].z < MAX_Z))
				{
					//got new possible distance
					float dx = pt_out[i].x - pt_out[j].x;
					float dy = pt_out[i].y - pt_out[j].y;
					float dz = pt_out[i].z - pt_out[j].z;

					float dist = sqrt((dx * dx) + (dy * dy) + (dz * dz));

					//draw the line only if distance within limits
					if ((dist > MIN_DIST) && (dist < MAX_DIST))
					{
						//save in a list to use for next stage
						Interconnect icon;
						icon.p1 = i;
						icon.p2 = j;
						icon.distance = dist;
						icon.objectID = -1;	//not got an object yet
						icon.pnt3_1 = pt_out[i];
						icon.pnt3_2 = pt_out[j];
						ic.push_back(icon);

						Point p1 = Point(kp[i].left.x * scale, kp[i].left.y * scale);
						Point p2 = Point(kp[j].left.x * scale, kp[j].left.y * scale);
						//draw the line
						cv::line(f1, p1, p2, Scalar(80, 80, 80), 1);

						p1 = Point(kp[i].right.x * scale, kp[i].right.y * scale);
						p2 = Point(kp[j].right.x * scale, kp[j].right.y * scale);
						//draw the line
						cv::line(f2, p1, p2, Scalar(80, 80, 80), 1);
					}

				}
				++j;
			}

		}
	}

	//fail if less than 3 interconnects within range
	if (ic.size() < 3)
	{
		//show the images to the window
		imshow(w_left, f1); waitKey(33);
		imshow(w_right, f2); waitKey(33);
		return false;
	}
	
	if (ic.size() == 3)
	{
		//there are exactly 3 interconnects, which means 3 points
		//so must be a femur marker

		//return = detet_model_marker(kp, ic);
	}

	//identify separate objects
	int curr_interconnect = 0;
	int num_objects = 0;
	//start with the 1st interconnect - set to belong to object 0
	ic[0].objectID = 0;
	bool done = false;
	bool * finished = new bool[ic.size()];
	for (int i = 0; i < ic.size(); ++i){finished[i] = false;}
	while (!done)
	{
		//find the next interconnect that is in current object and not all ready completed
		if ((ic[curr_interconnect].objectID == num_objects) && !finished[curr_interconnect])
		{
			//this interconnect is part of current object.
			//see whats attached to it if anything

			//get p1
			int p1 = ic[curr_interconnect].p1;

			//done = true;	//clear found a point flag (will remain true if no points found)

			//search all interconnects for a new one (except this one!)
			for (int i = 0; i < ic.size(); ++i)
			{
				if ((i != curr_interconnect) && (ic[i].objectID < 0) && ((ic[i].p1 == p1) || (ic[i].p2 == p1)))
				{
					//this interconnect was not previously marked, and shares a point
					//so mark it
					ic[i].objectID = num_objects;
					//done = false;	//found a point - still finding points so not done yet
				}
			}

			//repeat the above for other end of interconnect
			int p2 = ic[curr_interconnect].p2;

			for (int i = 0; i < ic.size(); ++i)
			{
				if ((ic[i].objectID < 0) && ((ic[i].p1 == p2) || (ic[i].p2 == p2)))
				{
					//this interconnect was not previously marked, and shares a point
					//so mark it
					ic[i].objectID = num_objects;
					//done = false;	//found a point - still finding points so not done yet
				}
			}

			//have finished this ic - no more possible connected segments
			finished[curr_interconnect] = true;

			//start again at beginning
			curr_interconnect = 0;

		}
		else
		{
			//this interconnect was not currently marked as part of current object, or is all ready completed, so move to next
			curr_interconnect++;
			if (curr_interconnect > ic.size() - 1)
			{
				//have a roll over.
				//this means didn't find another attached unfinished interconnect so current object is complete
				//check if there is another object to find
				bool another_object = false;
				for (int i = 0; i < ic.size(); ++i)
				{
					if ((ic[i].objectID < 0) && !another_object)
					{
						//this interconnect is not yet assigned so must be in the next object
						num_objects++;
						curr_interconnect = i;
						ic[i].objectID = num_objects;	//set this as the start of the next object
						another_object = true;
					}
					if (another_object) break;
				}
				//start at beginnign again for next object
				curr_interconnect = 0;

				if (!another_object)
				{
					//no more objects to find, so we are done
					done = true;
				}
			}
		}
	}
	delete[] finished;		//free memory

	//copy point data into objects vector
	vector<vector<Interconnect>> objects;
	for (int i = 0; i < num_objects + 1; ++i)
	{
		vector<Interconnect> object;
		for (int j = 0; j < ic.size(); ++j)
		{
			if (ic[j].objectID == i)
			{
				//this interconnect is part of current object i, so add it
				object.push_back(ic[j]);
			}
		}
		if ((object.size() == 3) || (object.size() == 6))
		{
			//only copy out if has 3 or 4 points, otherwise skip this (bad) object
			objects.push_back(object);
		}
		object.clear();
	}

	wire_found = false; 	model_found = false;

	//should now have a vector of objects, each of which is valid and a vector of points
	for (auto obj = begin(objects); obj != end(objects); ++obj)
	{
		//sort interconnects in current object from low distance to high distance
		bubblesort_interconnect(*obj);

		//for each object
		if (obj->size() == 6)
		{
			//this object has 6 interconnects, so 4 points, so must be a wire marker
			//only detect if haven't all ready done (cant be 2 wire markers)
			if (!wire_found)
			{
				//found_wire = detectMarker_wire(obj);
				wire_found = true;
				//got a 4 led visible wire marker

				//now all interconnects are sorted by the distance, know last 2 are long segments
				//so can get the centre point by averaging - these are the outside 4 points of marker
				//Point3f centre = ((*obj)[4].pnt3_1 + (*obj)[4].pnt3_2 + (*obj)[5].pnt3_1 + (*obj)[5].pnt3_2) / 4;
				Point3f four = { 4.f, 4.f, 4.f };
				Point3f centre = ((*obj)[4].pnt3_1 + (*obj)[4].pnt3_2 + (*obj)[5].pnt3_1 + (*obj)[5].pnt3_2) / 4.0f;

				//next get out vector - cross product of centre to any 2 points (use a short interconnect so 90 degrees apart)
				Point3f p1 = (*obj)[0].pnt3_1 - centre;
				Point3f p2 = (*obj)[0].pnt3_2 - centre;

				//cross product v1 and v2 gives out vector
				//Point3f v_out = p1.cross(p2);
				XMVECTOR wire_dir;
				XMVECTOR v1 = XMLoadFloat3(&XMFLOAT3(p1.x, p1.y, p1.z));
				XMVECTOR v2 = XMLoadFloat3(&XMFLOAT3(p2.x, p2.y, p2.z));
				XMVECTOR vC = XMLoadFloat3(&XMFLOAT3(centre.x, centre.y, centre.z));

				//get wire normal
				wire_dir = XMVector3Cross(v1, v2);
				if (wire_dir.m128_f32[2] < 0)
				{
					wire_dir = -wire_dir;	//facing away from camera so invert
				}

				//construct rotation matrix
				XMVECTOR v_out = XMVector3Normalize(wire_dir);
				XMVECTOR org = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
				XMVECTOR up = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

				wire_rot = XMMatrixLookAtRH(org, v_out, -up);		//get rotation
				wire_rot = XMMatrixTranspose(wire_rot);
				wire_pos = XMMatrixTranslationFromVector(vC);		//add position offset

			}
		}
		else
		{
			//3 points, so could be either.  Work out which
			auto ptr = begin(*obj);
			float d1 = ptr->distance;
			float d2 = (++ptr)->distance;
			float d3 = (++ptr)->distance;

			int p1 = 0, p2 = 0, p3 = 0;

			if ((d1 + d2 + d3) > 100)
			{
				//rough way to see if more long than short
				if (!model_found)
				{
					model_found = true;
					//go to model marker orientation calculator

					//first find lead point - use as centre
					Point3f centre;
					//this is only point not on the LONGEST segment - need to change
					if ( ((*obj)[0].p1 == (*obj)[1].p1) || ((*obj)[0].p1 == (*obj)[1].p2))
					{
						//common point was [0].p1
						centre = (*obj)[0].pnt3_1;
						p1 = (*obj)[0].p1;
					}
					else
					{
						//must be [0].p2 because short 2 segments must share a point
						centre = (*obj)[0].pnt3_2;
						p1 = (*obj)[0].p2;
					}

					//find the 2nd and 3rd leds - each end of long segment
					//however, may need swapped around
					p2 = (*obj)[2].p1;
					p3 = (*obj)[2].p2;


					//work out correct order of p2 and p3 using a dot product
					XMVECTOR p1_v = XMVectorSet(pt_out[p1].x, pt_out[p1].y, pt_out[p1].z, 0.f);
					XMVECTOR p2_v = XMVectorSet(pt_out[p2].x, pt_out[p2].y, pt_out[p2].z, 0.f);
					XMVECTOR p3_v = XMVectorSet(pt_out[p3].x, pt_out[p3].y, pt_out[p3].z, 0.f);

					XMVECTOR vec1 = XMVectorSubtract(p2_v, p1_v);
					XMVECTOR vec2 = XMVectorSubtract(p3_v, p1_v);

					XMVECTOR vec_out = XMVector3Cross(vec1, vec2);

					if (vec_out.m128_f32[2] > 0)
					{
						//swap
						vec_out = -vec_out;
						XMVECTOR tmp = p2_v;
						p2_v = p3_v;
						p3_v = tmp;
						int tmp2 = p2;
						p2 = p3;
						p3 = tmp2;
					}

					//save these 3 led positions - these should now be correct so can average with previous readings
					led1.x = pt_out[p1].x; led1.y = pt_out[p1].y;	led1.z = pt_out[p1].z;
					led2.x = pt_out[p2].x; led2.y = pt_out[p2].y;	led2.z = pt_out[p2].z;
					led3.x = pt_out[p3].x; led3.y = pt_out[p3].y;	led3.z = pt_out[p3].z;

					
					//at this stage implement an average filter
					//check if wildly different to previous value, if so dont use but increment a counter
					//if counter reaches threshold, reset filter
					//allows big jumps to happen (with a lag to check valid) and reject occassional outliers
					AverageLEDs();


					//now get up vector and out vector - recalculate both from averaged led positions
					p1_v = XMLoadFloat3(&led1);
					p2_v = XMLoadFloat3(&led2);
					p3_v = XMLoadFloat3(&led3);
					vec1 = XMVectorSubtract(p2_v, p1_v);
					vec2 = XMVectorSubtract(p3_v, p1_v);
					vec_out = XMVector3Normalize(XMVector3Cross(vec1, vec2));

					//up vector is centre of long segment to lead point
					XMVECTOR vec3 = XMVectorSet((led2.x + led3.x) / 2, (led2.y + led3.y) / 2, (led2.z + led3.z) / 2, 0.f);
					XMVECTOR up = XMVector3Normalize(XMVectorSubtract(vec3, p1_v));

					//right vector is cross product
					XMVECTOR right = XMVector3Cross(vec_out, up);
					XMVECTOR eye = XMLoadFloat3(&XMFLOAT3(0.0, 0.0f, 0.0f));

					//get the origin of guidewire - 49.5mm below led1 and 2.5mm behind
					XMVECTOR org = p1_v + (-up * 49.5) + (-vec_out * 2.5);
					
					XMStoreFloat3(&m_pos, org);

					
					//model_rot = XMMatrixRotationX(0.0f);
					//model_rot.r[2] = v_out;
					//model_rot.r[1] = -v_up;
					//model_rot.r[0] = -v_right;
					
					//model = XMMatrixLookAtRH(eye, v_out, v_up);

					model_pos = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);		//add position offset


				}
			}
			else
			{
				if (!wire_found)
				{
					wire_found = true;
					//got a 3 led visible wire marker

					//take average of longest line - will be centre point
					Point3f centre = ((*obj)[2].pnt3_1 + (*obj)[2].pnt3_2)  / 2;

					//next get out vector - cross product of centre to any 2 points (use a short interconnect so 90 degrees apart)
					Point3f p1 = (*obj)[0].pnt3_1 - centre;
					Point3f p2 = (*obj)[0].pnt3_2 - centre;

					//cross product v1 and v2 gives out vector
					//Point3f v_out = p1.cross(p2);
					XMVECTOR wire_dir;
					XMVECTOR v1 = XMLoadFloat3(&XMFLOAT3(p1.x, p1.y, p1.z));
					XMVECTOR v2 = XMLoadFloat3(&XMFLOAT3(p2.x, p2.y, p2.z));
					XMVECTOR vC = XMLoadFloat3(&XMFLOAT3(centre.x, centre.y, centre.z));

					//get wire normal
					wire_dir = XMVector3Cross(v1, v2);
					if (wire_dir.m128_f32[2] < 0)
					{
						wire_dir = -wire_dir;	//facing away from camera so invert
					}

					//construct rotation matrix
					XMVECTOR v_out = XMVector3Normalize(wire_dir);
					XMVECTOR org = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
					XMVECTOR up = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

					wire_rot = XMMatrixLookAtRH(org, v_out, -up);		//get rotation
					wire_rot *= XMMatrixTranspose(wire_rot);
					wire_pos = XMMatrixTranslationFromVector(vC);		//add position offset
				}
			}

		}
	}

	
	//debug: draw points
	{
		//for (auto i = begin(kp); i != end(kp); ++i)
		//{
			//circle(f1, i->left, 3, Scalar(255, 0, 0), 2);
			//circle(f2, i->right, 3, Scalar(255, 0, 0), 2);
		//}

		//draw text on the images - example usage of helper function
		//CVDrawText(f1, Point(50, 50), "Some text");

		//show the images to the window
		imshow(w_left, f1); waitKey(33);
		imshow(w_right, f2); waitKey(33);
	}
	

	//now check if currently sampling / collecting data for circle or sphere estimation
	if (collect_data == COLLECT_DATA_CIRCLE || collect_data == COLLECT_DATA_SPHERE)
	{
		//currently sampling a circle

		//check if this point is far enough away from the previous point: (currently only on lead led)

		float d = 0.f;
		if (led1_dataset.size() > 0)
		{
			d = sqrt(
				((led1.x - led1_dataset[led1_dataset.size()-1].x) * (led1.x - led1_dataset[led1_dataset.size()-1].x)) +
				((led1.y - led1_dataset[led1_dataset.size()-1].y) * (led1.y - led1_dataset[led1_dataset.size()-1].y)) +
				((led1.z - led1_dataset[led1_dataset.size()-1].z) * (led1.z - led1_dataset[led1_dataset.size()-1].z))
				);
		}
		
		float thresh = 2.f;

		if (d > thresh || led1_dataset.size() == 0)
		{

			led1_dataset.push_back(led1);
			led2_dataset.push_back(led2);
			led3_dataset.push_back(led3);

			//debug - draw a sphere
			geom->AddSphere(led1, 0.1f, COLOUR_YELLOW);
			geom->AddSphere(led2, 0.1f, COLOUR_YELLOW);
			geom->AddSphere(led3, 0.1f, COLOUR_YELLOW);
			
			//check not done
			if (led1_dataset.size() > datanum)
			{
				//finished collecting data, get on with circle calculation
				if (collect_data == COLLECT_DATA_CIRCLE)
				{
					cal1_finish();
				}
				else
				{
					cal2_finish();
				}
			}
		}

	}

	return true;
}


void StereoVision::SetupAverageFilter()
{
	//clear average buffer
	for (int i = 0; i < AVERAGE_NUM; i++)
	{
		led1_av[i] = { 0.f, 0.f, 0.f };
		led2_av[i] = { 0.f, 0.f, 0.f };
		led3_av[i] = { 0.f, 0.f, 0.f };
	}
}
void StereoVision::SetAverageOn()
{
	use_average = true;
}
void StereoVision::SetAverageOff()
{
	use_average = false;
}
void StereoVision::ToggleAverage()
{
	use_average = !use_average;
}

//routine to average the positions of all 3 detected leds
void StereoVision::AverageLEDs()
{
	led1_av[0] = led1;
	led2_av[0] = led2;
	led3_av[0] = led3;

	for (int i = AVERAGE_NUM - 1; i > 0; --i)
	{
		led1_av[i] = led1_av[i - 1];
		led2_av[i] = led2_av[i - 1];
		led3_av[i] = led3_av[i - 1];
		if (use_average)
		{
			led1.x += led1_av[i].x;		led1.y += led1_av[i].y;		led1.z += led1_av[i].z;
			led2.x += led2_av[i].x;		led2.y += led2_av[i].y;		led2.z += led2_av[i].z;
			led3.x += led3_av[i].x;		led3.y += led3_av[i].y;		led3.z += led3_av[i].z;
		}
	}

	if (use_average)
	{
		led1.x /= AVERAGE_NUM;		 led1.y /= AVERAGE_NUM;		led1.z /= AVERAGE_NUM;
		led2.x /= AVERAGE_NUM;		 led2.y /= AVERAGE_NUM;		led2.z /= AVERAGE_NUM;
		led3.x /= AVERAGE_NUM;		 led3.y /= AVERAGE_NUM;		led3.z /= AVERAGE_NUM;
	}

	return;
}

void StereoVision::GetPositions(XMFLOAT3& l1, XMFLOAT3& l2, XMFLOAT3& l3, XMFLOAT3& pos, XMFLOAT3& dir)
{
	l1 = led1;
	l2 = led2;
	l3 = led3;
	pos = m_pos;
	dir = m_dir;
	return;
}

bool StereoVision::capture_wire(vector<Point3f>leds, int& xout, int& yout, int& zout, XMMATRIX& rot)
{
	XMVECTOR wire_dir;
	XMVECTOR v1, v2, v3, vC;		//new vertices
	Point3f wire_center;

	//try to find the wire marker
	//here assume points are in order ie all ready paired between left and right images


	//are there 3 or 4 points found?
	if (leds.size() == 3)
	{
		//found exactly 3 points - one of leds must be hidden by wire or something.
		//get distances between 3 points.
		float d1, d2, d3;
		d1 = distance3f(leds[0], leds[1]);
		d2 = distance3f(leds[1], leds[2]);
		d3 = distance3f(leds[2], leds[0]);


		int p1, p2, p3;			//point indexes

		//Long distance is through centre.Find by average of long.
		if (d1 > max(d2, d3))
		{
			//d1 is long line
			p1 = 0; p2 = 1; p3 = 2;
			//wire_center = leds[0] + leds[1] / 2;
		}
		else if (d2 > max(d1, d3))
		{
			//d2 is long line
			p1 = 1; p2 = 2; p3 = 0;
			//wire_center = (leds[1] + leds[2]) / 2;
		}
		else
		{
			p1 = 2; p2 = 0; p3 = 1;
			//wire_center = leds[2] + leds[0] / 2;
			//d3 is long line
		}
		wire_center.x = (leds[p1].x + leds[p2].x) / 2;
		wire_center.y = (leds[p1].y + leds[p2].y) / 2;
		wire_center.z = (leds[p1].z + leds[p2].z) / 2;

		v1 = XMLoadFloat3(&XMFLOAT3(leds[p1].x, leds[p1].y, leds[p1].z));
		v2 = XMLoadFloat3(&XMFLOAT3(leds[p2].x, leds[p2].y, leds[p2].z));
		v3 = XMLoadFloat3(&XMFLOAT3(leds[p3].x, leds[p3].y, leds[p3].z));
		vC = XMLoadFloat3(&XMFLOAT3(wire_center.x, wire_center.y, wire_center.z));

		//get wire normal
		wire_dir = XMVector3Cross(XMVectorSubtract(v1, vC), XMVectorSubtract(v3, vC));
		if (wire_dir.m128_f32[2] < 0)
		{
			//facing away from camera so invert
			wire_dir = -wire_dir;
		}
	}
	else if (leds.size() == 4)
	{
		//found exactly 4 points
		//check co-planar, if not then fail (with error margin)
		//get distances between all points
		float d[6] = { 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f };
		lineseg seg[6];
		
		seg[0].dist = distance3f(leds[0], leds[1]);
		seg[1].dist = distance3f(leds[1], leds[2]);
		seg[2].dist = distance3f(leds[2], leds[3]);
		seg[3].dist = distance3f(leds[3], leds[0]);
		seg[4].dist = distance3f(leds[0], leds[2]);
		seg[5].dist = distance3f(leds[1], leds[3]);

		seg[0].p1 = 0;
		seg[1].p1 = 1;
		seg[2].p1 = 2;
		seg[3].p1 = 3;
		seg[4].p1 = 0;
		seg[5].p1 = 1;
		seg[0].p2 = 1;
		seg[1].p2 = 2;
		seg[2].p2 = 3;
		seg[3].p2 = 0;
		seg[4].p2 = 2;
		seg[5].p2 = 3;
		
		//find long 2 lines - sort by length
		bubblesort_float(d, 6);
		bubblesort_lineseg(seg, 6);

		//check long lines are perpendicular
		//Mat p1, p2;
		//p1 = leds[seg[4].p2] - leds[seg[4].p1];
		//p2 = leds[seg[5].p2] - leds[seg[5].p1];
		//float dotproduct = cvDotProduct(p1, p2);

		//use average of 2 long lines for centre
		wire_center.x = leds[seg[4].p1].x + leds[seg[4].p2].x + leds[seg[5].p1].x + leds[seg[5].p2].x;
		wire_center.x /= 4;
		wire_center.y = leds[seg[4].p1].y + leds[seg[4].p2].y + leds[seg[5].p1].y + leds[seg[5].p2].y;
		wire_center.y /= 4;
		wire_center.z = leds[seg[4].p1].z + leds[seg[4].p2].z + leds[seg[5].p1].z + leds[seg[5].p2].z;
		wire_center.z /= 4;

		//get normal using any 2 points with short segment (will be 90 degrees not opposite) and centre
		v1 = XMLoadFloat3(&XMFLOAT3(leds[seg[0].p1].x, leds[seg[0].p1].y, leds[seg[0].p1].z));
		//v2 = XMLoadFloat3(&XMFLOAT3(leds[p3].x, leds[p3].y, leds[p3].z));
		v3 = XMLoadFloat3(&XMFLOAT3(leds[seg[0].p2].x, leds[seg[0].p2].y, leds[seg[0].p2].z));
		vC = XMLoadFloat3(&XMFLOAT3(wire_center.x, wire_center.y, wire_center.z));

		//get wire normal
		wire_dir = XMVector3Cross(XMVectorSubtract(v1, vC), XMVectorSubtract(v3, vC));
		if (wire_dir.m128_f32[2] < 0)
		{
			//facing away from camera so invert
			wire_dir = -wire_dir;
		}

	}
	else
	{
		return false;	//couldn't identify a wire marker
	}

	//hopefully if got here have found the marker, have a location and direction vector

	//construct rotation matrix
	XMVECTOR v_out = XMVector3Normalize(wire_dir);
	XMVECTOR org = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR up = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

	rot = XMMatrixLookAtRH(org, v_out, -up);
	rot = XMMatrixTranspose(rot);

	xout = vC.m128_f32[0];
	yout = vC.m128_f32[1];
	zout = vC.m128_f32[2];


	return true;
}

void StereoVision::cal1_start()
{
	//enter first calibration stage - collect enough points for circle estimation
	use_average = false;
	collect_data = COLLECT_DATA_CIRCLE;		//set to circle first
	cal1_done = false;

	//clear the buffers
	led1_dataset.clear();
	led2_dataset.clear();
	led3_dataset.clear();
	datanum = 50;		//collect 100 points

	//clear any geometry all ready there
	geom->ClearAll();
}
bool StereoVision::cal1_isdone()
{
	return cal1_done;
}
void StereoVision::cal1_finish()
{
	//called when enough points collected to estimate the circle
	float radius1 = 0.f, radius2 = 0.f, radius3 = 0.f;
	XMFLOAT3 center1, center2, center3;
	XMFLOAT3 dir1, dir2, dir3;
	
	fitCircle(led1_dataset, center1, dir1, radius1);
	fitCircle(led2_dataset, center2, dir2, radius2);	
	fitCircle(led3_dataset, center3, dir3, radius3);

	//average the 3 centers and also dir for more accuracy
	XMFLOAT3 center = { (center1.x + center2.x + center3.x) / 3,
						(center1.y + center2.y + center3.y) / 3,
						(center1.z + center2.z + center3.z) / 3 };

	XMFLOAT3 dir = { (dir1.x + dir2.x + dir3.x) / 3,
					(dir1.y + dir2.y + dir3.y) / 3,
					(dir1.z + dir2.z + dir3.z) / 3 };

	//dir may be pointing away from screen so correct if necessary:
	if (dir.z > 0)
	{
		dir.x = -dir.x;
		dir.y = -dir.y;
		dir.z = -dir.z;
	}

	m_pos = center;
	m_dir = dir;
	XMVECTOR dr = XMLoadFloat3(&m_dir);
	dr = XMVector3Normalize(dr);
	XMStoreFloat3(&m_dir, dr);

	XMFLOAT3 p2 = { m_dir.x * 200 + m_pos.x,
					m_dir.y * 200 + m_pos.y,
					m_dir.z * 200 + m_pos.z };

	geom->AddSphere(m_pos, 0.1f, COLOUR_WHITE);
	geom->AddLine(m_pos, p2, COLOUR_WHITE);


	p2 = { dir1.x * 50 + center1.x,
		dir1.y * 50 + center1.y,
		dir1.z * 50 + center1.z };
	geom->AddSphere(center1, 0.1f, COLOUR_CYAN);
	geom->AddLine(center1, p2, COLOUR_CYAN);
	
	p2 = { dir2.x * 50 + center2.x,
		dir2.y * 50 + center2.y,
		dir2.z * 50 + center2.z };
	geom->AddSphere(center2, 0.1f, COLOUR_RED);
	geom->AddLine(center2, p2, COLOUR_RED);
	
	p2 = { dir3.x * 50 + center3.x,
		dir3.y * 50 + center3.y,
		dir3.z * 50 + center3.z };
	geom->AddSphere(center3, 0.1f, COLOUR_PURPLE);
	geom->AddLine(center3, p2, COLOUR_PURPLE);

	//center is now located at origin of wire, at base of the marker
	//dir is pointing towards wire tip.

	//now need to make this an offset relative to marker leds so that
	//the true center can be calculated from marker led locations
	//in other words need a transformation that corrects / calibrates
	//the measured orientation from the marker

	cal1_done = true;
	collect_data = COLLECT_DATA_NONE;		//turn off data collection
	use_average = true;		//turn average filter back on again

	//now have true centre rotation and position
	//calculate the offset from measured marker position
	//use current marker postion for this

	//get the current direction matrix from led data and invert
	XMMATRIX model_dir = XMMatrixTranspose(get_dir());

	//build a matrix base on centre of rotation found by regression
	//currently a direction vector only and position offset
	//need another perpendicular vector somehow

	//c is offset of rotation axis from led1, derotated by marker direction
	XMVECTOR c = XMVectorSubtract(XMLoadFloat3(&center), XMLoadFloat3(&led1));
	c = XMVector3Transform(c, model_dir);

	//d is direction vector, derotated by marker direction
	XMVECTOR d = XMLoadFloat3(&dir);
	d = XMVector3Transform(d, model_dir);


	//save both of above as float3's
	XMStoreFloat3(&calib_offset, c);
	XMStoreFloat3(&calib_dir, d);

	return;
}

//return matrix rotation to orientate an object with the marker
XMMATRIX StereoVision::get_dir()
{
	XMMATRIX model_dir = XMMatrixIdentity();		//initialise matrix
	//get the direction
	XMVECTOR L1 = XMLoadFloat3(&led1);
	XMVECTOR L2 = XMLoadFloat3(&led2);
	XMVECTOR L3 = XMLoadFloat3(&led3);
	XMVECTOR v1 = XMVectorSubtract(L2, L1);
	XMVECTOR v2 = XMVectorSubtract(L3, L1);
	XMVECTOR L4 = XMVectorSet(led2.x + ((led3.x - led2.x) / 2), led2.y + ((led3.y - led2.y) / 2), led2.z + ((led3.z - led2.z) / 2), 0.f);	//this is virtual top led between led2 and 3
	XMVECTOR v_up = XMVector3Normalize(XMVectorSubtract(L4, L1));
	XMVECTOR dir = XMVector3Normalize(XMVector3Cross(v1, v2));
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(v_up, dir));
	//model_dir = XMMatrixLookAtLH(-dir, XMVectorSet(0.f, 0.f, 0.f, 0.f),v_up);	//eye = dir, focus = 0
	model_dir.r[0] = right;
	model_dir.r[1] = v_up;
	model_dir.r[2] = dir;

	//so far this is all based on the leds alone, and not the measured axis of rotation.
	//if we have calibrated, then we can apply a further transformation to this matrix

	if (cal1_done)
	{
		//have calibration data so improve
	}
	else
	{
		//no calibration yet so just assume all in plane
		//rotation axis is 49.5mm below led1 and approx 1mm behind
		//assume dir is unchanged

	}


	return model_dir;
}

//return translation matrix
XMMATRIX StereoVision::get_pos()
{
	XMMATRIX T = XMMatrixIdentity();

	//for now just return a rough estimate
	//49.5mm below, 1mm behind

	XMVECTOR v1 = XMVector3Normalize(XMLoadFloat3(&m_dir)) * -1.f;
	//XMVECTOR v2 = XMVector3Normalize(XMLoadFloat3(&m_up)) * -49.5f;

	//XMVECTOR v3 = v1 + v2;
	

	return T;
}

//return a rotation matrix from a direction vector
XMMATRIX StereoVision::rot_from_dir(XMFLOAT3 dir)
{
	return XMMatrixIdentity();
}

void StereoVision::cal2_start()
{
	//enter first calibration stage - collect enough points for circle estimation
	use_average = false;
	collect_data = COLLECT_DATA_SPHERE;		//set to circle first
	cal2_done = false;

	//clear the buffers
	led1_dataset.clear();
	led2_dataset.clear();
	led3_dataset.clear();
	datanum = 600;		//collect 100 points

	//clear previous geometry
	geom->ClearAll();

}
bool StereoVision::cal2_isdone()
{
	return cal2_done;
}
void StereoVision::cal2_finish()
{
	//called when enough points collected to estimate the circle
	//this will be a very, very slow function which will block everything at the moment
	float radius1 = 0.f, radius2 = 0.f, radius3 = 0.f;
	XMFLOAT3 center1, center2, center3;

	//needs to seed with an initial guess at sphere center
	//try to use estimated wire tip location based on marker and wire length
	//or just use the centroid as below
	center1 = find_centroid(led1_dataset);
	LeastSquaresSphere(led1_dataset, radius1, center1, 500.f, 20.f);
	LeastSquaresSphere(led1_dataset, radius1, center1, 60.f, 20.f);
	LeastSquaresSphere(led1_dataset, radius1, center1, 6.f, 20.f);
	geom->AddSphere(center1, 0.05f, COLOUR_CYAN);

	center2 = center1;
	LeastSquaresSphere(led2_dataset, radius2, center2, 50.f, 20.f);
	LeastSquaresSphere(led2_dataset, radius2, center2, 6.f, 20.f);
	//LeastSquaresSphere(led2_dataset, radius2, center2, 2.f, 20.f);
	geom->AddSphere(center2, 0.05f, COLOUR_RED);

	center3 = center2;
	LeastSquaresSphere(led3_dataset, radius3, center3, 50.f, 20.f);
	LeastSquaresSphere(led3_dataset, radius3, center3, 6.f, 20.f);
	//LeastSquaresSphere(led3_dataset, radius3, center3, 2.f, 20.f);
	geom->AddSphere(center3, 0.05f, COLOUR_PURPLE);

	//average the 3 centers and also dir for more accuracy
	//XMFLOAT3 center = { (center1.x + center2.x + center3.x) / 3,
	//	(center1.y + center2.y + center3.y) / 3,
	//	(center1.z + center2.z + center3.z) / 3 };


	cal2_done = true;
	collect_data = COLLECT_DATA_NONE;	//turn off data collection
	use_average = true;		//turn average filter back on again
	return;
}
void StereoVision::Shutdown()
{
	m_cam1.release();
	m_cam2.release();
	destroyAllWindows();
}

void StereoVision::CVDrawText(Mat& mt, Point pt, char* chr)
{
	int fontFace = FONT_HERSHEY_COMPLEX_SMALL;
	double fontScale = 0.5;
	int thickness = 1;
	string str(chr);

	//show distances, coordinates etc
	//void cv::line(img, pt1, pt2, Scalar& color, int thickness, int lineType, in shift)
	//void putText(img, string&, Point org, int Font, double fontScale, color, thickness, lineType, bottomLeftOrigin)
	//origin is bottom left corner of text
	putText(mt, str, pt, fontFace, fontScale, Scalar(255, 255, 255), thickness);
}

void bubblesort_interconnect(vector<Interconnect>& arr)
{
	bool swapped = true;
	int j = 0;
	int n = arr.size();
	Interconnect tmp;
	while (swapped) {
		swapped = false;
		j++;
		for (int i = 0; i < n - j; i++) {
			if (arr[i].distance > arr[i + 1].distance) {
				tmp = arr[i];
				arr[i] = arr[i + 1];
				arr[i + 1] = tmp;
				swapped = true;
			}
		}
	}
}

void StereoVision::setGeom(GeometryClass* g)
{
	geom = g;
}