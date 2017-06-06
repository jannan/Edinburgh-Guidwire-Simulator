////////////////////////////////////////////////////////////////////////////////
// Filename: systemclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "systemclass.h"
#include <windowsx.h>

#define XRAY_MODE

LPWSTR m_userName;

SystemClass::SystemClass()
{
	m_Input = 0;
	m_Graphics = 0;
	//m_StereoVision = 0;
	StereoVision m_StereoVision;

	m_model_placed = false;
}

SystemClass::SystemClass(const SystemClass& other)
{
}

SystemClass::~SystemClass()
{
}

bool SystemClass::Initialize()
{
	//very first thing - initialise scenario reader and load the scenario
	scenario = new ScenarioFileClass;

	scenario->LoadConfigFile("scenarios\\global settings.txt");
	//load default scenario as found in "global settings.txt"
	//if (!scenario->LoadStartupScenario())  scenario->LoadScenario("scenarios\\dhs_right", "dhs_right.txt");

	scenario->LoadStartupScenario();
	//scenario->LoadScenario("scenarios\\dhs_right", "dhs_right.txt");
	//scenario->LoadScenario("scenarios\\elbow_supracondylar", "elbow_supracondylar.txt");
	//scenario->LoadScenario("scenarios\\sufe_right", "sufe_right.txt");


	m_apices[0] = XMFLOAT3(46, -123.9, -1150);
	m_apices[1] = XMFLOAT3(44.5, -122.9, -1144);
	m_apices[2] = XMFLOAT3(44.5, -122.9, -1139);
	m_apices[3] = XMFLOAT3(44.5, -122.9, -1135);
	m_apices[4] = XMFLOAT3(45.5, -122.9, -1132);
	m_apices[5] = XMFLOAT3(47.5, -122.9, -1128);
	m_apices[6] = XMFLOAT3(51.5, -122.9, -1125);
	m_apices[7] = XMFLOAT3(54.5, -122.9, -1121);
	m_apices[8] = XMFLOAT3(58.8, -122.9, -1119);
	m_apices[9] = XMFLOAT3(63.5, -122.9, -1117);
	m_apices[10] = XMFLOAT3(69.5, -122.9, -1117);
	m_apices[11] = XMFLOAT3(74.5, -122.9, -1118);
	m_apices[12] = XMFLOAT3(63.5, -139.9, -1132);
	m_apices[13] = XMFLOAT3(64.5, -95.9, -1132);
	m_apices[14] = XMFLOAT3(50.5, -105.9, -1131);
	m_apices[15] = XMFLOAT3(55.5f, -135.9f, -1131.f);


	int screenWidth, screenHeight;
	bool result;

	scenario->sc_multiwire = true; //overide

	//initialise marker detection object (contains everything for stereo camera to provide x, y, z output of marker and pose
	//may expand to track more than 1 marker
	result = m_StereoVision.Initialize(scenario->m_cam_left, scenario->m_cam_right, scenario->getChessboardSize(), scenario->m_cam_left_flip, scenario->m_cam_right_flip);
	if (!result)
	{
		//return false;
	}


	// Initialize the width and height of the screen to zero before sending the variables into the function.
	screenWidth = 0;
	screenHeight = 0;

	// Initialize the windows api.
	InitializeWindows(screenWidth, screenHeight);

	// Create the input object.  This object will be used to handle reading the keyboard input from the user.
	m_Input = new InputClass;
	if (!m_Input)
	{
		return false;
	}

	// Initialize the input object.
	m_Input->Initialize(m_hinstance, m_hwnd, screenWidth, screenHeight);

	// Create the graphics object.  This object will handle rendering all the graphics for this application.
	m_Graphics = new GraphicsClass;
	if (!m_Graphics)
	{
		return false;
	}

	// Initialize the graphics object.
	result = m_Graphics->Initialize(screenWidth, screenHeight, m_hwnd, scenario);
	if (!result)
	{
		return false;
	}
	m_Graphics->SetRenderState(GSTATE_3D);	//start in 3d view mode
	//m_Graphics->SetRenderState(GSTATE_II);

	//attach the geometry object from graphics class to stereovision so it can display things
	m_StereoVision.setGeom(m_Graphics->GetGeometry());

	system_state = STATE_START;		//set initial state - not calibrated, model not set
	SetState_Start();

#ifdef XRAY_MODE
	{
		set_xray_mode(true);
	}
#endif

}

void SystemClass::SetState_Start()
{
	system_state = STATE_START;
	m_running_assessment = false;
	m_practicing = false;
	if (m_StereoVision.ison)	MessageBox(m_hwnd, L"Register model position:\n\nPlace marker on wire and insert into distal hole on model\nThen press return", L"Register model", MB_OK);
	//m_Graphics->SetRenderState(GSTATE_3D);
}

void SystemClass::Shutdown()
{
	if (scenario)
	{
		delete scenario;
		scenario = 0;
	}

	// Release the graphics object.
	if (m_Graphics)
	{
		m_Graphics->Shutdown();
		delete m_Graphics;
		m_Graphics = 0;
	}

	// Release the input object.
	if (m_Input)
	{
		m_Input->Shutdown();
		delete m_Input;
		m_Input = 0;
	}

	// Shutdown the window.
	ShutdownWindows();

	return;
}

