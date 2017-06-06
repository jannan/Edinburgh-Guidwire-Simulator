#include "ScenarioFileClass.h"



ScenarioFileClass::ScenarioFileClass()
{
}


ScenarioFileClass::~ScenarioFileClass()
{
}


// Load a scenario file - pass the path to the file
//will populate the class with all information to run simulation
bool ScenarioFileClass::LoadScenario(char* path, char* filename)
{
	strcpy(sc_path, path);
	strcat(sc_path, "\\");
	strcpy(sc_filename, filename);

	char fullpath[200] = {};
	strcpy(fullpath, sc_path);
	strcat(fullpath, sc_filename);

	strcpy(sc_rawpath, sc_path);
	strcat(sc_rawpath, "CT_formatted.raw");

	if (!parseTextFile(fullpath))
		return false;
}

//if we have a startup scenario all ready (by reading the global settings) then load else fail
bool ScenarioFileClass::LoadStartupScenario()
{
	if (!m_startup_scenario) return false;
	if (!m_startup_dir) return false;
	
	char filename[200];
	strcpy(sc_path, m_startup_dir);

	strcpy(sc_filename, m_startup_dir);
	strcat(sc_filename, m_startup_scenario);
	if (!parseTextFile(sc_filename)) return false;

	strcpy(sc_rawpath, m_startup_dir);
	strcat(sc_rawpath, "CT_formatted.raw");	//save filename of CT data
}

