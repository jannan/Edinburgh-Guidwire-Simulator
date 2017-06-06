////////////////////////////////////////////////////////////////////////////////
// Filename: systemclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _SYSTEMCLASS_H_
#define _SYSTEMCLASS_H_

///////////////////////////////
// PRE-PROCESSING DIRECTIVES //
///////////////////////////////
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

//////////////
// INCLUDES //
//////////////
#include <windows.h>
#include <WinUser.h>
#include <time.h>

#include "resource.h"

///////////////////////
// MY CLASS INCLUDES //
///////////////////////
#include "inputclass.h"
#include "graphicsclass.h"
#include "stereovision.h"
#include "GeometryClass.h"
#include "ScenarioFileClass.h"


#define WIRE_LENGTH 115	//length of guidewire from marker - should match wire model from origin
//TODO: put this in a configuration file.

////////////////////////////////////////////////////////////////////////////////
// Class name: SystemClass
////////////////////////////////////////////////////////////////////////////////
class SystemClass
{
public:
	SystemClass();
	SystemClass(const SystemClass&);
	~SystemClass();

	bool Initialize();
	void SetState_Start();
	void Shutdown();
	void Run();

	bool CalibrateMarker_Step1();
	bool CalibrateMarker_Step2(XMFLOAT3 av);

	float CalculateTipApexDistance();

	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);
	void quit();
	void handleMenu(WPARAM wParam);

private:

	XMFLOAT3 m_apices[16];
	
	//todo - make this random
	int m_practice_order[16] = { 5, 7, 2, 10, 0, 6, 1, 4, 9, 3, 8, 14, 11, 12, 13, 15 };

	bool Frame();
	void InitializeWindows(int&, int&);
	void ShutdownWindows();

	void BeginCalibration1();
	void BeginCalibration2();

	char GetState();

	bool register_model1();
	bool register_model2();

	void StartAssessment();
	void EndAssessment();
	void assessment_saveII();

	void StartPractice();
	void PracticeNextWire();

	void set_xray_mode(bool mode);

private:
	int m_current_wire = 0;

	LPCWSTR m_applicationName;
	HINSTANCE m_hinstance;
	HWND m_hwnd;


	ScenarioFileClass* scenario;

	InputClass* m_Input;
	GraphicsClass* m_Graphics;
	StereoVision m_StereoVision;

	bool m_model_placed = false;

	int system_state = 0;
#define STATE_START 0		//after initialisation
#define STATE_REGISTER1 1		//first calibration step - collect points for circle fit
#define STATE_REGISTER2 2		//second step - collect points for sphere fit
#define STATE_RUN	100		//running mode - all calibrated - model location is set now, so measuring the wire
#define STATE_XRAY	200		//may use - collect data to average then produce an xray with or without time delay then back to STATE_RUN

#define CAL_NUMPOINTS_CIRCLE 500	//number of points to collect for the circle / sphere fitting
#define CAL_NUMPOINTS_SPHERE 200

	bool m_calibrating_marker = false;	//keep track of weather we are in marker calibration mode or not
	int _calib_marker_count = 0;	//keep track of how many data samples we have
	XMFLOAT3 _calib_marker_centers[CAL_NUMPOINTS_CIRCLE];
	XMFLOAT3 _calib_marker_truedir;	//final true calculated direction
	
	bool xray_mode = false;	//weather in xray mode - mode to show xray with or without a wire, no marker
//	bool xray_mode = true;	//weather in xray mode - mode to show xray with or without a wire, no marker


	bool AP_current = true;
	
	bool m_marker_visible = false;	//keep track of weather we can actually see the marker

	//some calibration parameters to save:
	XMFLOAT3 m_wire1_pos;
	XMFLOAT3 m_wire1_dir;
	XMFLOAT3 m_wire2_pos;
	XMFLOAT3 m_wire2_dir;

	//current wire position:
	XMFLOAT3 m_marker_pos;
	XMFLOAT3 m_marker_dir;
	XMMATRIX m_marker;
	XMMATRIX m_marker_translate;

	bool quit_request = false;

	wchar_t m_outputpath[30];
	bool m_running_assessment = false;
	bool m_practicing = false;
	int m_practice_wirenum = 0;

	//put in configuration file
	XMFLOAT3 m_apex = { 51.5f, -122.9f, -1123.f };
};


/////////////////////////
// FUNCTION PROTOTYPES //
/////////////////////////
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK dlgProcGetDetails(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK dlgProcAbout(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
/////////////
// GLOBALS //
/////////////
static SystemClass* ApplicationHandle = 0;



#endif