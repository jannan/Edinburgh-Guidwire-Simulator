////////////////////////////////////////////////////////////////////////////////
// Filename: graphicsclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "graphicsclass.h"


GraphicsClass::GraphicsClass()
{
	scenario = 0;

	m_Direct3D = 0;
	m_Camera = 0;
	m_Model = 0;
	m_Model2 = 0;
	m_Wire = 0;
	m_ColorShader = 0;
	m_Light = 0;
	m_Bitmap = 0;


	m_wireoffset = XMMatrixTranslation(0.0f, 0.0f, 0.0f);

	m_offset = XMMatrixTranslation(-103.0f, 121.0f, 1208.0f);		//translation to bring ct coordinates to world coordinates (in line with marker position)
	//m_offset = XMMatrixTranslation(-95.0f, 132.0f, 1223.0f);		//translation to bring ct coordinates to world coordinates (in line with marker position)
	//m_offset = XMMatrixTranslation(153, 312, 1295);


	m_textClass = 0;
	m_fontClass = 0;

	m_ct_org = XMLoadFloat3(&XMFLOAT3(-153.7f, -311.7f, 1294.f));			//origin of ct scan
	m_ct_markerloc = XMLoadFloat3(&XMFLOAT3(95.f, 132.f, -1233.f));		//position of marker origin in ct coordinates
}


GraphicsClass::GraphicsClass(const GraphicsClass& other)
{
}


GraphicsClass::~GraphicsClass()
{
}


bool GraphicsClass::Initialize(int screenWidth, int screenHeight, HWND hwnd, ScenarioFileClass* sc)
{
	m_show_model = false;
#ifdef II_SHOW_FEMUR
	m_show_model = true;
#endif

	scenario = sc;

	m_offset = XMMatrixTranslationFromVector(XMLoadFloat3(&sc->get_modeloffset()));


	bool result;
	m_screenwidth = (float)screenWidth;
	m_screenheight = (float)screenHeight;

	// Create the Direct3D object.
	m_Direct3D = new D3DClass;
	if (!m_Direct3D)
	{
		return false;
	}

	// Initialize the Direct3D object.
	result = m_Direct3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}


	// Create the camera object.
	m_Camera = new CameraClass;
	if (!m_Camera)
	{
		return false;
	}

	// Set the initial position of the camera.
	m_Camera->SetPosition(0.0f, 0.0f, 0.0f);

	// Create the model object.
	m_Model = new ModelClass;
	if (!m_Model)
	{
		return false;
	}

	// Initialize the model object.
	result = m_Model->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), MODEL_FEMUR, scenario->get_modelpath());
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the model femur object.", L"Error", MB_OK);
		return false;
	}

	//a second model
	m_Model2 = new ModelClass;
	if (!m_Model2)
	{
		return false;
	}
	result = m_Model2->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), MODEL_WIRE,"");
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the marker object.", L"Error", MB_OK);
		return false;
	}


	m_Wire = new ModelClass;
	result = m_Wire->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), MODEL_WIRE,"");
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the guidewire object.", L"Error", MB_OK);
		return false;
	}

	m_Screw = new ModelClass;
	result = m_Screw->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), MODEL_SCREW, "");

	m_Cannulated = new ModelClass;
	result = m_Cannulated->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), MODEL_CANNULATEDSCREW, "");

	//debug - load a sphere
	m_Sphere = new ModelClass;
	result = m_Sphere->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), MODEL_SPHERE,"");


	//load a geometry class to handle primitives
	m_Geometry = new GeometryClass;
	m_Geometry->Initialise(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext());


	/*
	int error = 3.f;
	//build a vector of points rotated around origin, pointing in dir
	XMFLOAT3 origin = {50, -25, 400 };
	XMFLOAT3 dir = {1.f, 0.f,-1.f };
	XMStoreFloat3(&dir, XMVector3Normalize(XMLoadFloat3(&dir)));
	float radius = 150;
	vector<XMFLOAT3> points;
	m_Geometry->AddSphere(origin, 0.05f, COLOUR_CYAN);
	m_Geometry->AddLine(origin, { (dir.x * 50) + origin.x, (dir.y * 50) + origin.y, (dir.z * 50) + origin.z }, COLOUR_CYAN);
	{
		XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
		XMVECTOR org = XMVectorSet(0.f, 0.f, 0.f, 0.f);
		XMVECTOR out = XMLoadFloat3(&dir);
		XMMATRIX r = XMMatrixTranspose(XMMatrixLookAtLH(org, out, up));
		XMMATRIX trans;

		srand(time(NULL));

		//add points around origin
		for (float angle = -30; angle < 30; angle += 1)
		{
			//XMFLOAT3 pnt = { 0, radius, 0 };
			float rad = 3.14159f / 180.f * angle;
			trans = XMMatrixRotationZ(rad);		//rotate point
			trans *= r;							//orient
			//add a random translation
			trans *= XMMatrixTranslation(rand() % error, rand() % error, rand() % error);

			trans *= XMMatrixTranslation(origin.x, origin.y, origin.z);
			XMVECTOR pt = XMVectorSet(0, radius, 0, 0);
			XMVECTOR pt_trans = XMVector3Transform(pt, trans);

			XMFLOAT3 newpt;
			XMStoreFloat3(&newpt, pt_trans);
			points.push_back(newpt);
			m_Geometry->AddSphere(points[points.size() - 1], .05f, COLOUR_CYAN);
		}
	}

	//find best fitting circle to get axis of rotation and direction
	//now working very well
	XMFLOAT3 center = { 0,0,0 }, dir_fit = { 0,0,0 };
	float r = 0.f;
	this->fitCircle(points, center, dir_fit, r, m_Geometry, true);


	//next - find a spherical center
	//so create some spherical test data points
	origin = { -50, 50, 400 };
	radius = 250;
	vector<XMFLOAT3>sphere;
	m_Geometry->AddSphere(origin, 0.2f, COLOUR_CYAN);
	for (float a1 = -90.f; a1 < 65; a1 += 5.f)
	{
		for (float a2 = 0.f; a2 < 65; a2 += 5.f)
		{
			XMMATRIX trans = XMMatrixRotationZ(a1 * (3.14159f / 180.f));
			trans *= XMMatrixRotationX(a2 * (3.14159f / 180.f));
			trans *= XMMatrixTranslation(rand() % error, rand() % error, rand() % error);
			trans *= XMMatrixTranslation(origin.x, origin.y, origin.z);
			XMVECTOR pt = XMVectorSet(0, radius, 0, 0);
			XMVECTOR pt_trans = XMVector3Transform(pt, trans);
			XMFLOAT3 newpt;
			XMStoreFloat3(&newpt, pt_trans);
			m_Geometry->AddSphere(newpt, 0.03f, COLOUR_CYAN);
			sphere.push_back(newpt);
		}
	}

	//have a set of points to fit a sphere to now
	//get the centroid
	XMFLOAT3 centroid = find_centroid(sphere);
	//m_Geometry->AddSphere(centroid, 0.2f, COLOUR_RED);
	//use the centroid as the starting center to search
	this->LeastSquaresSphere(sphere, radius, centroid, 300.f, 20.f);
	this->LeastSquaresSphere(sphere, radius, centroid, 20.f, 15.f);
	this->LeastSquaresSphere(sphere, radius, centroid, 2.f, 20.f);

	m_Geometry->AddSphere(centroid, 0.2f, COLOUR_WHITE);

	*/



	// Create the color shader object.
	m_ColorShader = new ColorShaderClass;
	if (!m_ColorShader)
	{
		return false;
	}

	// Initialize the color shader object.
	result = m_ColorShader->Initialize(m_Direct3D->GetDevice(), hwnd);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the color shader object.", L"Error", MB_OK);
		return false;
	}


	// Create the Light shader object.
	m_LightShader = new LightShaderClass;
	if (!m_LightShader)
	{
		return false;
	}

	// Initialize the color shader object.
	result = m_LightShader->Initialize(m_Direct3D->GetDevice(), hwnd);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the light shader object.", L"Error", MB_OK);
		return false;
	}

	// Create the light object.
	m_Light = new LightClass;
	if (!m_Light)
	{
		return false;
	}

	
	// Initialize the light object.
	m_Light->SetAmbientColor(.10f, .10f, .10f, 1.f);
	m_Light->SetDiffuseColor(.5f, .5f, .5f, 1.0f);
	m_Light->SetDirection(0.3f, -1.0f, 0.5f);
	m_Light->SetSpecularColor(.01f, .01f, .01f, 1.f);
	m_Light->SetSpecularPower(4.f);


	//initialise marker matrix
	m_Direct3D->GetWorldMatrix(m_markermatrix);
	m_Direct3D->GetWorldMatrix(m_modelmatrix);
	m_Direct3D->GetWorldMatrix(m_wirematrix[0]);
	m_Direct3D->GetWorldMatrix(m_wirematrix[1]);
	m_Direct3D->GetWorldMatrix(m_wirematrix[2]);

	//load offsets
	result = LoadOffset();

	//initialise volume class - for doing volume rendering
	m_IIwidth = 512;
	m_IIheight = 512;
	m_Volume = new VolumeClass;
	if (!m_Volume)
	{
		//return false;
	}
	m_Volume->Initialise(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), (uint) m_IIwidth, (uint)m_IIheight, scenario, m_Direct3D);


	// Create the bitmap object.
	m_Bitmap = new BitmapClass;
	if (!m_Bitmap)
	{
		return false;
	}

	// Initialize the bitmap object.
	//result = m_Bitmap->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, "tad.tga", 128, 26);
	result = m_Bitmap->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), screenWidth, screenHeight, "data\\textures\\II mask.tga", 200, 200);

	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the bitmap object.", L"Error", MB_OK);
		return false;
	}

	m_tex_IIcircle = new TextureClass;
	m_tex_IIcircle->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), "data\\textures\\II mask.tga");

	m_selectbox = new TextureClass;
	m_selectbox->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), "data\\textures\\select.tga");

	m_markernotvisible = new TextureClass;
	m_markernotvisible->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), "data\\textures\\marker not visible.tga");

	//create a test texture
	m_tex0 = new TextureClass;
	m_tex0->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), "data\\textures\\tad.tga");

	m_tex_apex = new TextureClass;
	m_tex_apex->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), "data\\textures\\apex marker.tga");

	m_tex_keys = new TextureClass;
	m_tex_keys->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), "data\\textures\\key_guide.tga");

	m_arrow = new TextureClass;
	m_arrow->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), "data\\textures\\arrow1.tga");

	m_textClass = new TextClass;
	m_textClass->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), m_ColorShader, hwnd, screenWidth, screenHeight, XMMatrixIdentity());

	m_fontClass = new FontClass;
	m_fontClass->Initialize(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext(), "data\\fonts\\font2.fnt", "data\\fonts\\font2_0.tga");

	return true;
}