// Parse a text file line by line.  Return false if unable to open or not a valid file, otherwise true.
bool ScenarioFileClass::parseTextFile(char* filename)
{
	ifstream fin;
	fin.open(filename);
	if (!fin.good())
		return false;

	numLines = 0; //reset number of lines read

	while (!fin.eof())
	{
		numLines++;	//increment number of lines read
		char buf[MAX_CHARS_PER_LINE];
		fin.getline(buf, MAX_CHARS_PER_LINE);	//get current line

		int n = 0;
		const char* token[MAX_TOKENS_PER_LINE] = {};	//initialise to 0

		token[0] = strtok(buf, DELIMITER);	//get first token
		if (token[0])
		{
			//line not blank
			for (n = 1; n < MAX_TOKENS_PER_LINE; n++)
			{
				token[n] = strtok(0, DELIMITER);
				if (!token[n]) break;
			}

			//process the tokens in current line
			if (n > 1)
			{
				std::string tk0 = std::string(token[0]);
				//more than 1 token so parse this line
				if (tk0 == "#TITLE")
				{
					if (token[1]) strcpy(sc_title, token[1]);
				}
				else if (tk0 == "#DESCRIPTION")
				{
					if (token[1]) strcpy(sc_desc, token[1]);
				}
				else if (tk0 == "#AUTHOR")
				{
					if (token[1]) strcpy(sc_author, token[1]);
				}
				else if (tk0 == "#DATE")
				{
					if (token[1]) strcpy(sc_date, token[1]);
				}
				else if (tk0 == "#CT_PATH")
				{
					if (token[1])
					{
						strcpy(sc_ctpath, sc_path);
						strcat(sc_ctpath, token[1]);
					}
				}
				else if (tk0 == "#MODEL_PATH")
				{
					if (token[1])
					{
						strcpy(sc_modelpath, sc_path);
						strcat(sc_modelpath, token[1]);
					}
				}
				else if (tk0 == "#WIRE_LENGTH")
				{
					if (token[1]) sc_wirelength = atoi(token[1]);
				}
				else if (tk0 == "#MULTIWIRE") {
					sc_multiwire = true;
				}
				else if (tk0 == "#NUM_WIRES")
				{
					if (token[1]) sc_numWires = atoi(token[1]);
				}
				else if (tk0 == "#SCALE")
				{
					if (token[1]) sc_scale = atof(token[1]);
				}
				else if (tk0 == "#MODEL_OFFSET")
				{
					//subdivide into more tokens
					const char* token2[3] = {};
					token2[0] = strtok((char*)token[1], ",");
					token2[1] = strtok(0, ",");
					token2[2] = strtok(0, ",");
					if (token2[0] && token2[1] && token2[2])
					{
						//have to have all 3!
						sc_modeloffset.x = atof(token2[0]);
						sc_modeloffset.y = atof(token2[1]);
						sc_modeloffset.z = atof(token2[2]);
					}
				}
				else if (tk0 == "#NUM_XR")
				{
					if (token[1]) sc_numxr = atoi(token[1]);
					if (sc_numxr < 0) sc_numxr = 0;
					if (sc_numxr > 5) sc_numxr = 5;
				}
				else if (tk0 == "#TRANS_WORLD2CT")
				{
					//subdivide into more tokens
					const char* token2[3] = {};
					token2[0] = strtok((char*)token[1], ",");
					token2[1] = strtok(0, ",");
					token2[2] = strtok(0, ",");
					if (token2[0] && token2[1] && token2[2])
					{
						//have to have all 3!
						sc_world2ct.x = atof(token2[0]);
						sc_world2ct.y = atof(token2[1]);
						sc_world2ct.z = atof(token2[2]);
					}
				}
				else if (tk0 == "#HEAD_CENTRE")
				{
					//subdivide into more tokens
					const char* token2[3] = {};
					token2[0] = strtok((char*)token[1], ",");
					token2[1] = strtok(0, ",");
					token2[2] = strtok(0, ",");
					if (token2[0] && token2[1] && token2[2])
					{
						//have to have all 3!
						sc_head_centre.x = atof(token2[0]);
						sc_head_centre.y = atof(token2[1]);
						sc_head_centre.z = atof(token2[2]);
					}
				}
				else if (tk0 == "#XR1")
				{
					if (token[1] = "CENTRE")		//for some reason single = required! dont change this!
					{
						//subdivide into more tokens
						const char* token2[3] = {};
						token2[0] = strtok((char*)token[2], ",");
						token2[1] = strtok(0, ",");
						token2[2] = strtok(0, ",");
						if (token2[0] && token2[1] && token2[2])
						{
							//have to have all 3!
							sc_II_focus[0].x = atof(token2[0]);
							sc_II_focus[0].y = atof(token2[1]);
							sc_II_focus[0].z = atof(token2[2]);
						}
					}
				}

				//following definitions are for global settings file
				else if (tk0 == "#CHESSBOARD_SIZE") {if (token[1]) m_chessboard_size = atof(token[1]);}
				else if (tk0 == "#CAM_LEFT") { if (token[1]) m_cam_left = atoi(token[1]); }
				else if (tk0 == "#CAM_RIGHT") { if (token[1]) m_cam_right = atoi(token[1]); }
				else if (tk0 == "#FLIPLEFT") { m_cam_left_flip = true; }
				else if (tk0 == "#FLIPRIGHT") { m_cam_right_flip = true; }
				else if (tk0 == "#SCENARIO_DIR") { if (token[1]) strcpy(m_dir_scenarios, token[1]); }
				else if (tk0 == "#STARTUP_SCENARIO") { if (token[1]) strcpy(m_startup_scenario, token[1]); }
				else if (tk0 == "#STARTUP_DIR") { if (token[1]) strcpy(m_startup_dir, token[1]); }
				else if (tk0 == "#RESULTS_DIR") { if (token[1]) strcpy(m_dir_results, token[1]); }

			}
		}
	}
}

char* ScenarioFileClass::getStartupScenario()
{
	return m_startup_scenario;
}


float ScenarioFileClass::getChessboardSize()
{
	return m_chessboard_size;
}

bool ScenarioFileClass::parseFloat(float& result)
{
	return false;
}

//return the path to ct (vtk not raw)
char* ScenarioFileClass::get_ctpath()
{
	return sc_ctpath;
}

char* ScenarioFileClass::get_rawpath()
{
	return sc_rawpath;
}

//return the path to model stl file
char* ScenarioFileClass::get_modelpath()
{
	return sc_modelpath;
}

//get wirelength
int ScenarioFileClass::get_wirelength()
{
	return sc_wirelength;
}

//get scale
float ScenarioFileClass::get_scale()
{
	return sc_scale;
}

//get model offset
XMFLOAT3 ScenarioFileClass::get_modeloffset()
{
	return sc_modeloffset;
}

//set size of ct scan
void ScenarioFileClass::set_ctsize(float x, float y, float z)
{

}

XMFLOAT3 ScenarioFileClass::get_world2ct()
{
	return sc_world2ct;
}

XMFLOAT3 ScenarioFileClass::get_II_focus()
{
	return sc_II_focus[0];
}

// Load the config file
void ScenarioFileClass::LoadConfigFile(char* filename)
{
	parseTextFile(filename);
}