void SystemClass::Run()
{
	MSG msg;
	bool done;

	// Initialize the message structure.
	ZeroMemory(&msg, sizeof(MSG));

	// Loop until there is a quit message from the window or the user.
	//this is the main loop of the application
	done = false;
	while (!done)
	{
		// Handle the windows messages.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If windows signals to end the application then exit out.
		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			//get the marker position if available - run the marker detector
			XMMATRIX rot = XMMatrixRotationX(0.0f);
			XMMATRIX wire_dir = XMMatrixIdentity();
			XMMATRIX model_dir = XMMatrixIdentity();
			XMMATRIX wire_pos, model_pos;
			bool result = false;
			bool wire_visible, model_visible;	//are markers are in view

			XMMATRIX MarkerMatrix = rot;
			XMFLOAT3 l1, l2, l3, m_pos, m_dir;

			//very large function to get location and rotation of wire and model markers, and bool indicating if visible or not
			result = m_StereoVision.Capture(wire_dir, wire_pos, model_dir, model_pos, wire_visible, model_visible, 0);
			
			//this gets the actual 3d position of the 3d detected leds - RAW result
			//this may be what we should actually be using here?
			m_StereoVision.GetPositions(l1, l2, l3, m_pos, m_dir);	//get led positions
			m_Graphics->SaveMarker(l1, l2, l3, m_pos, m_dir);		//save them to the graphics class for next render

			//todo: get led positions for left and right cameras so can display camera information
			//show / hide with keypress toggle or menu item

			//show something if couldn't get a marker position for some reason
			if (!result)
			{
				//couldnt get stereovision to work
				wire_visible = false;
				model_visible = false;

				//show warning message that wire not visible
				m_Graphics->SetMarkerVisible(false);

				m_marker_visible = false;
			}
			else
			{
				m_marker_visible = true;

				m_Graphics->SetMarkerVisible(true);	//dont show marker inivisible warning
				m_Graphics->SetGuidewireVisible(true);	//show guidewire

				//get the direction from led positions
				XMVECTOR L1 = XMLoadFloat3(&l1);
				XMVECTOR L2 = XMLoadFloat3(&l2);
				XMVECTOR L3 = XMLoadFloat3(&l3);
				XMVECTOR v1 = XMVectorSubtract(L2, L1);
				XMVECTOR v2 = XMVectorSubtract(L3, L1);
				XMVECTOR L4 = XMVectorSet( l2.x + ((l3.x - l2.x) / 2), l2.y + ((l3.y - l2.y) / 2), l2.z + ((l3.z - l2.z) / 2), 0.f);	//this is virtual top led between led2 and 3
				XMVECTOR L5 = L2 + (L3 - L2);
				XMVECTOR v_up = XMVector3Normalize(XMVectorSubtract(L4, L1));
				XMVECTOR dir = XMVector3Normalize(XMVector3Cross(v1, v2));
				XMVECTOR right = XMVector3Normalize(XMVector3Cross(v_up, dir));
				model_dir.r[0] = right;
				model_dir.r[1] = v_up;
				model_dir.r[2] = dir;

				if (m_calibrating_marker)
				{
					XMVECTOR led_av = (L1 + L2 + L3) / 3;
					XMFLOAT3 _led_average;
					XMStoreFloat3(&_led_average, led_av);
					CalibrateMarker_Step2(_led_average);
				}

				//update wire position
				m_Graphics->MoveWire(m_marker, m_marker_translate);

				XMStoreFloat3(&m_marker_dir, dir);
				XMVECTOR marker_origin = XMVector3Transform(XMVectorSet(0.f, 0.f, 0.f, 1.f), model_pos);
				XMStoreFloat3(&m_marker_pos, marker_origin);
				m_marker = model_dir;
				m_marker_translate = model_pos;

			}


			//This is used to show an axis marker - not really necessary
			//m_Graphics->UpdateMarker(MarkerMatrix);

			//do the frame processing.
			result = Frame();
			if (!result)
			{
				done = true;
			}
		}

	}

	return;
}

//run marker calibration - collect points around a circle and best-fit
bool SystemClass::CalibrateMarker_Step1()
{
	/*
		How to calibrate marker:
		Step 1: Signal we are collecting data.  Show a message to tell user what to do.  Go to step 2
		Step 2: Add geometry every frame new data captured.  Check if have enough data.  Go to step 3
		Step 3: Have enough data now.  Run best fit algorithm.  Tell user we have finished.  Save result.
	
	*/

	MessageBox(m_hwnd, L"Calibrate marker:\n\n1.  Insert wire with marker into a hole\n2.  Slowly rotate wire, watch collected data\n3.  Calibration finished automatically once enough data collected", L"Calibrate Marker", MB_OK);
	m_Graphics->SetRenderState(GSTATE_3D);		//make sure we are in 3d mode
	m_calibrating_marker = true;	//set flag to say we are in marker calibration mode
	_calib_marker_count = 0;	//reset counter for number of points
	m_Graphics->GetGeometry()->ClearAll();	//clear the geometry
	m_StereoVision.SetAverageOff();
	return true;
}

//collect points, assess weather have enough, and if do then initiate calculation stage
bool SystemClass::CalibrateMarker_Step2(XMFLOAT3 av)
{
	m_Graphics->GetGeometry()->AddSphere(av, 0.05f, COLOUR_YELLOW);
	_calib_marker_centers[_calib_marker_count] = av;	//save current centre for final calculation

	_calib_marker_count++;
	if (_calib_marker_count < CAL_NUMPOINTS_CIRCLE) return false;
	//have enough points

	//final calculation step
	vector<XMFLOAT3> points;
	for (int i = 0; i < _calib_marker_count; i++){points.push_back(_calib_marker_centers[i]);}
	XMFLOAT3 c, d;
	float rad;
	m_Graphics->fitCircle(points, c, d, rad, m_Graphics->GetGeometry(), true);

	//save calculated parameters
	_calib_marker_truedir = d;
	

	m_calibrating_marker = false;
	//m_Graphics->GetGeometry()->ClearAll();
	m_StereoVision.SetAverageOn();
	return true;
}