//this function takes a vector of points and returns best fit circle - center, radius and direction vector
bool GraphicsClass::fitCircle(vector<XMFLOAT3> points, XMFLOAT3& center, XMFLOAT3& dir, float& radius, GeometryClass* geom, bool display)
{
	//1 -> find best fit plane and align all points onto that plane - this is direction sorted
	XMFLOAT3 centroid = { 0,0,0 };
	fitplane(points, dir, centroid);	//find the least squares best fit plane
	if(display)geom->AddSphere(centroid, 0.2f, COLOUR_PURPLE);	//show centroid

	//test the fit by drawing a line
	// now find plane normal and centroid.
	XMFLOAT3 dir_line_vector = { dir.x * 50, dir.y * 50, dir.z * 50 };
	dir_line_vector.x += centroid.x;
	dir_line_vector.y += centroid.y;
	dir_line_vector.z += centroid.z;
	if(display)geom->AddLine(centroid, dir_line_vector, COLOUR_PURPLE);

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
	vector<Point3f> pt; 
	Point3f av;
	for each(XMFLOAT3 p in points_transformed)
	{
		Point3f t;
		t.x = p.x;
		t.y = p.y;
		pt.push_back(t);
		av.x += t.x; av.y += t.y;
	}
	av.x /= pt.size(); av.y /= pt.size();
	radius = 80.f;	//guess based on rough size of marker
	XMFLOAT2 cent = { av.x, av.y };

	Mat(pt).convertTo(pointsf, CV_32F);
//	RotatedRect box = fitEllipse(pointsf);
	//radius = (box.size.width + box.size.height) / 4;


	//if (display)geom->AddSphere(XMFLOAT3(box.center.x, box.center.y, 200.f), 0.1f, COLOUR_YELLOW);

	//the above opencv fit ellipse function gives us an approximate starting location
	//from here need to use minimise least squares to reduce the error
	//problem is opencv is fitting an elipse not a circle, so only works well for a full circle

	//XMFLOAT2 cent = { box.center.x, box.center.y };
	this->leastSquares(points_transformed, cent, radius, 300.f, 60.f);
	//and again
	this->leastSquares(points_transformed, cent, radius, 12.f, 15.f);
	//and one last time to get really accurate
	this->leastSquares(points_transformed, cent, radius, 1.f, 10.f);
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
		if(display)geom->AddSphere(tmp, 0.1f, COLOUR_YELLOW);
	}

	//transform the center back to original space
	XMVECTOR c_vec = XMVector3Transform(XMLoadFloat3(&XMFLOAT3(cent.x, cent.y, 0.f)), m1);
	XMStoreFloat3(&center, c_vec);
	if(display)geom->AddSphere(center, 0.05f, COLOUR_YELLOW);

	dir_line_vector = { dir.x * 150, dir.y * 150, dir.z * 150 };
	dir_line_vector.x += center.x;
	dir_line_vector.y += center.y;
	dir_line_vector.z += center.z;
	if (display)geom->AddLine(center, dir_line_vector, COLOUR_PURPLE);

	dir_line_vector = { dir.x * -150, dir.y * -150, dir.z * -150 };
	dir_line_vector.x += center.x;
	dir_line_vector.y += center.y;
	dir_line_vector.z += center.z;
	if (display)geom->AddLine(center, dir_line_vector, COLOUR_PURPLE);

	return true;
}

void GraphicsClass::leastSquares(vector<XMFLOAT3>& points_transformed, XMFLOAT2& c, float& radius, float search_width, float steps)
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

	float err_min = 1000000000.f;
	float x_min = 0.f;
	float y_min = 0.f;
	float r_min = 0.f;

	for (float y = y1; y < y2; y += ystep)
	{
		for (float x = x1; x < x2; x += xstep)
		{
			float r_sample = 0.f;
			for each (XMFLOAT3 p in points_transformed)
			{
				float di = sqrt(((p.x - x)*(p.x - x)) + ((p.y - y)*(p.y - y)));
				r_sample += di;
			}
			r_sample /= points_transformed.size();

			//get error
			float err = 0.f;
			for each (XMFLOAT3 p in points_transformed)
			{
				float di = sqrt(((p.x - x)*(p.x - x)) + ((p.y - y)*(p.y - y)));
				float err_tmp = (di - r_sample) * (di - r_sample);
				err += err_tmp;
			}

			//err is sum of squares of error based on current circle
			if (err < err_min)
			{
				err_min = err;
				x_min = x;
				y_min = y;
				r_min = r_sample;
			}
		}
	}

	c.x = x_min;
	c.y = y_min;
	radius = r_min;

	return;
}

