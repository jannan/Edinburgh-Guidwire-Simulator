#pragma once
////////////////////////////////////////////////////////////////////////////////
// Filename: graphicsclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _GRAPHICSCLASS_H_
#define _GRAPHICSCLASS_H_




//debug settings
//#define DEBUG_SHOW_FEMUR
//#define DEBUG_SHOW_FEMORAL_HEAD_SPHERE
#define DEBUG_SET_WIRE_POS

//////////////
// INCLUDES //
//////////////
#include "d3dclass.h"
#include "cameraclass.h"
#include "modelclass.h"
#include "colourshaderclass.h"
#include "Lightshaderclass.h"
#include "lightclass.h"
#include "bitmapclass.h"

#include "ScenarioFileClass.h"
#include "VolumeClass.h"
#include "GeometryClass.h"
#include "stereovision.h"
#include "Textclass.h"
#include "Fontclass.h"
#include "util.h"

#include "ScreenGrab.h"

#include <time.h>



#define POINT_APEX 	{-51.54,-122.9,-1123}
#define GUIDEWIRE_LENGTH 145.f


/////////////
// GLOBALS //
/////////////
const bool FULL_SCREEN = false;
const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 20000.0f;
const float SCREEN_NEAR = 0.1f;
//We'll need these four globals to start with.

////////////////////////////////////////////////////////////////////////////////
// Class name: GraphicsClass
////////////////////////////////////////////////////////////////////////////////
class GraphicsClass
{
public:
	bool m_show_wire = true;
	bool m_show_DHS_screw = false;
	bool m_show_cannulated_screw = false;
	bool m_showmodel = false;
	bool m_showApexCoordinates = true;
	bool m_showWireNum = true;
	bool m_showTip = false;
	bool m_showApexCross = true;
	bool m_showTAD_text = true;
	bool m_showPolarView = true;
	bool m_showNeckVector = true;
	bool m_showIIImages = true;
	int m_currentwire = 0;
	XMFLOAT3 m_apex = { 0.f, 0.f, 0.f };

public:
	GraphicsClass();
	GraphicsClass(const GraphicsClass&);
	~GraphicsClass();

	bool Initialize(int screenWidth, int screenHeight, HWND hwnd, ScenarioFileClass * sc);
	bool fitCircle(vector<XMFLOAT3> points, XMFLOAT3 & center, XMFLOAT3 & dir, float & radius, GeometryClass * geom, bool display);
	void leastSquares(vector<XMFLOAT3>& points_transformed, XMFLOAT2 & c, float & radius, float search_width, float steps);
	void LeastSquaresSphere(vector<XMFLOAT3>& points, float & radius, XMFLOAT3 & c, float search_width, float steps);
	void Shutdown();
	bool Frame();

	GeometryClass* GetGeometry();

	void SetMarkerWarningVisible(bool visible);
	void SetMarkerVisible(bool visible);
	void SetModelVisible(bool visible);
	void SetGuidewireVisible(bool visible);
	void SetGuidewire2Visible(bool visible);
	void SetActiveRectangleVisible(bool visible);
	void exposure_increase();
	void exposure_decrease();
	void SetIIAngleAP(float angle);
	void SetIIAngleLat(float angle);
	float GetIIAngleAP(float angle);
	float GetIIAngleLat(float angle);
	void SetUseCTCoords(bool setting);
	void II_setwire();
	void II_setAP(bool isAP);
	void setWireMatrix(XMMATRIX w);
	void SetIIView();
	void RenderII_current();
	HRESULT SaveII(LPCWSTR filename);
	HRESULT SaveScreenshot(LPCWSTR filename, int frame);
	void RenderII_lat();

	void UpdateMarker(XMMATRIX);
	void MoveModel(XMMATRIX & model_dir, XMMATRIX & model_pos);
	void MoveWire(XMMATRIX & wire_dir, XMMATRIX & wire_pos);
	void SetWireFromNeckVector(XMFLOAT3 offset);
	void MoveWire2(XMMATRIX & wire_dir, XMMATRIX & wire_pos);
	void MoveWireOffset(float x, float y, float z);
	void MoveOffset(float x, float y, float z);
	bool SaveOffset();
	bool LoadOffset();

	void SaveMarker(XMFLOAT3 led1, XMFLOAT3 led2, XMFLOAT3 led3, XMFLOAT3 pos, XMFLOAT3 dir);
	void SetRenderState(char state);
	void StartTest();
	void PracticeMode();
	void toggleShowModel();
	void handleMouse(int x, int y, WPARAM wparam, HWND hWnd);
	void RenderII_AP();
	void RenderII(bool AP, float rot, float rot2);
	void setApex(XMFLOAT3 apex);
	void calculateTAD();
	float getTAD();
	float getTrue();
	int getXRNum();
	string getTime();
	void setShowApex(bool show);
	void setShowTip(bool show);
	void setShowScrew(bool show);
	bool isShowScrew();
	void setShowTAD(bool show);