//finish calibration step 1:
//save 1st wire position - thats all this does (returns false if marker inivisible)
bool SystemClass::register_model1()
{
	if (!m_marker_visible)return false;

	m_wire1_pos = m_marker_pos;
	m_wire1_dir = m_marker_dir;

	//show a second guidewire in first position
	m_Graphics->MoveWire2(m_marker, XMMatrixTranslationFromVector(XMLoadFloat3(&m_marker_pos)));
	m_Graphics->SetGuidewire2Visible(true);

	m_Graphics->SetModelVisible(true);
	m_Graphics->MoveModel(m_marker, m_marker_translate);

	MessageBox(m_hwnd, L"Register model position:\n\nInsert wire into proximal hole on model\nThen press return", L"Register model", MB_OK);

	return true;
}
//finish calibration step2:
//save 2nd wire position and calculate resulting model position (false if marker invisible or parallel wires)
bool SystemClass::register_model2()
{
	if (!m_marker_visible)return false;

	m_wire2_pos = m_marker_pos;
	m_wire2_dir = m_marker_dir;

	//step 1: find intersection
	XMFLOAT3 intersection = { 0.f, 0.f, 0.f };
	if(!intersect2Rays(m_wire1_dir, m_wire1_pos, m_wire2_dir, m_wire2_pos, intersection))
	{
		return false;	//couldnt get intersection - must be 2 parallel wires so fail to complete this step
	}

	//now have intersection.  Get out vector as average of 2 wire vectors
	XMVECTOR v1 = XMVector3Normalize(XMLoadFloat3(&m_wire1_dir));
	XMVECTOR v2 = XMVector3Normalize(XMLoadFloat3(&m_wire2_dir));
	XMVECTOR v_out = XMVector3Normalize((v1 + v2) / 2);	//average
	XMVECTOR v_up = XMVector3Normalize(XMVector3Cross(v2, v1));	//cross product gives up vector
	if (v_up.m128_f32[1] < 0) v_up = -v_up;	//make sure up is really up
	XMVECTOR v_right = XMVector3Normalize(XMVector3Cross(v_up, v_out));	//further cross gives final orthogonal vector


	XMMATRIX model = XMMatrixIdentity();
	model.r[0] = v_right;
	model.r[1] = v_up;
	model.r[2] = v_out;

	//offset by intersection amount + intersection model position
	XMMATRIX model_pos = XMMatrixTranslationFromVector(XMLoadFloat3(&intersection));

	//hide second guidewire - no longer required
	//m_Graphics->SetGuidewire2Visible(false);

	//set model position and make visible
	m_Graphics->MoveModel(model, model_pos);
	m_Graphics->SetModelVisible(true);
	m_model_placed = true;

	m_Graphics->SetGuidewire2Visible(false);

	MessageBox(m_hwnd, L"Register model position:\n\nCheck position matches.  If not press spacebar to retry.\nThen press return", L"Register model", MB_OK);

	return true;
}

float SystemClass::CalculateTipApexDistance()
{
	//method:
	/* need location of apex in CT coordinates
	need wire direction and location
	need wire length


	project a point, wire length, along wire direction from wire origin
	measure distance on AP and lateral projections (using current Xrays?)
	sum distance to measure TAD.
	Also report absolute distance.

	2 points to try:
	{-51.54,-122.9,-1123}
	{51.54, 122.9, -1123}
	
	to measure TAD instead of total distance:
	calculate 3d position of wire tip
	project into screen coordinates using the II view matrix - first for AP, then for lateral
	result is in pixels, not mm.
	instead rotate by view matrix, dont project
	then discard z component.  now have x/y in mm -> use this

	option to display TAD graphics ontop of II images with a key press - toggle
	option to dsiplay TAD result with a key press - toggle
	always report TAD once simulation finished (ended by user)

	*/

	return 0.f;
}


void SystemClass::BeginCalibration1()
{
	system_state = STATE_REGISTER1;
	m_StereoVision.cal1_start();
}

void SystemClass::BeginCalibration2()
{
	if (m_StereoVision.cal1_isdone())
	{
		system_state = STATE_REGISTER2;
		m_StereoVision.cal2_start();
	}
}

char SystemClass::GetState()
{
	return system_state;
}

void SystemClass::StartAssessment()
{
	//get name and current time
	time_t currtime = time(nullptr);
	struct tm * timeinfo = 0;
	timeinfo = localtime(&currtime);

	//format into a date and time to put in file / dir name
	char timestamp[20];
	wstringstream str;
	str << L"output\\";
	if (strftime(timestamp, 20, "%y-%m-%d %H-%M", timeinfo) > 0)
	{
		str << timestamp;
	}
	else
	{
		str << L"unknown_time";
	}
	//check if there is all ready a directory with this, and if so, add an incremential number until unique

	wstring dir_temp = str.str();
	LPCWSTR dir = dir_temp.c_str();
	DWORD attrib = ::GetFileAttributes(dir);
	int i = 0;

	/*while (!(attrib == INVALID_FILE_ATTRIBUTES))
	{
		//directory already exists so find another

		//remake dirpath with number
		i++;
		attrib = ::GetFileAttributes(dir);
	}
	*/

	//make the output dir
	if (!CreateDirectory(dir, NULL))
	{
		MessageBox(m_hwnd, L"Couldnt create output directory to save results", L"Output Error", MB_OK);
		return;	//couldnt create directory - will fail if all ready exists
	}


	//save the output path
	wcscpy_s(m_outputpath, dir);

	//load apex position
	m_Graphics->setApex(XMFLOAT3(51.5f, -122.9f, -1123.f));

	//start the simulation
	m_Graphics->StartTest();

	//update menu items
	BOOL result = EnableMenuItem(GetMenu(m_hwnd), ID_SIMULATION_START, MF_GRAYED);
	result = EnableMenuItem(GetMenu(m_hwnd), ID_SIMULATION_FINISH, MF_ENABLED);
	result = EnableMenuItem(GetMenu(m_hwnd), ID_SIMULATION_RESET, MF_GRAYED);		//leave grayed until actually have a handler for it!
	result = EnableMenuItem(GetMenu(m_hwnd), ID_SIMULATION_PRACTICE, MF_GRAYED);

	m_practicing = false;
	m_running_assessment = true;
}