//find least squares sphere within search window specied at step interval
void GraphicsClass::LeastSquaresSphere(vector<XMFLOAT3>& points, float& radius, XMFLOAT3& c, float search_width, float steps)
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

	float err_min = 1000000000.f;
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
				float r_sample = 0.f;
				for each (XMFLOAT3 p in points)
				{
					float di = sqrt(((p.x - x)*(p.x - x)) + ((p.y - y)*(p.y - y)) + ((p.z - z) * (p.z - z)));
					r_sample += di;
				}
				r_sample /= points.size();
				//r_sample = 250;	//debug - fix radius
				//get error
				float err = 0.f;
				for each (XMFLOAT3 p in points)
				{
					float di = sqrt(((p.x - x)*(p.x - x)) + ((p.y - y)*(p.y - y)) + ((p.z - z) * (p.z - z)));
					float err_tmp = (di - r_sample) * (di - r_sample);
					err += err_tmp;
				}

				//err is sum of squares of error based on current circle
				if (err < err_min)
				{
					err_min = err;
					x_min = x;
					y_min = y;
					z_min = z;
					r_min = r_sample;
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

void GraphicsClass::Shutdown()
{
	// Release the light object.
	if (m_Light)
	{
		delete m_Light;
		m_Light = 0;
	}


	// Release the color shader object.
	if (m_ColorShader)
	{
		m_ColorShader->Shutdown();
		delete m_ColorShader;
		m_ColorShader = 0;
	}

	// Release the model object.
	if (m_Model)
	{
		m_Model->Shutdown();
		delete m_Model;
		m_Model = 0;
	}

	// Release the camera object.
	if (m_Camera)
	{
		delete m_Camera;
		m_Camera = 0;
	}

	//release volume object
	if (m_Volume)
	{
		delete m_Volume;
		m_Volume = 0;
	}

	// Release the Direct3D object.
	if (m_Direct3D)
	{
		m_Direct3D->Shutdown();
		delete m_Direct3D;
		m_Direct3D = 0;
	}

	// Release the bitmap object.
	if (m_Bitmap)
	{
		m_Bitmap->Shutdown();
		delete m_Bitmap;
		m_Bitmap = 0;
	}

	return;
}

bool GraphicsClass::Frame()
{
	bool result;


	// Render the graphics scene.
	result = Render(0.0f);
	if (!result)
	{
		return false;
	}

	return true;
}

GeometryClass* GraphicsClass::GetGeometry()
{
	return m_Geometry;
}

void GraphicsClass::SetMarkerWarningVisible(bool visible)
{
	m_showMarkerVisible = visible;
}

void GraphicsClass::SetModelVisible(bool visible)
{
	m_modelvisible = visible;
}

void GraphicsClass::SetMarkerVisible(bool visible)
{
	m_markervisible = visible;
}

void GraphicsClass::SetGuidewireVisible(bool visible)
{
	m_guidewirevisible = visible;
}
void GraphicsClass::SetGuidewire2Visible(bool visible)
{
	m_guidewire2visible = visible;
}

void GraphicsClass::SetActiveRectangleVisible(bool visible)
{
	m_showActiveRectangle = visible;
}

void GraphicsClass::exposure_increase()
{
	m_Volume->Exposure_Increase(m_AP_active);
	m_AP_active ? RenderII_AP() : RenderII_lat();
}
void GraphicsClass::exposure_decrease()
{
	m_Volume->Exposure_Decrease(m_AP_active);
	m_AP_active ? RenderII_AP() : RenderII_lat();
}

void GraphicsClass::SetIIAngleAP(float angle)
{
	m_II_AP_angle = angle;
	RenderII_AP();
	calculateTAD();
}
void GraphicsClass::SetIIAngleLat(float angle)
{
	m_II_lat_angle = angle;
	RenderII_lat();
	calculateTAD();
}
float GraphicsClass::GetIIAngleAP(float angle)
{
	return m_II_AP_angle;
}
float GraphicsClass::GetIIAngleLat(float angle)
{
	return m_II_lat_angle;
}

void GraphicsClass::SetUseCTCoords(bool setting)
{
	m_useCTWireCoords = setting;
}

void GraphicsClass::II_setwire()
{
	m_wirematrixII[m_currentwire] = m_wirematrix[m_currentwire];
}


void GraphicsClass::II_setAP(bool isAP)
{
	m_AP_active = isAP;
}

void GraphicsClass::setWireMatrix(XMMATRIX w)
{
	m_wirematrixII[0] = w;
	m_wirematrix[0] = w;
}

void GraphicsClass::SetIIView()
{
	//hide second guidewire shadow
	SetGuidewire2Visible(false);

	//all ready running - ie femur dropped so switch to II view mode
	RenderII_lat();	//dont increment
	RenderII_AP();	//the counter and dont save xray
	SetRenderState(GSTATE_II);
}

void GraphicsClass::RenderII_current()
{
	m_AP_active ? RenderII_AP() : RenderII_lat();
	m_xr_count++;

	//save the output
}

HRESULT GraphicsClass::SaveII(LPCWSTR filename)
{
	//save the current II texture to a file - incrementing filename number
	int i = 0;
	m_AP_active ? i = 0 : i = 1;
	LPCWSTR ending = L".png";
	wstring f = wstring(filename) + to_wstring(m_xr_count) + ending;
	HRESULT result = SaveWICTextureToFile(m_Direct3D->GetDeviceContext(), m_Volume->GetResource(i), GUID_ContainerFormatPng, (LPCWSTR)f.c_str());
	return result;
}

HRESULT GraphicsClass::SaveScreenshot(LPCWSTR filename, int frame)
{
	LPCWSTR ending = L".png";
	wstring f = wstring(filename) + ending;
	HRESULT result = SaveWICTextureToFile(m_Direct3D->GetDeviceContext(), m_Volume->GetResource(frame),GUID_ContainerFormatPng, (LPCWSTR)f.c_str());
	return result;
}

void GraphicsClass::RenderII_lat()
{
	RenderII(false, m_II_lat_angle, 10.f);
	//m_AP_active = false;
}

void GraphicsClass::RenderII_AP()
{
	RenderII(true, m_II_AP_angle, 10.f);
	//m_AP_active = true;
}

void GraphicsClass::RenderII(bool AP, float rot, float rot2)
{
	//function to draw all II AP stuff to a texture


#ifdef DEBUG_SET_WIRE_POS
	//used to set the wire from apex and offset instead of measuring wire
	//alternative - do this if no cameras plugged in
	//SetWireFromNeckVector(XMFLOAT3(0.f, 0.f, 0.f));		//get the new wire matrix based on neck vector and offset
	
#endif

	//XMVECTOR v_world2ct = XMLoadFloat3(&scenario->get_world2ct());
	XMVECTOR v_world2ct = XMLoadFloat3(&m_Volume->getWorld2CT());		//think this is translation to move model / subject coordinates to CT cube coordinates

	D3D11_VIEWPORT vp;
	XMFLOAT2 topleft = { 0,0 };
	XMFLOAT2 bottomright = { 0,0 };
	XMMATRIX worldMatrix = XMMatrixIdentity();
	XMMATRIX orthoMatrix = XMMatrixIdentity();
	bool result = false;

	int vp_width = (int)m_screenwidth;
	int vp_height = (int)m_screenheight;

	vp.Width = m_IIwidth;
	vp.Height = m_IIheight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_Direct3D->GetDeviceContext()->RSSetViewports(1, &vp);		//set active viewport to above

	topleft = { 0,0 };
	bottomright = {m_IIwidth,m_IIheight };


	//spin the II viewport
	m_Volume->setRotation(rot, AP, AP?0.f:-30.f);
	m_Volume->Render(m_Direct3D->GetDeviceContext(), topleft, bottomright, AP);

	//clear back buffer
	m_Direct3D->ClearDepthStencil();

	//reset normal backface culling - disabling this step stops mask from working
	m_Direct3D->SetRasterizerState();
	
	//turn on z buffer again
	m_Direct3D->TurnZBufferOn();

	//not sure why this doesnt work
	m_Direct3D->TurnOnAlphaBlending();


#ifdef DEBUG_SHOW_FEMUR
	{
		//Render the femur ontop of ct scan still onto the texture - DEBUG feature
		worldMatrix = XMMatrixTranslationFromVector(v_world2ct);
		m_Model->Render(m_Direct3D->GetDeviceContext());
		result = m_ColorShader->Render(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, m_Volume->getViewMatrix(), m_Volume->getProjectionMatrix(),
			m_Model->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), { 0.f, 1.f, 0.f, 0.5f });

		//result = m_LightShader->Render(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, m_Volume->getViewMatrix(), m_Volume->getProjectionMatrix(),
		//	m_Model->GetTexture(), m_Light->GetDirection(), m_Light->GetAmbientColor(), m_Light->GetDiffuseColor(), m_Camera->GetPosition(), m_Light->GetSpecularColor(), m_Light->GetSpecularPower());
	}
#endif

	//debug - render a sphere in femoral head
#ifdef DEBUG_SHOW_FEMORAL_HEAD_SPHERE
	{
		float sphere_diammeter = 48.f;
		sphere_diammeter -= 10.f;		//take away distance want tip of screw to be under subchondral bone
		sphere_diammeter /= 40.f;	//the sphere has diammeter of 40
		worldMatrix = XMMatrixScaling(sphere_diammeter, sphere_diammeter, sphere_diammeter);
		worldMatrix *= XMMatrixTranslationFromVector(XMLoadFloat3(&scenario->sc_head_centre));
		worldMatrix *= XMMatrixTranslationFromVector(v_world2ct);
		m_Sphere->Render(m_Direct3D->GetDeviceContext());
		result = m_ColorShader->Render(m_Direct3D->GetDeviceContext(), m_Sphere->GetIndexCount(), worldMatrix, m_Volume->getViewMatrix(), m_Volume->getProjectionMatrix(),
			m_Sphere->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), { 0.f, 1.f, 0.f, 0.3f });

	}