	bool getheadsphereintersect(XMFLOAT3 tip, XMFLOAT3 & intersect);
	bool getApexOffset(XMFLOAT2 offset, XMFLOAT3 & result);

private:
	ScenarioFileClass* scenario;

	XMMATRIX marker2ctbox(int wirenum);

	bool Render(float);
	D3DClass* m_Direct3D;
	CameraClass* m_Camera;
	ModelClass* m_Model;
	ModelClass* m_Model2;
	ModelClass* m_Wire;
	ModelClass* m_Sphere;
	ModelClass* m_Screw;
	ModelClass* m_Cannulated;

	GeometryClass* m_Geometry;

	ColorShaderClass* m_ColorShader;
	LightShaderClass* m_LightShader;
	LightClass* m_Light;
	VolumeClass* m_Volume;

	XMMATRIX m_markermatrix;
	XMMATRIX m_modelmatrix;
	XMMATRIX m_wirematrix[3];
	XMMATRIX m_wire2matrix;
	XMMATRIX m_wirematrixII[3];	//to hold wire position for II shots - only updated when pedal pressed
	bool	 m_wireonII = false;

	XMMATRIX m_model_pos;
	XMMATRIX m_model_dir;
	XMMATRIX m_wire_pos;
	XMMATRIX m_wire_dir;
	XMMATRIX m_wire2_pos;
	XMMATRIX m_wire2_dir;

	XMMATRIX m_wireoffset;
	XMMATRIX m_offset;

	XMVECTOR m_ct_org;			//origin of ct scan
	XMVECTOR m_ct_markerloc;	//position of marker origin in ct coordinates
	
	XMFLOAT3 m_world2ct;

	XMFLOAT3 led1, led2, led3;	//location of marker leds
	XMFLOAT3 marker_pos;
	XMFLOAT3 marker_dir;

	//these are for viewing along neck axis and for moving relative to neck
	//used for polar head view and some TAD stuff
	XMMATRIX m_neckaxis;
	XMMATRIX m_neckaxisview;

	XMMATRIX m_II_mat_ap;
	XMMATRIX m_II_mat_lat;

	float m_screenwidth;
	float m_screenheight;

	float m_IIwidth;
	float m_IIheight;

	float m_II_AP_angle = -6.f;
	float m_II_lat_angle = 84.f;	//set to true lateral - todo: load these from scenario
	bool m_AP_active = TRUE;

	BitmapClass* m_Bitmap;

	char system_state;
#define GSTATE_3D 0
#define GSTATE_II 1
#define GSTATE_ALL 2

	TextureClass* m_tex_IIcircle = NULL;
	TextureClass* m_tex0 = NULL;
	TextureClass* m_selectbox = NULL;
	TextureClass* m_markernotvisible = NULL;
	TextureClass* m_tex_apex = NULL;
	TextureClass* m_tex_keys = NULL;
	TextureClass* m_arrow = NULL;

	bool m_modelvisible = FALSE;
	bool m_markervisible = FALSE;
	bool m_guidewirevisible = FALSE;
	bool m_guidewire2visible = FALSE;

	bool m_show_numxr = false;
	bool m_showTime = false;
	bool m_showMarkerVisible = true;	//shows the "Marker not visible" warning
	bool m_showActiveRectangle = true;	//draw a rectangle around active II image
	bool m_useCTWireCoords = false;		//if true, wire position is given in ct coordinates, not world coordinates

	int m_xr_count = 0;

	TextClass* m_textClass = 0;
	FontClass* m_fontClass = 0;
	
	//tip apex variables
	float m_TAD_AP = 0.f;
	float m_TAD_LAT = 0.f;
	float m_TAD = 0.f;
	float m_TAD_true = 0.f;

	time_t m_sim_start;
	time_t m_sim_total;
	time_t m_sim_limit;

	bool m_show_model = true;
	bool m_show_screw = true;

	//mouse interaction
	bool mouse_isdoing = false;		//set to true when interacting with image
	int mouse_which = 0;			//set to 0 for AP, 1 for lateral image
	int mouse_x = 0, mouse_y = 0;	//store location of mouse - used to reset as scrolling
	bool mouse_left = false;		//is left button down?
	bool mouse_right = false;		//is right button down?
	bool mouse_shift = false;
	bool mouse_ctrl = false;
	int mouse_totalx = 0;
	int mouse_totaly = 0;
};

#endif