void SystemClass::EndAssessment()
{
	//save the results - make a report
		
	//show message to say saved

	//disable the start test menu item
	BOOL result = EnableMenuItem(GetMenu(m_hwnd), ID_SIMULATION_START, MF_ENABLED);
	result = EnableMenuItem(GetMenu(m_hwnd), ID_SIMULATION_FINISH, MF_GRAYED);
	result = EnableMenuItem(GetMenu(m_hwnd), ID_SIMULATION_RESET, MF_GRAYED);
	result = EnableMenuItem(GetMenu(m_hwnd), ID_SIMULATION_PRACTICE, MF_ENABLED);

	//go back to 3d view or enter practice mode
	//reset clock
	//reset II images

	m_running_assessment = false;

	StartPractice();	//run straight from assessment to practice mode
}

void SystemClass::assessment_saveII()
{
	//open the log file, create if not there
	wstring fname = m_outputpath; 
	fname += L"\\results.txt";

	wstringstream ws;
	ws << m_outputpath;
	ws << L"\\results.txt";
	wstring nm = ws.str();
	ofstream f;
	f.open(nm.c_str(), ios::app);

	f << "Time: " << m_Graphics->getTime().c_str() << ", XR num:" << m_Graphics->getXRNum() << ", TAD:" << m_Graphics->getTAD() << ", True:" << m_Graphics->getTrue() << "\n";

	f.close();
}

void SystemClass::StartPractice()
{
	MessageBox(m_hwnd, L"Entering practice mode.\nKeep aiming for targets with wire as directed.\nPress return once finished a wire.", L"Practice", MB_OK);

	//make sure assessment not currently running
	//if so -> signal end assessment menu item pressed

	//load apex position
	m_Graphics->setApex(XMFLOAT3(51.5f, -122.9f, -1123.f));

	m_Graphics->PracticeMode();	//set graphics to practice mode

	BOOL result = EnableMenuItem(GetMenu(m_hwnd), ID_SIMULATION_PRACTICE, MF_GRAYED);
	m_practicing = true;

	PracticeNextWire();

}

void SystemClass::PracticeNextWire()
{
	m_Graphics->setApex(m_apices[m_practice_order[m_practice_wirenum]]);
	m_practice_wirenum++;
	if (m_practice_wirenum > 15) m_practice_wirenum = 0;
	m_Graphics->PracticeMode();	//reset stats and reset views
}

//enter or leave the xray mode
void SystemClass::set_xray_mode(bool mode)
{
	xray_mode = mode;
	if (xray_mode)
	{
		//entered xray mode
		m_Graphics->toggleShowModel();
		m_Graphics->SetMarkerWarningVisible(false);		//turn off warning about marker not being visible
		m_Graphics->SetActiveRectangleVisible(false);	//turn off rectangle around active image
		m_Graphics->SetRenderState(GSTATE_II);

		//load apex position
		m_Graphics->setApex(XMFLOAT3(52.5f, -122.9f, -1123.f));	//todo: set from scenario file
		m_Graphics->setShowApex(false);
		m_Graphics->setShowTip(false);
		m_Graphics->setShowTAD(false);
		m_Graphics->m_showApexCoordinates = false;
		m_Graphics->m_showWireNum = true;
		m_Graphics->setShowScrew(false);
		m_Graphics->m_showTAD_text = false;
		//m_Graphics->m_showPolarView = false;
		m_Graphics->m_show_DHS_screw = false;

		XMFLOAT2 offset;
		float ap_rotate = 0.f;
		float lat_rotate = -0.f; //+ve -> aim anterior

		int wnum = 0;

		for (int i = 0; i < 25; i++)
		{

			switch (i) {

				case 0:				offset = { 0.f, 0.f };			ap_rotate = 0.f;		lat_rotate = 0.0f;		break;
				case 1:				offset = { -5.f, 0.f };			ap_rotate = 0.f;		lat_rotate = 0.0f;		break;
				case 2:				offset = { 5.f, 0.f };			ap_rotate = 0.f;		lat_rotate = 0.0f;		break;
				case 3:				offset = { 5.f, -5.f };			ap_rotate = 0.f;		lat_rotate = -6.0f;		break;
				case 4:				offset = { 5.f, 5.f };			ap_rotate = 0.f;		lat_rotate = 6.0f;		break;
				case 5:				offset = { 0.f, -5.f };			ap_rotate = 0.f;		lat_rotate = -5.0f;		break;
				case 6:				offset = { -5.f, -5.f };		ap_rotate = 0.f;		lat_rotate = -5.0f;		break;
				case 7:				offset = { -5.f, 5.f };			ap_rotate = 0.f;		lat_rotate =  5.0f;		break;
				case 8:				offset = { 0.f, 5.f };			ap_rotate = 0.f;		lat_rotate = 5.0f;		break;


				case 9:				offset = { -10.f, -5.f };		ap_rotate = 0.f;		lat_rotate = -5.0f;		break;
				case 10:				offset = { -10.f, 0.f };		ap_rotate = 0.f;		lat_rotate = 0.0f;		break;
				case 11:				offset = { -10.f, 5.f };		ap_rotate = 0.f;		lat_rotate = 5.0f;		break;
				case 12:			offset = { 10.f, -5.f };		ap_rotate = 0.f;		lat_rotate = -5.0f;		break;
				case 13:			offset = { 10.f, 0.f };			ap_rotate = 0.f;		lat_rotate = 0.0f;		break;
				case 14:			offset = { 10.f, 5.f };			ap_rotate = 0.f;		lat_rotate = 5.0f;		break;
				case 15:			offset = { -5.f, -10.f };		ap_rotate = 0.f;		lat_rotate = -10.0f;	break;
				case 16:			offset = { 0.f, -10.f };		ap_rotate = 0.f;		lat_rotate = -10.0f;	break;
				case 17:			offset = { 5.f, -10.f };		ap_rotate = 0.f;		lat_rotate = -10.0f;	break;
				case 18:			offset = { -5.f, 10.f };		ap_rotate = 0.f;		lat_rotate = 10.0f;		break;
				case 19:			offset = { 0.f, 10.f };			ap_rotate = 0.f;		lat_rotate = 10.0f;		break;
				case 20:			offset = { 5.f, 10.f };			ap_rotate = 0.f;		lat_rotate = 10.0f;		break;

				case 21:			offset = { -10.f, -10.f };		ap_rotate = 0.f;		lat_rotate = -10.0f;	break;
				case 22:			offset = { -10.f, 10.f };		ap_rotate = 0.f;		lat_rotate = 10.0f;		break;
				case 23:			offset = { 10.f, -10.f };		ap_rotate = 0.f;		lat_rotate = -10.0f;	break;
				case 24:			offset = { 10.f, 10.f };		ap_rotate = 0.f;		lat_rotate = 10.0f;		break;

			}

			//	1. apex offset.  2- project onto sphere using neck vector.  3-set an angle here.  4-find other end of wire.
			XMFLOAT3 wire_offset = { 0.f, 0.f, 0.f };
			XMVECTOR apex = XMLoadFloat3(&m_Graphics->m_apex);
			XMStoreFloat3(&wire_offset, apex + XMLoadFloat3(&wire_offset));	//add offset to apex
			bool result = m_Graphics->getApexOffset(offset, wire_offset);
			XMFLOAT3 wire_tip;
			bool intersected = m_Graphics->getheadsphereintersect(wire_offset, wire_tip);
			//wire_tip = wire_offset;	//debug -bypass sphere intersect5

			//wire_tip should now have a valid location on subchondral bone.  Do angles as before.


			XMMATRIX w;
			w = XMMatrixTranslation(0.f, 0.f, -145.f);			//translate so tip is at apex
			w *= XMMatrixRotationY((-35.f + ap_rotate) * (3.14159f / 180.f));	//rotate in AP
			w *= XMMatrixRotationX((13.f + lat_rotate) * (3.14159f / 180.f));	//rotate in lateral
			w *= XMMatrixTranslationFromVector(XMLoadFloat3(&wire_tip));	//move to newly calculated wire tip.

			//w *= XMMatrixTranslation(51.5f, -124.1f, -1123.f);	//move to apex
			//w *= XMMatrixTranslation(lateral_offset, posterior_offset, proximal_offset);				//final offset from apex (x -> move lateral, y-> move inferior, z-> move proximal)

			m_Graphics->SetUseCTCoords(true);	//use the ct coordinates instead of real world coordinates which we dont know
			m_Graphics->setWireMatrix(w);		//load position of wire
			m_Graphics->calculateTAD();			//get the TAD
			//bool f = m_Graphics->Frame();		//this is required - todo:check why
			m_Graphics->RenderII_AP();
			m_Graphics->RenderII_lat();

			//save current images
			wstring f_ap = L"./wires/II_" + to_wstring(i) + L"_" + to_wstring((int)offset.x) + L"," + to_wstring((int)offset.y) + L"_AP";
			wstring f_lat = L"./wires/II_" + to_wstring(i) + L"_" + to_wstring((int)offset.x) + L"," + to_wstring((int)offset.y) + L"_lat";
			HRESULT res = m_Graphics->SaveScreenshot(f_ap.c_str(), 0);
			res = m_Graphics->SaveScreenshot(f_lat.c_str(), 1);

			//save parameters
			//open the log file, create if not there
			wstring fname = L"./wires/data.txt";
			ofstream f;
			f.open(fname.c_str(), ios::app);
			f << "XR: " << i << "," << ", TAD:" << m_Graphics->getTAD() << ", X:" << offset.x << ",Y:" << offset.y << "\n";
			f.close();
		}
	}
	else
	{
		//leaving xray mode
	}
}