#endif

	XMVECTOR v_wire_tip;

	//show a line up neck - this DOESNT BELONG HERE! TEMPORARY for PROF SIMPSONS WIRES!!!!
	if (m_showNeckVector)
	{

	}


	XMVECTOR marker_offset = m_ct_markerloc - m_ct_org;

	for (int i = 0; i < 3; i++)
	{
		XMMATRIX wire2world = m_wirematrixII[i];			//wire2world is matrix to move camera coordinates to CT volume coordinates
		wire2world *= XMMatrixInverse(nullptr, m_model_pos);			//subtract model position
		wire2world *= XMMatrixTranspose(m_model_dir);					//derotate from model direction
		wire2world *= XMMatrixRotationY(3.14159f / 2.f);
		wire2world *= XMMatrixRotationZ(-3.14159f);					//rotate 180 degrees back
		wire2world *= XMMatrixInverse(nullptr, m_offset);		//finally move to marker position
		wire2world *= XMMatrixTranslationFromVector(v_world2ct);	//last step- translate to CT cube position from world coordinates

		//render wire using solid black pixel shader onto the texture
		if (!m_useCTWireCoords)
		{
			worldMatrix = wire2world;		//using real world coordinates from sensor
		}
		else
		{
			worldMatrix = m_wirematrixII[i] * XMMatrixTranslationFromVector(v_world2ct);	//using model coordinates - just need to move to CT render Cube
		}

		if (m_show_wire)
		{
			m_Wire->Render(m_Direct3D->GetDeviceContext());
			m_Direct3D->TurnOffAlphaBlending();
			if (i == m_currentwire)
			{
				result = m_ColorShader->RenderSolid(m_Direct3D->GetDeviceContext(), m_Wire->GetIndexCount(), worldMatrix, m_Volume->getViewMatrix(), m_Volume->getProjectionMatrix(),
					m_Wire->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor());
			}
			else
			{
				result = m_ColorShader->RenderSolid(m_Direct3D->GetDeviceContext(), m_Wire->GetIndexCount(), worldMatrix, m_Volume->getViewMatrix(), m_Volume->getProjectionMatrix(),
					m_Wire->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor());
			}
		}

		if (m_show_DHS_screw)
		{
			//render the screw on top of wire
			XMMATRIX tMat = XMMatrixRotationX(3.14159f) * XMMatrixTranslation(0.f, 0.f, 145.f);
			if (!m_useCTWireCoords)
			{
				worldMatrix = tMat * wire2world;		//using real world coordinates from sensor
			}
			else
			{
				worldMatrix = tMat * m_wirematrixII[i] * XMMatrixTranslationFromVector(v_world2ct);	//using ct coordinates
			}
			m_Screw->Render(m_Direct3D->GetDeviceContext());
			m_Direct3D->TurnOffAlphaBlending();
			result = m_ColorShader->RenderSolid(m_Direct3D->GetDeviceContext(), m_Screw->GetIndexCount(), worldMatrix, m_Volume->getViewMatrix(), m_Volume->getProjectionMatrix(),
				m_Screw->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor());
		}

		if (m_show_cannulated_screw)
		{
			//render a cannulated screw on top of guidewire
			XMMATRIX tMat = XMMatrixRotationX(3.14159f) * XMMatrixTranslation(0.f, 0.f, 145.f);
			if (!m_useCTWireCoords)
			{
				worldMatrix = tMat * wire2world;		//using real world coordinates from sensor
			}
			else
			{
				worldMatrix = tMat * m_wirematrixII[i] * XMMatrixTranslationFromVector(v_world2ct);	//using ct coordinates
			}
			m_Cannulated->Render(m_Direct3D->GetDeviceContext());
			m_Direct3D->TurnOffAlphaBlending();
			result = m_ColorShader->RenderSolid(m_Direct3D->GetDeviceContext(), m_Cannulated->GetIndexCount(), worldMatrix, m_Volume->getViewMatrix(), m_Volume->getProjectionMatrix(),
				m_Cannulated->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor());
		}

	}


	//display 2d bitmaps over the top
	orthoMatrix = XMMatrixOrthographicLH(m_IIwidth, m_IIheight, 100.f, 1000.f);
	m_Direct3D->TurnZBufferOff();
	m_Direct3D->TurnOnAlphaBlending();


	//show location of tip and apex using a red cross if enabled
	{
		XMVECTOR v_tip_pos = XMVectorSet(0.f, 0.f, GUIDEWIRE_LENGTH, 1.f);
		XMVECTOR apex_pos = XMLoadFloat3(&m_apex);

		XMMATRIX wire2world = m_wirematrixII[0];			//wire2world is matrix to move camera coordinates to CT volume coordinates
		if (!m_useCTWireCoords)
		{
			wire2world *= XMMatrixInverse(nullptr, m_model_pos);			//subtract model position
			wire2world *= XMMatrixTranspose(m_model_dir);					//derotate from model direction
			wire2world *= XMMatrixRotationY(3.14159f / 2.f);
			wire2world *= XMMatrixRotationZ(-3.14159f);					//rotate 180 degrees back
			wire2world *= XMMatrixInverse(nullptr, m_offset);
		}
		wire2world *= XMMatrixTranslationFromVector(v_world2ct);
		
		//wire2world = marker2ctbox(0);

		XMVECTOR tip_pos_t = XMVector3Project(v_tip_pos, 0.f, 0.f, m_IIwidth, m_IIheight, 100.f, 1000.f, m_Volume->getProjectionMatrix(), m_Volume->getViewMatrix(), wire2world);
		//XMVECTOR tip_pos_t = XMVector3Project(v_tip_pos, 0.f, 0.f, m_IIwidth, m_IIheight, 100.f, 1000.f, m_Volume->getProjectionMatrix(), m_Volume->getViewMatrix(), XMMatrixTranslation(189.7f, 311.7f, 1295.f));
		float tip_x = tip_pos_t.m128_f32[0] - (m_tex_apex->getWidth() / 2.f);
		float tip_y = tip_pos_t.m128_f32[1] - (m_tex_apex->getHeight() / 2.f);
		
		XMVECTOR apex_pos_t = XMVector3Project(apex_pos, 0.f, 0.f, m_IIwidth, m_IIheight, 100.f, 1000.f, m_Volume->getProjectionMatrix(), m_Volume->getViewMatrix(), XMMatrixTranslation(189.7f, 311.7f, 1295.f));
		float apex_x = apex_pos_t.m128_f32[0] - (m_tex_apex->getWidth() / 2.f);
		float apex_y = apex_pos_t.m128_f32[1] - (m_tex_apex->getHeight() / 2.f);

		//if display tip then show a cross at the wire tip
		if (m_showTip) //(m_showTip)
		{
			m_Bitmap->Resize((int)m_tex_apex->getWidth(), (int)m_tex_apex->getHeight());
			//m_Bitmap->Render(m_Direct3D->GetDeviceContext(), v_tip_pos.m128_f32[0] - (m_tex_apex->getWidth() / 2), v_tip_pos.m128_f32[1] - (m_tex_apex->getHeight() / 2), (int)m_IIwidth, (int)m_IIheight);
			m_Bitmap->Render(m_Direct3D->GetDeviceContext(), tip_x, tip_y, (int)m_IIwidth, (int)m_IIheight);
			m_ColorShader->RenderTexture(m_Direct3D->GetDeviceContext(), m_Bitmap->GetIndexCount(), XMMatrixIdentity(), XMMatrixIdentity(), orthoMatrix,
				m_tex_apex->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), COLOUR_WHITE);
		}

		//and at apex - if show apex
		if (m_showApexCross)
		{
			m_Bitmap->Resize((int)m_tex_apex->getWidth(), (int)m_tex_apex->getHeight());
			m_Bitmap->Render(m_Direct3D->GetDeviceContext(), apex_x, apex_y, (int)m_IIwidth, (int)m_IIheight);
			m_ColorShader->RenderTexture(m_Direct3D->GetDeviceContext(), m_Bitmap->GetIndexCount(), XMMatrixIdentity(), XMMatrixIdentity(), orthoMatrix,
				m_tex_apex->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), COLOUR_WHITE);
		}
	}

	//display mask over the top - still rendering to a texture
	m_Bitmap->Resize((int)m_IIwidth, (int)m_IIheight);
	m_Bitmap->Render(m_Direct3D->GetDeviceContext(), 0, 0, (int)m_IIwidth, (int)m_IIheight);
	m_ColorShader->RenderTexture(m_Direct3D->GetDeviceContext(), m_Bitmap->GetIndexCount(), XMMatrixIdentity(), XMMatrixIdentity(), orthoMatrix,
		m_tex_IIcircle->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), COLOUR_WHITE);

	//switch back to normal rendering mode
	m_Direct3D->TurnOffAlphaBlending();
	m_Direct3D->SetBackBufferRenderTarget();
	m_Direct3D->SetRasterizerState();

	return;
}

void GraphicsClass::setApex(XMFLOAT3 apex)
{
	m_apex = apex;

	//also redo neck vector
	XMFLOAT3 p1 = m_apex;
	XMFLOAT3 p2 = { 113.5f, -101.9f, -1196.0f };
	p2 = scenario->sc_head_centre;
	XMVECTOR v1 = XMLoadFloat3(&p1);
	XMVECTOR v2 = XMLoadFloat3(&p2);
	//create a matrix to rotate into view along neck axis.
	//Can now use this to move from 2d grid into neck axis
	XMVECTOR v_up = XMVectorSet(0.f, -1.f, 0.f, 0.f);
	m_neckaxisview = XMMatrixLookAtLH(v1, v2, v_up);

	m_neckaxisview *= XMMatrixRotationZ(10.f * 3.14159f / 180.f);	//rotate so AP up, lat sideways

	m_neckaxis = XMMatrixTranspose(m_neckaxisview);
}


void GraphicsClass::calculateTAD()
{
	//work out tip apex distance and true distance

	XMVECTOR _2ctbox = XMLoadFloat3(&m_Volume->getWorld2CT());	//this is translation vector to cT cube
	XMMATRIX _marker2ctbox = m_wirematrix[0];			//wire2world is matrix to move camera coordinates to CT volume coordinates
	if (!m_useCTWireCoords)	//Only move from camera sensor coordinates to model coordinates if using the sensor, otherwise all ready in model coordinates
	{
		_marker2ctbox *= XMMatrixInverse(nullptr, m_model_pos);			//subtract model position
		_marker2ctbox *= XMMatrixTranspose(m_model_dir);					//derotate from model direction
		_marker2ctbox *= XMMatrixRotationY(3.14159f / 2.f);
		_marker2ctbox *= XMMatrixRotationZ(-3.14159f);					//rotate 180 degrees back
		_marker2ctbox *= XMMatrixInverse(nullptr, m_offset);
	}
	_marker2ctbox *= XMMatrixTranslationFromVector(_2ctbox);	//final step - translate to CT rendering box

	XMVECTOR tip_pos = XMVectorSet(0.f, 0.f, GUIDEWIRE_LENGTH, 1.f);
	XMVECTOR apex_pos = XMLoadFloat3(&m_apex);	//load saved apex position in model coordinates
	apex_pos.m128_f32[3] = 1.f;		//set 4th component

	//set II rotation
	m_Volume->setRotation(m_II_AP_angle, true, 0.f);
	XMMATRIX ap_view = m_Volume->getViewMatrix();
	m_Volume->setRotation(m_II_lat_angle, false, -10.f);
	XMMATRIX lat_view = m_Volume->getViewMatrix();

	XMMATRIX t = XMMatrixTranslationFromVector(_2ctbox) * ap_view;
	XMVECTOR apex_pos_ap = XMVector4Transform(apex_pos, t);

	t = _marker2ctbox * ap_view;
	XMVECTOR tip_pos_ap = XMVector4Transform(tip_pos, t);
	//have rotated the points so now facing the camera.  Just ditch the z component to measure distance
	float tip_x = tip_pos_ap.m128_f32[0];
	float tip_y = tip_pos_ap.m128_f32[1];
	float apex_x = apex_pos_ap.m128_f32[0];
	float apex_y = apex_pos_ap.m128_f32[1];

	//work out distance
	float distance = sqrt(((tip_x - apex_x) * (tip_x - apex_x)) + ((tip_y - apex_y) * (tip_y - apex_y)));
	m_TAD_AP = distance;			//save in relevant field
	
	//recalculate full TAD
	t = XMMatrixTranslation(189.7f, 311.7f, 1295.f) * lat_view;
	XMVECTOR apex_pos_lat = XMVector4Transform(apex_pos, t);

	t = _marker2ctbox * lat_view;
	XMVECTOR tip_pos_lat = XMVector4Transform(tip_pos, t);
	//have rotated the points so now facing the camera.  Just ditch the z component to measure distance
	tip_x = tip_pos_lat.m128_f32[0];
	tip_y = tip_pos_lat.m128_f32[1];
	apex_x = apex_pos_lat.m128_f32[0];
	apex_y = apex_pos_lat.m128_f32[1];

	//work out distance
	distance = sqrt(((tip_x - apex_x) * (tip_x - apex_x)) + ((tip_y - apex_y) * (tip_y - apex_y)));
	m_TAD_LAT = distance;			//save in relevant field
	

	//Tip apex distance is sum of AP distance and lateral distance
	m_TAD = m_TAD_AP + m_TAD_LAT;

	//calcualte true distance
	XMVECTOR apex_pos_t = XMVector4Transform(apex_pos, XMMatrixTranslationFromVector(_2ctbox));
	XMVECTOR tip_pos_t = XMVector4Transform(tip_pos, _marker2ctbox);
	XMVECTOR di = XMVector3Length(XMVectorSubtract(tip_pos_t, apex_pos_t));
	m_TAD_true = di.m128_f32[0];
}

