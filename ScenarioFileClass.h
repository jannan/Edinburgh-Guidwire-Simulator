#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include "d3dclass.h"

using std::ifstream;
using namespace std;

const int MAX_CHARS_PER_LINE = 512;
const int MAX_TOKENS_PER_LINE = 20;
const char* const DELIMITER = ":";

const int MAX_XR_VIEWS = 5;

class ScenarioFileClass
{
public:
	int sc_numWires = 1;	//number of wires to handle
	bool sc_multiwire = false;	//weather using multiwire support (eg for cannulated screws)

	XMFLOAT3 sc_head_centre = { 0.f, 0.f, 0.f };

	//id of cameras to use
	int m_cam_left;
	int m_cam_right;
	//flags to flip a camera input over
	bool m_cam_left_flip = false;
	bool m_cam_right_flip = false;

	int m_start_mode = 0;			//which mode to start into: CALIB; XRAY
	bool m_start_showmodel = false;
	bool m_start_showwire = false;
	bool m_start_showdhs = false;
	bool m_start_showcann = false;

public:
	ScenarioFileClass();
	~ScenarioFileClass();
	// Load a scenario file
	bool LoadScenario(char* path, char* filename);
	bool LoadStartupScenario();

	float getChessboardSize();

private:
	char sc_path[200];	//path of current scenario
	char sc_filename[200]; //filename of current scenario
	int numLines = 0;	//number of lines parsed by file parser
	char sc_title[200];
	char sc_desc[200];
	char sc_author[50];
	char sc_date[10];
	char sc_ctpath[200];
	char sc_rawpath[200];
	char sc_modelpath[200];
	int sc_wirelength;
	float sc_scale;
	XMFLOAT3 sc_modeloffset;
	XMFLOAT3 sc_world2ct;

	//II setup data
	int sc_numxr;
	XMFLOAT3 sc_II_focus[MAX_XR_VIEWS];
	char sc_II_name[MAX_XR_VIEWS][10];
	int sc_II_distance[MAX_XR_VIEWS];
	float sc_II_angle[MAX_XR_VIEWS];
	float sc_II_angle_min[MAX_XR_VIEWS];
	float sc_II_angle_max[MAX_XR_VIEWS];
	float sc_II_brightness[MAX_XR_VIEWS];


	bool parseFloat(float & result);
public:
	// return path to ct scan file (vtk not raw)
	char* get_ctpath();
	char* get_rawpath();
	char* get_modelpath();
	int get_wirelength();
	float get_scale();
	XMFLOAT3 get_modeloffset();
	void set_ctsize(float x, float y, float z);
	XMFLOAT3 get_world2ct();

	XMFLOAT3 get_II_focus();
	// Load the config file
	void LoadConfigFile(char* filename);

	// Parse a text file line by line.  Return false if unable to open or not a valid file, otherwise true.
	bool parseTextFile(char* filename);
	char * getStartupScenario();
private:
	char m_startup_scenario[200];	//actual filename
	char m_startup_dir[200];	//store current scenario directory
	float m_chessboard_size;
	char m_dir_scenarios[200];
	char m_dir_results[200];

};