bool SystemClass::Frame()
{
	bool result;


	//update direct input
	result = m_Input->Frame();
	if (!result)
		return false;

	// Check if the user pressed escape and wants to exit the application.
	if (m_Input->IsKeyDown(VK_ESCAPE) || quit_request)
	{
		if (!m_running_assessment)
		{
			return false;
		}
		else
		{
			if (MessageBox(m_hwnd, L"Are you sure you want to quit the assessment?", L"Quit", MB_OKCANCEL) == IDOK)
			{
				//pressed ok
				return false;
			}
			m_Input->KeyUp(VK_ESCAPE);
		}
	}


	if (m_Input->IsKeyDown(VK_F5))
	{
		m_Input->KeyUp(VK_F5);
		//m_Graphics->RenderII_AP();
		AP_current = true;
		m_Graphics->II_setAP(true);
	}
	if (m_Input->IsKeyDown(VK_F6))
	{
		m_Input->KeyUp(VK_F6);
		//m_Graphics->RenderII_lat();
		AP_current = false;
		m_Graphics->II_setAP(false);
	}


	if (m_Input->IsKeyDown(VK_SPACE))
	{
		//space key
		//m_model_placed = true;

		//used to increment states if appropriate
		if ((system_state == STATE_START))		//also able to re-enter incase loose femur position
		{
			BeginCalibration1();
		}
		else if (system_state == STATE_REGISTER1 && m_StereoVision.cal1_isdone())
		{
			BeginCalibration2();
		}
		else if (system_state == STATE_RUN)
		{
			system_state = STATE_START;		//restart femur measurement
			m_Graphics->SetRenderState(GSTATE_3D);
			m_Graphics->SetModelVisible(false);
			m_model_placed = false;
		}
	}

	if (m_Input->IsKeyDown(VK_F7))
	{
		(AP_current) ? m_Graphics->SetIIAngleAP(m_Graphics->GetIIAngleAP(0.f) - 5) : m_Graphics->SetIIAngleLat(m_Graphics->GetIIAngleLat(0.f) - 5);
		m_Input->KeyUp(VK_F7);
	}
	if (m_Input->IsKeyDown(VK_F8))
	{
		(AP_current) ? m_Graphics->SetIIAngleAP(m_Graphics->GetIIAngleAP(0.f) + 5) : m_Graphics->SetIIAngleLat(m_Graphics->GetIIAngleLat(0.f) + 5);
		m_Input->KeyUp(VK_F8);
	}

	if (m_Input->IsKeyDown(VK_UP))
	{
		//m_Graphics->MoveOffset(0.0, -1.0, 0.0);
		m_apex.y -= 1.f;
		m_Graphics->setApex(m_apex);
		m_Graphics->RenderII_AP();
		m_Graphics->RenderII_lat();

		m_Input->KeyUp(VK_UP);	//reset press event
	}
	if (m_Input->IsKeyDown(VK_DOWN))
	{
		//m_Graphics->MoveOffset(0.0, 1.0, 0.0);
		m_apex.y += 1.f;
		m_Graphics->setApex(m_apex);
		m_Graphics->RenderII_AP();
		m_Graphics->RenderII_lat();
		m_Input->KeyUp(VK_DOWN);	//reset press event
	}
	if (m_Input->IsKeyDown(VK_LEFT))
	{
		//m_Graphics->MoveOffset(-1.0, 0.0, 0.0);
		m_apex.x -= 1.f;
		m_Graphics->setApex(m_apex);
		m_Graphics->RenderII_AP();
		m_Graphics->RenderII_lat();
		m_Input->KeyUp(VK_LEFT);	//reset press event
	}
	if (m_Input->IsKeyDown(VK_RIGHT))
	{
		//m_Graphics->MoveOffset(1.0, 0.0, 0.0);
		m_apex.x += 1.f;
		m_Graphics->setApex(m_apex);
		m_Graphics->RenderII_AP();
		m_Graphics->RenderII_lat();
		m_Input->KeyUp(VK_RIGHT);	//reset press event
	}
	if (m_Input->IsKeyDown(VK_NUMPAD8))
	{
		//m_Graphics->MoveOffset(0.0, 0.0, -1.0);
		m_apex.z -= 1.f;
		m_Graphics->setApex(m_apex);
		m_Graphics->RenderII_AP();
		m_Graphics->RenderII_lat();
		m_Input->KeyUp(VK_NUMPAD8);	//reset press event
	}
	if (m_Input->IsKeyDown(VK_NUMPAD2))
	{
		//m_Graphics->MoveOffset(0.0, 0.0, 1.0);
		m_apex.z += 1.f;
		m_Graphics->setApex(m_apex);
		m_Graphics->RenderII_AP();
		m_Graphics->RenderII_lat();
		m_Input->KeyUp(VK_NUMPAD2);	//reset press event
	}
	if (m_Input->IsKeyDown(VK_F1))
	{
		//save - todo: change to display help
		m_Graphics->SaveOffset();
	}
	if (m_Input->IsKeyDown('F'))
	{
		m_Input->KeyUp('F');
		m_StereoVision.ToggleAverage();
	}
	if (m_Input->IsKeyDown(VK_RETURN))
	{
		m_Input->KeyUp(VK_RETURN);
		if (system_state == STATE_RUN)
		{
			if (m_practicing)
			{
				PracticeNextWire();
			}
			else if (!m_running_assessment)
			{
				m_Graphics->SetIIView();	//hide the second gudiewire, show II screens
				StartPractice();
			}
		}
		else if (system_state == STATE_START)	//femur not dropped so start registration
		{
			//call do cal1 to finish the registration 1st part
			if (register_model1()) system_state = STATE_REGISTER1;
		}
		else if (system_state == STATE_REGISTER1)
		{
			if (register_model2()) system_state = STATE_RUN;
		}
		else if (system_state == STATE_REGISTER2)
		{

		}
		
	}
	if (m_Input->IsKeyDown(VK_F2))
	{
		m_Input->KeyUp(VK_F2);
		if (system_state == STATE_RUN)
		{
			m_Graphics->SetRenderState(GSTATE_II);
		}
	}
	if (m_Input->IsKeyDown(VK_F3))
	{
		m_Input->KeyUp(VK_F3);
		if (system_state == STATE_RUN)
		{
			m_Graphics->SetRenderState(GSTATE_3D);
		}
	}
	if (m_Input->IsKeyDown(VK_F4))
	{
		m_Input->KeyUp(VK_F4);
		if (system_state == STATE_RUN)
		{
			m_Graphics->SetRenderState(GSTATE_3D);
		}
	}

	if (m_Input->IsKeyDown(VK_ADD))
	{
		m_Input->KeyUp(VK_ADD);
		m_Graphics->exposure_increase();
	}
	if (m_Input->IsKeyDown(VK_SUBTRACT))
	{
		m_Input->KeyUp(VK_SUBTRACT);
		m_Graphics->exposure_decrease();
	}

	if (m_Input->IsKeyDown('1'))
	{
		m_Input->KeyUp('1');
		m_current_wire = 0;
		m_Graphics->m_currentwire = 0;
		//m_Graphics->RenderII_AP(); m_Graphics->RenderII_lat(); m_Graphics->Frame();
	}

	if (m_Input->IsKeyDown('2'))
	{
		m_Input->KeyUp('2');
		if (scenario->sc_multiwire)
		{
			m_current_wire = 1;
			m_Graphics->m_currentwire = 1;
			//m_Graphics->RenderII_AP(); m_Graphics->RenderII_lat(); m_Graphics->Frame();
		}
	}

	if (m_Input->IsKeyDown('3'))
	{
		m_Input->KeyUp('3');
		if (scenario->sc_multiwire)
		{
			m_current_wire = 2;
			m_Graphics->m_currentwire = 2;
			//m_Graphics->RenderII_AP(); m_Graphics->RenderII_lat(); m_Graphics->Frame();
		}
	}



	if (m_Input->IsKeyDown('B'))
	{
		if (m_marker_visible)
		{
			m_Input->KeyUp('B');
			m_Graphics->II_setwire();	//update wire position only now on xrays
			if (system_state == STATE_RUN) m_Graphics->RenderII_current();	//render and increment counter
			//work out new TAD based on current wire position and ideal xray angles
			//save the current xray
			m_Graphics->calculateTAD();		//get an up to date TAD
			
			//if currently in the assessment then save the II image
			if (m_running_assessment)
			{

				wstring str = m_outputpath;
				str += L"\\II_";

				m_Graphics->SaveII(str.c_str());	//leave ending up to the saveII function - will append number and type extension

				//save a line in the assessment log file
				assessment_saveII();

			}
		}
	}
	
	// Do the frame processing for the graphics object.
	result = m_Graphics->Frame();
	if (!result)
	{
		return false;
	}

	return true;
}