float GraphicsClass::getTAD()
{
	return m_TAD;
}
float GraphicsClass::getTrue()
{
	return m_TAD_true;
}
int GraphicsClass::getXRNum()
{
	return m_xr_count;
}
string GraphicsClass::getTime()
{
	//get current time in string format
	//timer
	time_t t_now = time(nullptr);
	double diff = difftime(t_now, m_sim_start);
	int seconds = (int)diff % 60;
	int minutes = (int)(diff / 60) % 60;

	ostringstream os_time;
	//os_time << "Time: ";
	os_time.width(2);
	os_time.fill('0');
	os_time << minutes << ":";
	os_time.width(2);
	os_time << seconds;

	string txt = os_time.str();

	return txt;
}

void GraphicsClass::setShowScrew(bool show)
{
	m_show_screw = show;
}
bool GraphicsClass::isShowScrew()
{
	return m_show_screw;
}

void GraphicsClass::setShowApex(bool show)
{
	m_showApexCross = show;
}
void GraphicsClass::setShowTip(bool show)
{
	m_showTip = show;
}

void GraphicsClass::setShowTAD(bool show)
{
	m_showTAD_text = show;
}

//return a matrix to translate wire position to ct rendering box ready for display
XMMATRIX GraphicsClass::marker2ctbox(int wirenum)
{
	XMVECTOR _2ctbox = XMLoadFloat3(&m_Volume->getWorld2CT());	//this is translation vector to cT cube
	XMMATRIX _marker2ctbox = m_wirematrix[wirenum];			//wire2world is matrix to move camera coordinates to CT volume coordinates
	if (!m_useCTWireCoords)	//Only move from camera sensor coordinates to model coordinates if using the sensor, otherwise all ready in model coordinates
	{
		_marker2ctbox *= XMMatrixInverse(nullptr, m_model_pos);			//subtract model position
		_marker2ctbox *= XMMatrixTranspose(m_model_dir);					//derotate from model direction
		_marker2ctbox *= XMMatrixRotationY(3.14159f / 2.f);
		_marker2ctbox *= XMMatrixRotationZ(-3.14159f);					//rotate 180 degrees back
		_marker2ctbox *= XMMatrixInverse(nullptr, m_offset);
	}
	//_marker2ctbox *= XMMatrixTranslationFromVector( _2ctbox);
	return _marker2ctbox;
}

//take a tip position and return sphere intersect to get equivalent below subchondral surface
//problem - not right on lateral view!
bool GraphicsClass::getheadsphereintersect(XMFLOAT3 tip, XMFLOAT3 &intersect)
{
	//load wire tip position
	XMVECTOR tip_pos = XMLoadFloat3(&tip);

	//load neck direction vector
	XMVECTOR direction = XMVector4Transform(XMVectorSet(0.f, 0.f, 1.f, 0.f), XMMatrixInverse(nullptr,m_neckaxisview));	//try to set direction to rotation of wire matrix
	
	XMFLOAT3 line_dir, line_org, int1, int2;
	XMStoreFloat3(&line_dir, direction);
	XMStoreFloat3(&line_org, tip_pos);
	int num_intersections = intersectRaySphere(scenario->sc_head_centre, 19.f, line_dir, line_org, int1, int2);

	XMFLOAT3 new_tip;
	if (num_intersections == 1)
	{
		intersect = int1;
		return true;
	}
	else if (num_intersections == 2)
	{
		float dist1 = XMVector3Length(XMVectorSubtract(XMLoadFloat3(&int1), XMLoadFloat3(&m_apex))).m128_f32[0];
		float dist2 = XMVector3Length(XMVectorSubtract(XMLoadFloat3(&int2), XMLoadFloat3(&m_apex))).m128_f32[0];
		(dist1 < dist2) ? intersect = int1 : intersect = int2;
		return true;
	}
	else
	{
		//was no intersection between wire and tip, so use old tip.
		XMStoreFloat3(&intersect, tip_pos);
		return false;
	}

}

bool GraphicsClass::getApexOffset(XMFLOAT2 offset, XMFLOAT3 &result)
{
	XMVECTOR tip = XMVectorSet(0.f, 0.f, 0.f, 1.f) + XMLoadFloat2(&offset);
	XMVECTOR tip_trans = XMVector4Transform(tip, XMMatrixInverse(nullptr,m_neckaxisview));
	XMStoreFloat3(&result, tip_trans);
	return true;
}


bool GraphicsClass::Render(float rotation)
{
	XMMATRIX worldMatrix, viewMatrix, projectionMatrix, orthoMatrix;
	bool result;

	float vp_width = m_screenwidth;
	float vp_height = m_screenheight;

	//make a viewport - a rectangle to draw to within frame buffer
	D3D11_VIEWPORT vp;

	if (system_state == GSTATE_3D)
	{
		vp.Width = m_screenwidth;
		vp.Height = m_screenheight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		m_Direct3D->GetDeviceContext()->RSSetViewports(1, &vp);		//set active viewport to above
	}
	else if (system_state == GSTATE_ALL)
	{
		vp_width = m_screenwidth / 2;
		vp_height = m_screenheight / 2;

		vp.Width = vp_width;
		vp.Height = vp_height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = vp_width / 2;
		vp.TopLeftY = vp_height;
		m_Direct3D->GetDeviceContext()->RSSetViewports(1, &vp);		//set active viewport to above
	}

	// Clear the buffers to begin the scene.
	m_Direct3D->BeginScene(0.0f, 0.0f, 0.0f, 1.f);

	// Generate the view matrix based on the camera's position.
	if (m_modelvisible)
	{
		XMVECTOR t = XMVector3Transform(XMVectorSet(0.f, 0.f, 0.f, 1.f), m_model_pos);
		XMFLOAT3 tmp;
		XMStoreFloat3(&tmp, t);
		m_Camera->Render(tmp);
	}
	else
	{
		m_Camera->Render({ 0.f, 0.f, 400.f });
	}

	// Get the world, view, and projection matrices from the camera and d3d objects.
	m_Direct3D->GetWorldMatrix(worldMatrix);
	m_Camera->GetViewMatrix(viewMatrix);
	m_Direct3D->GetProjectionMatrix(projectionMatrix);
	m_Direct3D->GetOrthoMatrix(orthoMatrix);

	//recalculate projectionMatrix based on new viewport data
	float fieldOfView = 3.141592654f / 4.0f;	//45 degrees
	float screenAspect = vp_width / vp_height;
	projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, 100.f, 1000.0f);

	if ((system_state == GSTATE_3D) || (system_state == GSTATE_ALL))
	{

		m_Direct3D->TurnZBufferOn();
		m_Direct3D->TurnOffAlphaBlending();
		m_Direct3D->SetBackBufferRenderTarget();
		m_Direct3D->SetRasterizerState();

		if (m_modelvisible)
		{
			worldMatrix = m_modelmatrix;
			m_Model->Render(m_Direct3D->GetDeviceContext());
			result = m_LightShader->Render(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
				m_Model->GetTexture(), m_Light->GetDirection(), m_Light->GetAmbientColor(), m_Light->GetDiffuseColor(), m_Camera->GetPosition(), m_Light->GetSpecularColor(), m_Light->GetSpecularPower());
		}

		/*
		//render the marker
		worldMatrix = m_markermatrix;
		m_Model2->Render(m_Direct3D->GetDeviceContext());
		result = m_ColorShader->Render(m_Direct3D->GetDeviceContext(), m_Model2->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
			m_Model2->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), XMFLOAT4(1.f, 0.f, 0.f, 1.f));
		if (!result)
		{
			return false;
		}
		*/

		//render the guidewire
		if (m_guidewirevisible)
		{
			for (int i = 0; i < 3; i++)
			{
				worldMatrix = m_wirematrix[i];
				m_Wire->Render(m_Direct3D->GetDeviceContext());
				if (i == m_currentwire)
				{
					result = m_ColorShader->Render(m_Direct3D->GetDeviceContext(), m_Wire->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
						m_Wire->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), XMFLOAT4(1.f, 0.f, 0.f, 1.f));
				}
				else
				{
					result = m_ColorShader->Render(m_Direct3D->GetDeviceContext(), m_Wire->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
						m_Wire->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), XMFLOAT4(0.f, 1.f, 0.f, 1.f));
				}
			}
		}

		//render the second guidewire
		if (m_guidewire2visible)
		{
			worldMatrix = m_wire2matrix;
			m_Wire->Render(m_Direct3D->GetDeviceContext());
			result = m_ColorShader->Render(m_Direct3D->GetDeviceContext(), m_Wire->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
				m_Wire->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), XMFLOAT4(0.1f, 0.4f, 0.f, 1.f));
		}

		//render all geometry primitives
		m_Geometry->Render(m_Direct3D->GetDeviceContext(), viewMatrix, projectionMatrix, m_ColorShader, m_Light);

		//render the marker led positions
		worldMatrix = XMMatrixScaling(0.2f, 0.2f, 0.2f);
		worldMatrix *= XMMatrixTranslation(led1.x, led1.y, led1.z);
		m_Sphere->Render(m_Direct3D->GetDeviceContext());
		result = m_ColorShader->Render(m_Direct3D->GetDeviceContext(), m_Sphere->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
			m_Sphere->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), COLOUR_CYAN);

		worldMatrix = XMMatrixScaling(0.2f, 0.2f, 0.2f);
		worldMatrix *= XMMatrixTranslation(led2.x, led2.y, led2.z);
		m_Sphere->Render(m_Direct3D->GetDeviceContext());
		result = m_ColorShader->Render(m_Direct3D->GetDeviceContext(), m_Sphere->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
			m_Sphere->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), COLOUR_PURPLE);

		worldMatrix = XMMatrixScaling(0.2f, 0.2f, 0.2f);
		worldMatrix *= XMMatrixTranslation(led3.x, led3.y, led3.z);
		m_Sphere->Render(m_Direct3D->GetDeviceContext());
		result = m_ColorShader->Render(m_Direct3D->GetDeviceContext(), m_Sphere->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
			m_Sphere->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), XMFLOAT4(1.f, 0.f, 0.f, 1.f));


	}

	orthoMatrix = XMMatrixOrthographicLH(vp_width, vp_height, 100.f, 2000.f);

	if ((system_state == GSTATE_ALL) || (system_state == GSTATE_II))
	{
		
		//reset to full screen rendering second viewport
		vp.Width = m_screenwidth;
		vp.Height = m_screenheight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		m_Direct3D->GetDeviceContext()->RSSetViewports(1, &vp);		//set active viewport to above

		if(1)
		//if (!m_showPolarView)
		{
			//display texture assembled above in 2d
			m_Direct3D->TurnZBufferOff();
			m_Bitmap->Resize((int)vp_width / 2, (int)vp_width / 2);
			m_Bitmap->Render(m_Direct3D->GetDeviceContext(), 1, 0, (int)vp_width, (int)vp_height);
			m_ColorShader->RenderTexture(m_Direct3D->GetDeviceContext(), m_Bitmap->GetIndexCount(), XMMatrixIdentity(), XMMatrixIdentity(), orthoMatrix,
				m_Volume->GetTexture(0), m_Light->GetDirection(), m_Light->GetDiffuseColor(), COLOUR_WHITE);


			//display lateral
			m_Bitmap->Render(m_Direct3D->GetDeviceContext(), (int)vp_width / 2, 0, (int)vp_width, (int)vp_height);
			m_ColorShader->RenderTexture(m_Direct3D->GetDeviceContext(), m_Bitmap->GetIndexCount(), XMMatrixIdentity(), XMMatrixIdentity(), orthoMatrix,
				m_Volume->GetTexture(1), m_Light->GetDirection(), m_Light->GetDiffuseColor(), COLOUR_WHITE);


			if (m_showActiveRectangle)
			{
				//draw a selection rectangle around active image
				if (m_AP_active)
				{
					m_Bitmap->Render(m_Direct3D->GetDeviceContext(), 1, 1, (int)vp_width, (int)vp_height);
				}
				else
				{
					m_Bitmap->Render(m_Direct3D->GetDeviceContext(), (int)vp_width / 2, 0, (int)vp_width, (int)vp_height);
				}
				m_Direct3D->TurnOnAlphaBlending();
				m_ColorShader->RenderTexture(m_Direct3D->GetDeviceContext(), m_Bitmap->GetIndexCount(), XMMatrixIdentity(), XMMatrixIdentity(), orthoMatrix,
					m_selectbox->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), COLOUR_WHITE);
			}
		}

		//render polar view if enabled
		if (m_showPolarView)
		{
			float width = 500.f;
			float height = 400.f;
			float topx = m_screenwidth-width;
			float topy = m_screenheight - height;

			//set viewport
			vp.Width = width;
			vp.Height = height;
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			vp.TopLeftX = topx;
			vp.TopLeftY = topy;
			m_Direct3D->GetDeviceContext()->RSSetViewports(1, &vp);		//set active viewport to above

			

			//redo projection matrix based on new viewport size
			screenAspect = width / height; // height / width;
			//projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, 10.f, 1000.0f);
			projectionMatrix = XMMatrixOrthographicLH(80.f, 80.f/screenAspect, 10.f, 1000.f);
//			projectionMatrix = XMMatrixOrthographicLH(width, height / screenAspect, 10.f, 1000.f);
			orthoMatrix = XMMatrixOrthographicLH(vp_width, vp_height, 0.f, 2000.f);

			//generate new view matrix to look at head, down neck.  Camera 50mm back from head as well.
			viewMatrix = m_neckaxisview * XMMatrixTranslation(0.f, 0.f, 50.f);
			worldMatrix = XMMatrixIdentity();


			
			//render model over top in translucent
			m_Direct3D->TurnZBufferOn();
			worldMatrix = XMMatrixIdentity();
			m_Model->Render(m_Direct3D->GetDeviceContext());
			result = m_ColorShader->Render(m_Direct3D->GetDeviceContext(), m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
				m_Model->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), { 1.f, 1.f, 0.f, 1.f });
			


			m_Direct3D->TurnZBufferOff();

			XMVECTOR _model2ct = XMLoadFloat3(&m_Volume->getWorld2CT());
			//set II rotation
			m_Volume->setRotation(m_II_lat_angle, false, -10.f);
			XMMATRIX lat_view = XMMatrixTranslationFromVector(_model2ct) * m_Volume->getViewMatrix();	//not transposed
			m_Volume->setRotation(m_II_AP_angle, true, 0.f);
			XMMATRIX ap_view = XMMatrixTranslationFromVector(_model2ct)	* m_Volume->getViewMatrix();	//not transposed


			float sphere_diammeter = 48.f / 40.f;
			worldMatrix = XMMatrixScaling(sphere_diammeter, sphere_diammeter, sphere_diammeter);
			worldMatrix *= XMMatrixTranslationFromVector(XMLoadFloat3(&scenario->sc_head_centre));
			m_Sphere->Render(m_Direct3D->GetDeviceContext());
			result = m_ColorShader->Render(m_Direct3D->GetDeviceContext(), m_Sphere->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
				m_Sphere->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), { 0.f, 0.f, 1.f, 1.f });


			sphere_diammeter = 48.f;	//this is the head diammeter in mm
			sphere_diammeter -= 10;		//take away distance want tip of screw to be under subchondral bone (half this)
			sphere_diammeter /= 40.f;	//the sphere model has diammeter of 40
			worldMatrix = XMMatrixScaling(sphere_diammeter, sphere_diammeter, sphere_diammeter);
			worldMatrix *= XMMatrixTranslationFromVector(XMLoadFloat3(&scenario->sc_head_centre));

			//render sphere in TAD colours
			result = m_ColorShader->RenderTAD(m_Direct3D->GetDeviceContext(), m_Sphere->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
				m_apex, ap_view, lat_view);

			//now draw the tip position
			XMMATRIX _marker2ctbox = marker2ctbox(0);	//load a matrix to translate marker to ct rendering cube
						
			//code section below gets wire tip location.
			//then tests wire intersection with the head sphere, set to offset below subchondral bone
			//choses intersection closed to the apex.
			XMVECTOR tip_pos = XMVectorSet(0.f, 0.f, GUIDEWIRE_LENGTH, 1.f);
			tip_pos = XMVector3Transform(tip_pos, _marker2ctbox);	//give tip position in model coordinates
			XMVECTOR entry_pos = XMVector3Transform(XMVectorSet(0.f, 0.f, 0.f, 1.f), _marker2ctbox);
			XMVECTOR direction = XMVectorSubtract(tip_pos, entry_pos);
						
			XMFLOAT3 line_dir, line_org, int1, int2;
			XMStoreFloat3(&line_dir, direction);
			XMStoreFloat3(&line_org, tip_pos);
			int num_intersections = intersectRaySphere(scenario->sc_head_centre, 19.f, line_dir, line_org, int1, int2);

			XMFLOAT3 new_tip;
			if (num_intersections == 1)
			{
				new_tip = int1;
			}

			
			else if (num_intersections == 2)
			{
				float dist1 = XMVector3Length(XMVectorSubtract(XMLoadFloat3(&int1), XMLoadFloat3(&m_apex))).m128_f32[0];
				float dist2 = XMVector3Length(XMVectorSubtract(XMLoadFloat3(&int2), XMLoadFloat3(&m_apex))).m128_f32[0];
				(dist1 < dist2) ? new_tip = int1 : new_tip = int2;
			}
			else
			{
				//was no intersection between wire and tip, so use old tip.
				XMStoreFloat3(&new_tip, tip_pos);
			}

			//m_Direct3D->TurnZBufferOff();

			//render a sphere at wire tip location projected onto sphere
			worldMatrix = XMMatrixScaling(.05f, .05f, .05f);
			worldMatrix *= XMMatrixTranslationFromVector(XMLoadFloat3(&new_tip));
			m_Sphere->Render(m_Direct3D->GetDeviceContext());
			result = m_ColorShader->Render(m_Direct3D->GetDeviceContext(), m_Sphere->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
				m_Sphere->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), { 1.f, 1.f, 1.f, 1.f });
			

			//render a cross at tip location
			m_Direct3D->TurnOnAlphaBlending();
			m_Direct3D->TurnZBufferOff();
			XMVECTOR v = XMVector3Transform(XMLoadFloat3(&new_tip), viewMatrix);
			//XMVECTOR v = XMVector3Project(XMLoadFloat3(&new_tip), 0.f, 0.f, vp.Width, vp.Height, 0.f, 100.f, projectionMatrix, viewMatrix, XMMatrixIdentity());
			float x = v.m128_f32[0] - (m_tex_apex->getWidth() / 2);
			float y = v.m128_f32[1] - (m_tex_apex->getHeight() / 2);
			//x = 0.f; y = 0.f;
			m_Bitmap->Resize((int)m_tex_apex->getWidth()*1, (int)m_tex_apex->getHeight()*1);
			m_Bitmap->Render(m_Direct3D->GetDeviceContext(), x, y, (int)vp.Width, (int)vp.Height);
			m_ColorShader->RenderTexture(m_Direct3D->GetDeviceContext(), m_Bitmap->GetIndexCount(), XMMatrixIdentity(), XMMatrixIdentity(), orthoMatrix,
				m_tex_apex->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), COLOUR_WHITE);


			//render a grid of spheres to test setup