LRESULT CALLBACK SystemClass::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch (umsg)
	{
		// Check if a key has been pressed on the keyboard.
	case WM_KEYDOWN:
	{
		// If a key is pressed send it to the input object so it can record that state.
		m_Input->KeyDown((unsigned int)wparam);
		return 0;
	}

	// Check if a key has been released on the keyboard.
	case WM_KEYUP:
	{
		// If a key is released then send it to the input object so it can unset the state for that key.
		m_Input->KeyUp((unsigned int)wparam);
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		//mouse moved
		//is mouse over an II?
		int xPos = GET_X_LPARAM(lparam);
		int yPos = GET_Y_LPARAM(lparam);
		m_Graphics->handleMouse(xPos, yPos, wparam, m_hwnd);
		bool b = true;
	}

	// Any other messages send to the default message handler as our application won't make use of them.
	default:
	{
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
	}
}


void SystemClass::InitializeWindows(int& screenWidth, int& screenHeight)
{
	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;
	int posX, posY;


	// Get an external pointer to this object.	
	ApplicationHandle = this;

	// Get the instance of this application.
	m_hinstance = GetModuleHandle(NULL);

	// Give the application a name.
	m_applicationName = L"NavSim Ortho";

	// Setup the windows class with default settings.
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hinstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	// wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hCursor = LoadCursor(NULL, IDC_CROSS);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wc.lpszClassName = m_applicationName;
	wc.cbSize = sizeof(WNDCLASSEX);

	// Register the window class.
	RegisterClassEx(&wc);

	// Determine the resolution of the clients desktop screen.
	screenHeight = GetSystemMetrics(SM_CYSCREEN);
	screenWidth = GetSystemMetrics(SM_CXSCREEN);

	// Setup the screen settings depending on whether it is running in full screen or in windowed mode.
	if (FULL_SCREEN)
	{
		// If full screen set the screen to maximum size of the users desktop and 32bit.
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsHeight = (unsigned long)screenHeight;
		dmScreenSettings.dmPelsWidth = (unsigned long)screenWidth;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Change the display settings to full screen.
		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		// Set the position of the window to the top left corner.
		posX = posY = 0;
	}
	else
	{
		// leave width and height as screen resolution
		//screenWidth = 1920;
		//screenHeight = 1080;

		// Place the window in the middle of the screen.
		posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;
	}

	// Create the window with the screen settings and get the handle to it.
	m_hwnd = CreateWindowEx(WS_EX_APPWINDOW, m_applicationName, m_applicationName,
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
		posX, posY, screenWidth, screenHeight, NULL, NULL, m_hinstance, NULL);

	// Bring the window up on the screen and set it as main focus.
	ShowWindow(m_hwnd, SW_SHOW);
	SetForegroundWindow(m_hwnd);
	SetFocus(m_hwnd);

	// Hide the mouse cursor.
	//ShowCursor(false);

	return;
}