#ifdef debug_showgridspheres			
			for (int i = -15; i < 16; i+=5)
			{
				for (int j = -15; j < 16; j+=5)
				{
					XMVECTOR pos = XMVectorSet((float)i, (float)j, 0.f, 1.f);	//set position
					pos = XMVector4Transform(pos, XMMatrixInverse(nullptr, m_neckaxisview));
					XMFLOAT3 p, p2; XMStoreFloat3(&p, pos);
					bool intersected = getheadsphereintersect(p, p2);
					if (!intersected) p2 = p;
					//render a sphere at wire tip location projected onto sphere
					worldMatrix = XMMatrixScaling(.05f, .05f, .05f);
					worldMatrix *= XMMatrixTranslationFromVector(XMLoadFloat3(&p2));
					m_Sphere->Render(m_Direct3D->GetDeviceContext());
					result = m_ColorShader->Render(m_Direct3D->GetDeviceContext(), m_Sphere->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix,
						m_Sphere->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), { 1.f, 1.f, 1.f, 1.f });

				}
			}
#endif

			//reset rendering options
			m_Direct3D->TurnZBufferOn();
			m_Direct3D->setWireFrame(false);
		}

		//reset to full screen rendering second viewport
		vp.Width = m_screenwidth;
		vp.Height = m_screenheight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		m_Direct3D->GetDeviceContext()->RSSetViewports(1, &vp);		//set active viewport to above


		//below is 2d overlay stuff

		m_Direct3D->TurnZBufferOff();
		m_Direct3D->TurnOnAlphaBlending();


		//display TAD if turned on
		if (m_showTAD_text)
		{
#ifdef SHOW_TAD_BREAKDOWN
			XMMATRIX orthoMatrix2 = orthoMatrix * XMMatrixScaling(0.5f, 0.5f, 1.f);
			ostringstream os;
			os.precision(1);	//set to 1 decimal place
			os << fixed << m_TAD << "mm";	//use "fixed" floating point notation - format flag manipulator in stream class
			string txt = "TAD:" + os.str();
			m_textClass->UpdateSentence(m_textClass->getSentence(0), (char*)txt.c_str(), -940, -510, (int)vp_width, (int)vp_height, 1.f, 1.f, 1.f, m_Direct3D->GetDeviceContext());
			m_textClass->Render(m_Direct3D->GetDeviceContext(), XMMatrixIdentity(), orthoMatrix2);

			ostringstream os2;
			os2.precision(1);
			os2 << fixed << m_TAD_true << "mm";
			txt = "True:" + os2.str();
			m_textClass->UpdateSentence(m_textClass->getSentence(0), (char*)txt.c_str(), -940, -470, (int)vp_width, (int)vp_height, 1.f, 1.f, 1.f, m_Direct3D->GetDeviceContext());
			m_textClass->Render(m_Direct3D->GetDeviceContext(), XMMatrixIdentity(), orthoMatrix2);

			
			os.str("");
			os << fixed << m_TAD_AP << "mm";
			txt = "AP distance:" + os.str();
			m_textClass->UpdateSentence(m_textClass->getSentence(0), (char*)txt.c_str(), -940, -430, (int)vp_width, (int)vp_height, 1.f, 1.f, 1.f, m_Direct3D->GetDeviceContext());
			m_textClass->Render(m_Direct3D->GetDeviceContext(), XMMatrixIdentity(), orthoMatrix2);

			os.str("");
			os << fixed << m_TAD_LAT << "mm";
			txt = "Lat distance:" + os.str();
			m_textClass->UpdateSentence(m_textClass->getSentence(0), (char*)txt.c_str(), -940, -390, (int)vp_width, (int)vp_height, 1.f, 1.f, 1.f, m_Direct3D->GetDeviceContext());
			m_textClass->Render(m_Direct3D->GetDeviceContext(), XMMatrixIdentity(), orthoMatrix2);
#else
			XMMATRIX orthoMatrix2 = orthoMatrix;
			ostringstream os;
			os.precision(1);	//set to 1 decimal place
			os << fixed << m_TAD << "mm";	//use "fixed" floating point notation - format flag manipulator in stream class
			string txt = "TAD:" + os.str();
			m_textClass->UpdateSentence(m_textClass->getSentence(0), (char*)txt.c_str(), 15, 50, (int)vp_width, (int)vp_height, 1.f, 1.f, 1.f, m_Direct3D->GetDeviceContext());
			m_textClass->Render(m_Direct3D->GetDeviceContext(), XMMatrixIdentity(), orthoMatrix2);
#endif

		}

		if (m_show_numxr)
		{
			//show number xrays taken
			ostringstream os;
			os << "Num XRs:" << m_xr_count;
			string txt = os.str();
			m_textClass->UpdateSentence(m_textClass->getSentence(0), (char*)txt.c_str(), 15, 95, (int)vp_width, (int)vp_height, 1.f, 1.f, 1.f, m_Direct3D->GetDeviceContext());
			m_textClass->Render(m_Direct3D->GetDeviceContext(), XMMatrixIdentity(), orthoMatrix);
		}

		if (m_showTime)
		{
			//timer
			time_t t_now = time(nullptr);
			double diff = difftime(t_now, m_sim_start);
			int seconds = (int)diff % 60;
			int minutes = (int)(diff / 60) % 60;

			ostringstream os_time;
			os_time << "Time: ";
			os_time.width(2);
			os_time.fill('0');
			os_time << minutes << ":";
			os_time.width(2);
			os_time << seconds;

			string txt = os_time.str();
			m_textClass->UpdateSentence(m_textClass->getSentence(0), (char*)txt.c_str(), 15, 145, (int)vp_width, (int)vp_height, 1.f, 1.f, 1.f, m_Direct3D->GetDeviceContext());
			m_textClass->Render(m_Direct3D->GetDeviceContext(), XMMatrixIdentity(), orthoMatrix);
		}

		if (m_showApexCoordinates)
		{
			//also show the current coordinates of apex
			ostringstream os;
			os.precision(2);
			os << fixed;
			os << "Apex:" << m_apex.x << "," << m_apex.y << "," << m_apex.z;
			string txt = os.str();
			m_textClass->UpdateSentence(m_textClass->getSentence(0), (char*)txt.c_str(), 15, 175, (int)vp_width, (int)vp_height, 1.f, 1.f, 1.f, m_Direct3D->GetDeviceContext());
			m_textClass->Render(m_Direct3D->GetDeviceContext(), XMMatrixIdentity(), orthoMatrix);

		}

		if (m_showWireNum)
		{
			//show current wire ID:
			ostringstream os;
			os << "Current wire: " << m_currentwire;
			m_textClass->UpdateSentence(m_textClass->getSentence(0), (char*)os.str().c_str(), 15, 10, (int)vp_width, (int)vp_height, 1.f, 1.f, 1.f, m_Direct3D->GetDeviceContext());
			m_textClass->Render(m_Direct3D->GetDeviceContext(), XMMatrixIdentity(), orthoMatrix);
		}
	}


	//if marker is not visible then render the warning message - only if set to display warning (default yes)
	if (!m_markervisible && m_showMarkerVisible)
	{
		m_Direct3D->TurnZBufferOff();
		m_Direct3D->TurnOnAlphaBlending();
		m_Bitmap->Resize(m_markernotvisible->getWidth(), m_markernotvisible->getHeight());
		m_Bitmap->Render(m_Direct3D->GetDeviceContext(), (int)(vp_width - m_markernotvisible->getWidth()) / 2, 50, (int)vp_width, (int)vp_height);
		m_ColorShader->RenderTexture(m_Direct3D->GetDeviceContext(), m_Bitmap->GetIndexCount(), XMMatrixIdentity(), XMMatrixIdentity(), orthoMatrix,
			m_markernotvisible->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), COLOUR_WHITE);
		m_Direct3D->TurnOffAlphaBlending();
		m_Direct3D->TurnZBufferOn();
	}

	//render some text

	//show the key guide

	m_Direct3D->TurnOnAlphaBlending();
	m_Direct3D->TurnZBufferOff();
	m_Bitmap->Resize(m_tex_keys->getWidth(), m_tex_keys->getHeight());
	m_Bitmap->Render(m_Direct3D->GetDeviceContext(), (int)50, (int)vp_height - m_tex_keys->getHeight(), (int)vp_width, (int)vp_height);
	m_ColorShader->RenderTexture(m_Direct3D->GetDeviceContext(), m_Bitmap->GetIndexCount(), XMMatrixIdentity(), XMMatrixIdentity(), orthoMatrix,
		m_tex_keys->GetTexture(), m_Light->GetDirection(), m_Light->GetDiffuseColor(), COLOUR_WHITE);
	m_Direct3D->TurnOffAlphaBlending();
	m_Direct3D->TurnZBufferOn();

	// Present the rendered scene to the screen.
	m_Direct3D->EndScene();

	return true;
}

//save location of leds and markers so can render positions
void GraphicsClass::SaveMarker(XMFLOAT3 l1, XMFLOAT3 l2, XMFLOAT3 l3, XMFLOAT3 pos, XMFLOAT3 dir)
{
	led1 = l1;
	led2 = l2;
	led3 = l3;
	marker_dir = dir;
	marker_pos = pos;
	return;
}

void GraphicsClass::UpdateMarker(XMMATRIX marker)
{
	m_markermatrix = marker;
}

void GraphicsClass::MoveModel(XMMATRIX& model_dir, XMMATRIX& model_pos)
{
	m_modelmatrix = m_offset;										//1.  Move femur from model coordinates to marker coordinates
	m_modelmatrix *= XMMatrixRotationZ(3.14159f);					//2.  spin around 180 degrees over z axis of femur - why is this happening?
	m_modelmatrix *= XMMatrixRotationY(-3.14159f/2);				//2.  spin around 180 degrees over y axis of femur
	m_modelmatrix *= model_dir;										//3.  spin to femur marker direction
	m_modelmatrix *= model_pos;										//4.  move to femur marker position

	//save positions
	m_model_pos = model_pos;
	m_model_dir = model_dir;
}

void GraphicsClass::MoveWire(XMMATRIX& wire_dir, XMMATRIX& wire_pos)
{
	m_wirematrix[m_currentwire] = wire_dir;	//orientate wire
	m_wirematrix[m_currentwire] *= XMMatrixTranslation(0.f, 0.f, -7.f);	//fudge to make wire correct length
	m_wirematrix[m_currentwire] *= wire_pos;	//and move

	//save positions
	m_wire_pos = wire_pos;
	m_wire_dir = wire_dir;
}


//get the wire matrix from the neck vector and a supplied offset
void GraphicsClass::SetWireFromNeckVector(XMFLOAT3 offset)
{

	//this needs to go somewhere else!
	//make up a new wire tip location
	XMVECTOR ctip = XMVectorSet(5.f, 5.f, 0.f, 1.f);
	XMVECTOR cdir = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	ctip = XMVector4Transform(ctip, XMMatrixInverse(nullptr, m_neckaxisview));	//take from neck view to model coords
	cdir = XMVector4Transform(cdir, XMMatrixInverse(nullptr, m_neckaxisview));		//dont translate this one, just rotate
	XMVECTOR centry = (cdir * GUIDEWIRE_LENGTH) + ctip;
	//XMStoreFloat3(&wire_tip, ctip);
	//XMStoreFloat3(&wire_dir, cdir);
	//
	//now have a tip, entry, dir.  Can make a matrix - how? look at with up vector
	//what is up vector/
	XMVECTOR v_up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	XMVECTOR v_eye = XMVectorSet(0.f, 0.f, 0.f, 0.f);
	XMMATRIX wire_dir = XMMatrixLookAtLH(v_eye, cdir, v_up);
	XMMATRIX wire_pos = XMMatrixTranslationFromVector(centry);
	//and save
	m_wirematrix[m_currentwire] = wire_dir;	//orientate wire
	//m_wirematrix[m_currentwire] *= XMMatrixTranslation(0.f, 0.f, -7.f);	//fudge to make wire correct length - shouldnt need this
	m_wirematrix[m_currentwire] *= wire_pos;	//and move
}

void GraphicsClass::MoveWire2(XMMATRIX& wire_dir, XMMATRIX& wire_pos)
{
	m_wire2matrix = wire_dir;	//orientate wire
	m_wire2matrix *= wire_pos;	//and move

	//m_wirematrix *= m_wireoffset;		//and move it relative to the model
	//save positions

	m_wire2_pos = wire_pos;
	m_wire2_dir = wire_dir;
}

void GraphicsClass::MoveWireOffset(float x, float y, float z)
{
	m_wireoffset *= XMMatrixTranslation(x, y, z);
}

void GraphicsClass::MoveOffset(float x, float y, float z)
{
	m_offset *= XMMatrixTranslation(x, y, z);
}

bool GraphicsClass::SaveOffset()
{
	//write current offsets to a file
	return false;
}

bool GraphicsClass::LoadOffset()
{
	//load the offsets from file
	return false;
}

void GraphicsClass::SetRenderState(char state)
{
	system_state = state;
}

//reset the assessment
void GraphicsClass::StartTest()
{
	m_sim_start = time(nullptr);
	m_xr_count = 0;
	m_TAD = 0;
	m_TAD_true = 0;

	m_showApexCross = false;
	m_showTip = false;
	m_showTAD_text = false;
	m_showTime = false;
	m_show_numxr = false;

	SetIIView();	//show II stuff
}

void GraphicsClass::PracticeMode()
{
	m_sim_start = time(nullptr);
	m_xr_count = 0;
	m_TAD = 0;
	m_TAD_true = 0;

	m_showApexCross = true;
	m_showTip = true;
	m_showTAD_text = true;
	m_showTime = true;
	m_show_numxr = true;

	m_modelvisible = false;
	m_show_screw = false;

	SetIIView();	//show II stuff

}

void GraphicsClass::toggleShowModel()
{
	m_show_model = !m_show_model;
	//RenderII_AP();
	//RenderII_lat();
	return;
}

//handle a mouse event
void GraphicsClass::handleMouse(int x, int y, WPARAM wparam, HWND hWnd)
{
	/*
		Left button drag - rotate
		+shift - translate
		+ctrl - zoom
		+alt - z rotate
	*/

	//button flags:
	bool lbutton = false, ctrl = false, shift = false, alt = false;

	//is currently interacting?
	if (wparam & MK_CONTROL)
	{
		//control down;
		ctrl = true;
	}
	if (wparam & MK_SHIFT)
	{
		//shift down
		shift = true;
	}
	if (wparam & MK_LBUTTON)
	{
		//left button
		lbutton = true;
	}
	if (wparam & MK_ALT)
	{
		alt = true;
	}

	if (mouse_isdoing)
	{
		//currently scrolling or interacting with something
		if (!lbutton)
		{
			//button released so stop doing whatever it was
			mouse_isdoing = false;
		}
		//else if moved out of zone then end (at the moment because mouse position set isnt working
		else
		{
			//still doing something - rotation, scroll, zoom, zrotation are options
			//act on new position
			//reset the mouse location
			POINT pnt = { x, y };
			ClientToScreen(hWnd, &pnt);
			//SetFocus(hWnd);
			SetCursorPos(pnt.x, pnt.y);	//doesnt work - why not?

			//work out new rotation
			//x and y
			int dx = x - mouse_x;
			int dy = y - mouse_y;

			float rotation = (float)dx * 1.f;
			if (abs(rotation) > 2.f)
			{
				mouse_x = x;
				mouse_y = y;
				if (!shift) {
					if (x < (m_screenwidth / 2))
					{
						SetIIAngleAP(GetIIAngleAP(0.f) + rotation);
						//RenderII_AP();
					}
					else
					{
						SetIIAngleLat(GetIIAngleLat(0.f) + rotation);
						//RenderII_lat();
					}
				}
			}
			
		}
	}
	else
	{
		//not currently doing anything
		if (lbutton)
		{
			mouse_isdoing = true;	//set to doing a task if mouse button down and over a target
			mouse_x = x;
			mouse_y = y;

			//get base rotations / translations
		}
		//first - are we over a target ? and which one
		//no - exit, ignore button
		//yes - process, start action
	}

}