void SystemClass::ShutdownWindows()
{
	// Show the mouse cursor.
	ShowCursor(true);

	// Fix the display settings if leaving full screen mode.
	if (FULL_SCREEN)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	// Remove the window.
	DestroyWindow(m_hwnd);
	m_hwnd = NULL;

	// Remove the application instance.
	UnregisterClass(m_applicationName, m_hinstance);
	m_hinstance = NULL;

	// Release the pointer to this class.
	ApplicationHandle = NULL;

	return;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	switch (umessage)
	{
		// Check if the window is being destroyed.
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}

	// Check if the window is being closed.
	case WM_CLOSE:
	{
		PostQuitMessage(0);
		return 0;
	}
	case WM_COMMAND:
	{
		int i = LOWORD(wparam);

		if (LOWORD(wparam) == ID_FILE_QUIT)
		{
			ApplicationHandle->quit();
			return 0;
		}
		else
		{
			//message is a menu bar command so handle this
			ApplicationHandle->handleMenu(wparam);
		}
	}

	// All other messages pass to the message handler in the system class.
	default:
	{
		return ApplicationHandle->MessageHandler(hwnd, umessage, wparam, lparam);
	}
	}
}

void SystemClass::quit()
{
	quit_request = true;
}

void SystemClass::handleMenu(WPARAM wParam)
{
	switch (LOWORD(wParam))
	{
		case ID_HELP_ABOUT:
		DialogBox(m_hinstance, MAKEINTRESOURCE(IDD_ABOUT), m_hwnd, (DLGPROC)dlgProcAbout);
			break;

		case ID_SIMULATION_START:
			//start the assessment

			if (!(system_state == STATE_RUN))
			{
				MessageBox(m_hwnd, L"Register model position first", L"Register model", MB_OK);
				break;		//not yet registered femur
			}
			else
			{

				//first display the start assessment dialog to gather information
				//if (DialogBox(m_hinstance, MAKEINTRESOURCE(IDD_GETDETAILS), m_hwnd, (DLGPROC)dlgProcGetDetails) == IDOK)
				//{
					//I think OK was pressed.  How to get the info??
					LPWSTR nm = m_userName;

					//get the other fields as well

					//start the simulation assessment
					StartAssessment();

				//}
				//else
				//{
					//cancelled so no details.  Dont start the simulation
				//}
			}
			break;

		case ID_SIMULATION_FINISH:
			//finsh the assessment and save
			if (MessageBox(m_hwnd, L"Are you sure you want to finish the assessment and save results?", L"Finish assessment", MB_YESNO) == IDYES)
			{
				//pressed yes so quit assessment and save
				EndAssessment();
			}//else ignore
			break;

		case ID_SIMULATION_RESET:
			//reset simulation
			//display confirmation box
			break;

		case ID_SIMULATION_PRACTICEMODE:
			//enter practice mode if not in assessment
			if (system_state == STATE_RUN)
			{
				StartPractice();	//only if in system_run mode.  Check not running an assessment
			}
			else
			{
				if (system_state < STATE_RUN)
				{
					//not yet registered
					MessageBox(m_hwnd, L"Register model position first", L"Register model", MB_OK);
				}
			}
			break;

		case ID_VIEW_TOGGLETAD:
			m_Graphics->setShowApex(true);
			m_Graphics->setShowTip(true);
			break;

		case ID_VIEW_SHOWAPEX:
			m_Graphics->m_showApexCross = !m_Graphics->m_showApexCross;
			m_Graphics->m_showApexCross ?
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_SHOWAPEX, MF_CHECKED) :
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_SHOWAPEX, MF_UNCHECKED);
			m_Graphics->RenderII_AP();
			m_Graphics->RenderII_lat();
			//m_Graphics->Frame();
			break;

		case ID_VIEW_SHOWAPEXCOORDINATES:
			m_Graphics->m_showApexCoordinates = !m_Graphics->m_showApexCoordinates;
			m_Graphics->m_showApexCoordinates ?
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_SHOWAPEXCOORDINATES, MF_CHECKED) :
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_SHOWAPEXCOORDINATES, MF_UNCHECKED);
			m_Graphics->RenderII_AP();
			m_Graphics->RenderII_lat();
			//m_Graphics->Frame();
			break;


		case ID_VIEW_SHOWTAD:
			m_Graphics->m_showTAD_text = !m_Graphics->m_showTAD_text;
			m_Graphics->m_showTAD_text ?
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_SHOWTAD, MF_CHECKED) :
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_SHOWTAD, MF_UNCHECKED);
			m_Graphics->RenderII_AP();
			m_Graphics->RenderII_lat();
			break;

		case ID_VIEW_SHOWMODELOVERII:
			m_Graphics->toggleShowModel();
			break;

		case ID_VIEW_ENTERXRAYMODE:
			set_xray_mode(!xray_mode);
			break;

		case ID_SETTINGS_CALIBRATEMARKER:
			CalibrateMarker_Step1();
			break;

		case ID_VIEW_TOGGLESHOWWIRE:
			m_Graphics->m_show_wire = !m_Graphics->m_show_wire;
			m_Graphics->m_show_wire ?
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_TOGGLESHOWWIRE, MF_CHECKED) :
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_TOGGLESHOWWIRE, MF_UNCHECKED);
			m_Graphics->RenderII_AP();
			m_Graphics->RenderII_lat();
			break;

		case ID_VIEW_SHOWTIP:
			m_Graphics->m_showTip = !m_Graphics->m_showTip;
			m_Graphics->m_showTip ?
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_SHOWTIP, MF_CHECKED) :
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_SHOWTIP, MF_UNCHECKED);
			m_Graphics->RenderII_AP();
			m_Graphics->RenderII_lat();
			break;

		case ID_VIEW_TOGGLESHOWDHSSCREW:
			m_Graphics->m_show_DHS_screw = !m_Graphics->m_show_DHS_screw;
			m_Graphics->m_show_DHS_screw ? 
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_TOGGLESHOWDHSSCREW, MF_CHECKED) :
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_TOGGLESHOWDHSSCREW, MF_UNCHECKED);
			m_Graphics->RenderII_AP();
			m_Graphics->RenderII_lat();
			//m_Graphics->Frame();
			break;

		case ID_VIEW_TOGGLESHOWCANNULATEDSCREW:
			m_Graphics->m_show_cannulated_screw = !m_Graphics->m_show_cannulated_screw;
			m_Graphics->m_show_cannulated_screw ?
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_TOGGLESHOWCANNULATEDSCREW, MF_CHECKED) :
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_TOGGLESHOWCANNULATEDSCREW, MF_UNCHECKED);
			m_Graphics->RenderII_AP();
			m_Graphics->RenderII_lat();
			//m_Graphics->Frame();
			break;

		case ID_VIEW_TOGGLESHOWPOLARPLOT:
			m_Graphics->m_showPolarView = !m_Graphics->m_showPolarView;
			m_Graphics->m_showPolarView ?
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_TOGGLESHOWPOLARPLOT, MF_CHECKED) :
				CheckMenuItem(GetMenu(m_hwnd), ID_VIEW_TOGGLESHOWPOLARPLOT, MF_UNCHECKED);
			m_Graphics->Frame();
			break;

	}
}


BOOL CALLBACK dlgProcAbout(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:

		case IDCANCEL:
			EndDialog(hwndDlg, wParam);
			return true;
		}
	}
	return false;
}

BOOL CALLBACK dlgProcGetDetails(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	const char* combo1_strings[] = { "Student", "Foundation Year Doctor", "ST/CT1 or CDF", "ST2-3", "ST4-8", "Post CCT" };
	HWND combo1;

	switch (message)
	{
	/*case WM_CREATE:
		combo1 = GetDlgItem(hwndDlg, IDC_COMBO1);
		for (int i = 0; i < 5; i++)
		{
		//	SendMessage(combo1, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>((LPCTSTR)combo1_strings[i]));
		}
		SendMessage(combo1, CB_ADDSTRING, 0, (LPARAM)"Student");
		SendMessage(combo1, CB_ADDSTRING, 0, (LPARAM)"Foundation");
		SendMessage(combo1, CB_ADDSTRING, 0, (LPARAM)"ST1");
		SendMessage(combo1, CB_SETCURSEL, 0, 0);

		return true;
		*/
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (!GetDlgItemText(hwndDlg, IDC_EDIT1, m_userName, 20))
			{
				m_userName = L"unknown";
			}
		case IDCANCEL:
			EndDialog(hwndDlg, wParam);
			return true;
		}

	}
	
	return false;
}