#include "VolumeClass.h"



VolumeClass::VolumeClass()
{
}


VolumeClass::~VolumeClass()
{
}


bool VolumeClass::Initialise(ID3D11Device* device, ID3D11DeviceContext* devicecontext, UINT width, UINT height, ScenarioFileClass* sc, D3DClass* d3dclass)
{
	//save handle to scenario
	scenario = sc;

	//load focus position
	g_II_focus = scenario->get_II_focus();

	//save handle to d3dclass
	d3d = d3dclass;

	//save handle to device
	g_pd3dDevice = device;
	g_iWindowWidth = width;
	g_iWindowHeight = height;

	//create the shaders
	{
		//model position shaders
		{
			HRESULT result;
			ID3DBlob* pBlob = NULL;
			ID3DBlob* errorMessage;
			if (FAILED(D3DCompileFromFile(L"data\\shaders\\model_position.hlsl", NULL, NULL, "ModelPositionVS", "vs_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
				&pBlob, &errorMessage)))
			{
				return false;
			}
			device->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pVSModelPosition);
			//input layout:
			D3D11_INPUT_ELEMENT_DESC layout[] =
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			UINT numElements = ARRAYSIZE(layout);
			//create input layout
			device->CreateInputLayout(layout, numElements, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &g_pVLModelPosition);
			pBlob->Release();

			//pixel shader
			pBlob = NULL;
			if (FAILED(D3DCompileFromFile(L"data\\shaders\\model_position.hlsl", NULL, NULL, "ModelPositionPS", "ps_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &pBlob, &errorMessage)))
			{
				return false;
			}
			device->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pPSModelPosition);
			pBlob->Release();
		}

		//ray casting shaders
		{
			HRESULT result;
			ID3DBlob* pBlob = NULL;
			ID3DBlob* errorMessage;

			if (FAILED(D3DCompileFromFile(L"data\\shaders\\raycast.vs", NULL, NULL, "RayCastVS", "vs_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
				&pBlob, &errorMessage)))
			{
				return false;
			}
			device->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pVSRayCast);
			//input layout:
			D3D11_INPUT_ELEMENT_DESC layout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			UINT numElements = ARRAYSIZE(layout);
			//create input layout
			device->CreateInputLayout(layout, numElements, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &g_pVLRayCast);
			pBlob->Release();

			// Compile and create the pixel shaders
			pBlob = NULL;
			if (FAILED(D3DCompileFromFile(L"data\\shaders\\raycast.ps", NULL, NULL, "RayCastPS", "ps_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &pBlob, &errorMessage)))
			{

				return false;
			}
			device->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pPSRayCast);
			pBlob->Release();

		}


	}

	//create front and back render targets (2D textures)
	{
		D3D11_TEXTURE2D_DESC descTex;
		ZeroMemory(&descTex, sizeof(descTex));
		descTex.ArraySize = 1;
		descTex.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		descTex.Usage = D3D11_USAGE_DEFAULT;
		descTex.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		descTex.Width = g_iWindowWidth;
		descTex.Height = g_iWindowHeight;
		descTex.MipLevels = 1;
		descTex.SampleDesc.Count = 1;
		descTex.CPUAccessFlags = 0;

		// Front and back side
		for (int i = 0; i < 2; i++)
		{
			g_pd3dDevice->CreateTexture2D(&descTex, NULL, &g_pTexPosition[i]);//);

			// Create resource view
			g_pd3dDevice->CreateShaderResourceView(g_pTexPosition[i], NULL, &g_pTexPositionRV[i]);

			// Create render target view
			g_pd3dDevice->CreateRenderTargetView(g_pTexPosition[i], NULL, &g_pTexPositionRTV[i]);
		}
	}


	//Create the output textures to render the xrays to (
	//2 textures, one for AP, one for lateral)
	{
		D3D11_TEXTURE2D_DESC descTex;
		ZeroMemory(&descTex, sizeof(descTex));
		descTex.ArraySize = 1;
		descTex.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		descTex.Usage = D3D11_USAGE_DEFAULT;
		descTex.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		descTex.Width = g_iWindowWidth;
		descTex.Height = g_iWindowHeight;
		descTex.MipLevels = 1;
		descTex.SampleDesc.Count = 1;
		descTex.CPUAccessFlags = 0;


		g_pd3dDevice->CreateTexture2D(&descTex, NULL, &g_pTexOut[0]);
		g_pd3dDevice->CreateShaderResourceView(g_pTexOut[0], NULL, &g_pTexOutRV[0]);
		g_pd3dDevice->CreateRenderTargetView(g_pTexOut[0], NULL, &g_pTexOutRTV[0]);

		g_pd3dDevice->CreateTexture2D(&descTex, NULL, &g_pTexOut[1]);
		g_pd3dDevice->CreateShaderResourceView(g_pTexOut[1], NULL, &g_pTexOutRV[1]);
		g_pd3dDevice->CreateRenderTargetView(g_pTexOut[1], NULL, &g_pTexOutRTV[1]);
	}


	// Rasterizer states: front and back face culling
	{
		// Back face culling
		D3D11_RASTERIZER_DESC rasterizerDesc;
		ZeroMemory(&rasterizerDesc, sizeof(rasterizerDesc));
		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode = D3D11_CULL_BACK;
		rasterizerDesc.DepthClipEnable = true;
		//V_RETURN(
		g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &g_pRasterizerStateCullBack);//);

																						  // Front face culling
		ZeroMemory(&rasterizerDesc, sizeof(rasterizerDesc));
		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode = D3D11_CULL_FRONT;
		rasterizerDesc.DepthClipEnable = true;
		g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &g_pRasterizerStateCullFront);
	}

	// Create the sample state
	{
		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory(&sampDesc, sizeof(sampDesc));
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);//);
	}

	bool load_result = load_vtk(scenario->get_ctpath(), scenario->get_rawpath());
	//bool load_result = load_raw(scenario->get_ctpath(), scenario->get_rawpath());
	//bool load_result = load_raw(scenario->get_ctpath(), "scenarios\\dhs_right\\hip_formatted.raw");

	// Create ray-cast vertex and index buffers
	{
		float _x1 = 0.f;
		float _y1 = 0.f;
		float _z1 = 0.f;

		float _x2 = g_ct_width;
		float _y2 = g_ct_height;
		float _z2 = g_ct_depth;
		
		//vertex positions of bounding cube
		XMFLOAT3 v[] =
		{
			XMFLOAT3(_x1, _y1, _z1),
			XMFLOAT3(_x1, _y1, _z2),
			XMFLOAT3(_x1, _y2, _z1),
			XMFLOAT3(_x1, _y2,  _z2),
			XMFLOAT3(_x2, _y1, _z1),
			XMFLOAT3(_x2, _y1, _z2),
			XMFLOAT3(_x2,  _y2, _z1),
			XMFLOAT3(_x2,  _y2, _z2),
		};

		//texture positions - align ct scan data to fit bounding cube exactly
		XMFLOAT3 t[] =
		{
			XMFLOAT3(1.f, 0.f, 0.f),
			XMFLOAT3(1.f, 0.f,  1.f),
			XMFLOAT3(1.f,  1.f, 0.f),
			XMFLOAT3(1.f,  1.f,  1.f),
			XMFLOAT3(0.f, 0.f, 0.f),
			XMFLOAT3(0.f, 0.f,  1.f),
			XMFLOAT3(0.f,  1.f, 0.f),
			XMFLOAT3(0.f,  1.f,  1.f),
		};

		VertexType vertices[8];
		for (int i = 0; i < 8; ++i)
		{
			vertices[i].position.x = v[i].x;
			vertices[i].position.y = v[i].y;
			vertices[i].position.z = v[i].z;
			vertices[i].texture.x = t[i].x;
			vertices[i].texture.y = t[i].y;
			vertices[i].texture.z = t[i].z;
		}

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(VertexType) * ARRAYSIZE(vertices);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA initData;
		ZeroMemory(&initData, sizeof(initData));
		initData.pSysMem = vertices;
		g_pd3dDevice->CreateBuffer(&bd, &initData, &g_pVertexBufferCube);

		// Create index buffer
		WORD indices[] =
		{
			0, 1, 2,
			2, 1, 3,

			0, 4, 1,
			1, 4, 5,

			0, 2, 4,
			4, 2, 6,

			1, 5, 3,
			3, 5, 7,

			2, 3, 6,
			6, 3, 7,

			5, 4, 7,
			7, 4, 6,
		};
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.ByteWidth = sizeof(WORD) * ARRAYSIZE(indices);
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		ZeroMemory(&initData, sizeof(initData));
		initData.pSysMem = indices;
		g_pd3dDevice->CreateBuffer(&bd, &initData, &g_pIndexBufferCube);
	}


	// Constant buffers
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT; // D3D11_USAGE_DYNAMIC; //  D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(CBEveryFrameVS);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;// D3D11_CPU_ACCESS_WRITE; // was 0
		//bd.MiscFlags = 0;
		//bd.StructureByteStride = 0;
		g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pCBEveryFrameVS);

		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT; //  D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(CBEveryFramePS);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;// D3D11_CPU_ACCESS_WRITE; // was 0
		//bd.MiscFlags = 0;
		//bd.StructureByteStride = 0;
		g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pCBEveryFramePS);

		CBImmutable cb;
		cb.fInvWindowSize[0] = 1.f / g_iWindowWidth;
		cb.fInvWindowSize[1] = 1.f / g_iWindowHeight;

		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.ByteWidth = sizeof(CBImmutable);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;// D3D11_CPU_ACCESS_WRITE; // was 0
		//bd.MiscFlags = 0;
		//bd.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA initData;
		ZeroMemory(&initData, sizeof(initData));
		initData.pSysMem = &cb;
		g_pd3dDevice->CreateBuffer(&bd, &initData, &g_pCBImmutable);//);
	}

	// Matrices
	{
		// Initialize the view matrix - will be changed anyway on each render call
		//based on II angle
		//set at to centre of femoral head - focus on centre of head
		//set eye to correct rotation - should equal location of II source
		XMVECTOR eye = XMVectorSet(0.f, 1.5f, -5.5f, 0.f);
		XMVECTOR at = XMVectorSet(0.f, 0.2f, 0.f, 0.f);
		XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
		XMMATRIX mView = XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up));

		//Initialise field of view based on dimensions of image intensifier
		FLOAT II_diammeter = 350.0f;	//diammeter of detector
		FLOAT II_distance = 1000.0f;	//distance from source to detector
		float fov = 2 * atan((II_diammeter / 2) / II_distance);

		// Initialize the projection matrix
		float aspect = g_iWindowWidth / g_iWindowHeight;
		XMMATRIX mProjection = XMMatrixTranspose(XMMatrixPerspectiveFovLH(fov, aspect, 0.1f, 10.f));

		// View-projection matrix
		g_mViewProjection = XMMatrixMultiply(mProjection, mView);
	}

	return true;
}


void VolumeClass::Exposure_Increase(bool AP)
{
	AP? g_II_AP_exposure *= 0.9f : g_II_lat_exposure *= 0.9f;
}
void VolumeClass::Exposure_Decrease(bool AP)
{
	AP ? g_II_AP_exposure *= 1.1f : g_II_lat_exposure *= 1.1f;
}


void VolumeClass::setSize(float width, float height)
{
	g_iWindowWidth = width;
	g_iWindowHeight = height;
	return;
}

//set the II rotation and view matrix
//?put time delay here?
void VolumeClass::setRotation(float rotation, bool AP, float z_rot)
{

	rotation *= -3.14159f / 180.f;	//change to radians

	//eye is at II source
	XMVECTOR eye = XMVectorSet(0.f, -500.f, 0.f, 1.f);	//we are 500mm away from origin looked at
	
	//rotate eye location - so rotate II around
	XMMATRIX tm = XMMatrixIdentity();
	tm *= XMMatrixRotationZ(rotation);
	if (!AP)
	{
		tm *= XMMatrixRotationY(-55.f * (3.14159f / 180.f));	//axis is 45 degrees from horizontal
	}
	else
	{
		tm *= XMMatrixRotationY(-0.f * (3.14159f / 180.f));	//axis is 45 degrees from horizontal
	}

	tm *= XMMatrixTranslation(g_II_focus.x, g_II_focus.y, g_II_focus.z);	//center back on femoral neck
	eye = XMVector4Transform(eye, tm);
	
	XMVECTOR at = XMVectorSet(g_II_focus.x, g_II_focus.y, g_II_focus.z, 1.f);	//set to the position of the femoral head
	XMVECTOR up;
	if (AP)
	{
		up = XMVectorSet(0.f, 0.f, 1.f, 1.f);	//set to top of II image
	}
	else
	{	//lateral
		up = XMVectorSet(0.f, -1.f, 0.01f, 1.f);	//set to top of II image
		up = XMVector4Transform(up, XMMatrixRotationZ(20.f * (3.14159f / 180.f)));	//correction to keep view horizontal
	}
	XMVECTOR to = XMVector3Normalize(XMVectorSubtract(at, eye));
	XMMATRIX mView = XMMatrixLookAtLH(eye, at, up);
	g_m_view = mView;

	//Initialise field of view based on dimensions of image intensifier
	FLOAT II_diammeter = 250.0f;	//diammeter of detector
	FLOAT II_distance = 1000.0f;	//distance from source to detector
	float fov = 2 * atan((II_diammeter / 2) / II_distance);
	float fov_deg = fov * (180.f / 3.14159f);	//just for debug
	
	// Initialize the projection matrix
	float aspect = g_iWindowWidth / g_iWindowHeight;
	XMMATRIX mProjection = XMMatrixPerspectiveFovLH(fov, aspect, 0.1f, 10000.f);
	g_m_project = mProjection;

	// View-projection matrix
	g_mViewProjection = XMMatrixMultiply(mView, mProjection);

	//transpose FOR GPU
	g_mViewProjection = XMMatrixTranspose(g_mViewProjection);
}

//NOT TRANSPOSED
XMMATRIX VolumeClass::getProjectionMatrix()
{
	return g_m_project;
}

//NOTE NOT TRANSPOSED!
XMMATRIX VolumeClass::getViewMatrix()
{
	return g_m_view;
}

//return the offset to move world coordinates to ct voxel coordinates
XMFLOAT3 VolumeClass::getWorld2CT()
{
	XMFLOAT3 offset = {};
	offset.x = g_ct_width + g_ct_org_x;
	offset.y = -g_ct_org_y;
	offset.z = -g_ct_org_z;
	return offset;
}

//debug function to load "hip_formatted.raw"
bool VolumeClass::load_raw(char* vtk, char* raw)
{
	ifstream mFile;
	mFile.open(vtk, ios::in | ios::binary);
	if (!mFile.is_open())
	{
		return false;
	}

	//char line[200] = "";

	//get header data
	char line[10][200];
	mFile.getline(line[0], 200);
	mFile.getline(line[1], 200);
	mFile.getline(line[2], 200);
	mFile.getline(line[3], 200);
	mFile.getline(line[4], 200);
	mFile.getline(line[5], 200);
	mFile.getline(line[6], 200);
	mFile.getline(line[7], 200);
	mFile.getline(line[8], 200);
	mFile.getline(line[9], 200);
	mFile.close();

	//line[0] should start with "# VTK"
	if (string(line[0]).substr(0, 5) != string("# vtk"))return false;

	//line 3 should be "BINARY"
	if (string(line[2]) != string("BINARY")) return false;

	//get dimensions in line[4]
	size_t pos1 = string(line[4]).find(' ', 0);
	size_t pos2 = string(line[4]).find(' ', pos1 + 1);
	size_t pos3 = string(line[4]).find(' ', pos2 + 1);
	size_t pos4 = string(line[4]).find(' ', pos3 + 1);

	int ct_width = stoi(string(line[4]).substr(pos1 + 1, (pos2 - pos1 - 1)));
	int ct_height = stoi(string(line[4]).substr(pos2 + 1, (pos3 - pos2 - 1)));
	int ct_depth = stoi(string(line[4]).substr(pos3 + 1, (pos4 - pos3 - 1)));

	//get spacing values in line[5] "SPACING xxx yyyy zzzzzz"
	pos1 = string(line[5]).find(' ', 0);
	pos2 = string(line[5]).find(' ', pos1 + 1);
	pos3 = string(line[5]).find(' ', pos2 + 1);
	pos4 = string(line[5]).find(' ', pos3 + 1);

	float ct_space_x = stof(string(line[5]).substr(pos1 + 1, (pos2 - pos1 - 1)));
	float ct_space_y = stof(string(line[5]).substr(pos2 + 1, (pos3 - pos2 - 1)));
	float ct_space_z = stof(string(line[5]).substr(pos3 + 1, (pos4 - pos3 - 1)));

	//save size in mm (same as model units)
	g_ct_width = (float)ct_width * (float)ct_space_x;
	g_ct_height = (float)ct_height * (float)ct_space_y;
	g_ct_depth = (float)ct_depth * (float)ct_space_z;

	//get origin values in line[6] floats "
	pos1 = string(line[6]).find(' ', 0);
	pos2 = string(line[6]).find(' ', pos1 + 1);
	pos3 = string(line[6]).find(' ', pos2 + 1);
	pos4 = string(line[6]).find(' ', pos3 + 1);

	//save origin data (in mm - same as model)
	g_ct_org_x = stof(string(line[6]).substr(pos1 + 1, (pos2 - pos1 - 1)));
	g_ct_org_y = stof(string(line[6]).substr(pos2 + 1, (pos3 - pos2 - 1)));
	g_ct_org_z = stof(string(line[6]).substr(pos3 + 1, (pos4 - pos3 - 1)));



	//read actual data - ct_width * ct_height * ct_depth * 2 bytes
	int ct_size = ct_width * ct_height * ct_depth;


	//now load this from the raw file
	BYTE* buffer = (BYTE *)malloc(ct_size * sizeof(BYTE));		//allocate buffer
	mFile.open(raw, ios::out | ios::binary);
	mFile.read((char*)buffer, ct_size);

	//finally load into 3d texture on GPU

	D3D11_TEXTURE3D_DESC descTex;
	ZeroMemory(&descTex, sizeof(descTex));
	descTex.Height = ct_width;
	descTex.Width = ct_height;
	descTex.Depth = ct_depth;
	descTex.MipLevels = 1;
	descTex.Format = DXGI_FORMAT_R8_UNORM;
	descTex.Usage = D3D11_USAGE_IMMUTABLE;
	descTex.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	descTex.CPUAccessFlags = 0;
	// Initial data
	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = buffer;
	initData.SysMemPitch = ct_width;
	initData.SysMemSlicePitch = ct_width * ct_height;

	// Create texture
	g_pd3dDevice->CreateTexture3D(&descTex, &initData, &g_pTexVolume);

	// Create a resource view of the texture
	g_pd3dDevice->CreateShaderResourceView(g_pTexVolume, NULL, &g_pTexVolumeRV);

	free(buffer);



	return true;
}


//load a vtk file into the 3d texture - check if formatted raw file exists yet, make if not
bool VolumeClass::load_vtk(char* vtk, char* raw)
{
	bool rawfound = false;
	ifstream f(raw);
	if (f.good()) rawfound = true;
	f.close();

	ifstream mFile;
	mFile.open(vtk, ios::in | ios::binary);
	if (!mFile.is_open())
		return false;

	//get header data
	char line[10][200];
	mFile.getline(line[0], 200);
	mFile.getline(line[1], 200);
	mFile.getline(line[2], 200);
	mFile.getline(line[3], 200);
	mFile.getline(line[4], 200);
	mFile.getline(line[5], 200);
	mFile.getline(line[6], 200);
	mFile.getline(line[7], 200);
	mFile.getline(line[8], 200);
	mFile.getline(line[9], 200);

	//line[0] should start with "# VTK"
	if (string(line[0]).substr(0, 5) != string("# vtk"))return false;

	//line 3 should be "BINARY"
	if (string(line[2]) != string("BINARY")) return false;

	//get dimensions in line[4]
	size_t pos1 = string(line[4]).find(' ', 0);
	size_t pos2 = string(line[4]).find(' ', pos1 + 1);
	size_t pos3 = string(line[4]).find(' ', pos2 + 1);
	size_t pos4 = string(line[4]).find(' ', pos3 + 1);
	
	int ct_width = stoi(string(line[4]).substr(pos1 + 1, (pos2 - pos1 - 1)));
	int ct_height = stoi(string(line[4]).substr(pos2 + 1, (pos3 - pos2 - 1)));
	int ct_depth = stoi(string(line[4]).substr(pos3 + 1, (pos4 - pos3 - 1)));

	//get spacing values in line[5] "SPACING xxx yyyy zzzzzz"
	pos1 = string(line[5]).find(' ', 0);
	pos2 = string(line[5]).find(' ', pos1 + 1);
	pos3 = string(line[5]).find(' ', pos2 + 1);
	pos4 = string(line[5]).find(' ', pos3 + 1);

	float ct_space_x = stof(string(line[5]).substr(pos1 + 1, (pos2 - pos1 - 1)));
	float ct_space_y = stof(string(line[5]).substr(pos2 + 1, (pos3 - pos2 - 1)));
	float ct_space_z = stof(string(line[5]).substr(pos3 + 1, (pos4 - pos3 - 1)));

	g_ct_width = (float)ct_width * (float)ct_space_x;
	g_ct_height = (float)ct_height * (float)ct_space_y;
	g_ct_depth = (float)ct_depth * (float)ct_space_z;

	//get origin values in line[6] floats "
	pos1 = string(line[6]).find(' ', 0);
	pos2 = string(line[6]).find(' ', pos1 + 1);
	pos3 = string(line[6]).find(' ', pos2 + 1);
	pos4 = string(line[6]).find(' ', pos3 + 1);

	g_ct_org_x = stof(string(line[6]).substr(pos1 + 1, (pos2 - pos1 - 1)));
	g_ct_org_y = stof(string(line[6]).substr(pos2 + 1, (pos3 - pos2 - 1)));
	g_ct_org_z = stof(string(line[6]).substr(pos3 + 1, (pos4 - pos3 - 1)));

	//read actual data - ct_width * ct_height * ct_depth * 2 bytes
	int ct_size = ct_width * ct_height * ct_depth;
	BYTE* buffer = (BYTE *)malloc(ct_size * sizeof(BYTE));		//allocate buffer

	//so got header info from vtk file.
	//is there a raw formatted file yet at given location?
	if (!rawfound)
	{
		//doesnt exist so load data from vtk file then make the raw
		short min = 9999;
		short newmin = 9999;
		short newmax = -9999;
		//scale whole ct scan to be 8 bit values - this is very slow.  Save as raw
		for (int i = 0; i < ct_size; ++i)
		{
			byte b1 = mFile.get();
			byte b2 = mFile.get();
			short w = (b1 << 8) + b2;
			if (w < min) min = w;
			buffer[i] = (w + 1024) / 14;
		}
	}

	mFile.close();

	if (!rawfound)
	{
		//save formated ct scan as raw data
		ofstream fOut;
		fOut.open(raw, ios::out | ios::binary);
		fOut.write((char*)buffer, ct_size);
		fOut.close();	//close newly created raw file
	}

	//now should definately have a raw file, so load data from it.
	mFile.open(raw, ios::out | ios::binary);
	mFile.read((char*)buffer, ct_size);
	mFile.close();	//close vtk file

	//finally load into 3d texture on GPU
	D3D11_TEXTURE3D_DESC descTex;
	ZeroMemory(&descTex, sizeof(descTex));
	descTex.Height = ct_width;
	descTex.Width = ct_height;
	descTex.Depth = ct_depth;
	descTex.MipLevels = 1;
	descTex.Format = DXGI_FORMAT_R8_UNORM;
	descTex.Usage = D3D11_USAGE_IMMUTABLE;
	descTex.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	descTex.CPUAccessFlags = 0;
	// Initial data
	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = buffer;
	initData.SysMemPitch = ct_width;
	initData.SysMemSlicePitch = ct_width * ct_height;
	// Create texture
	g_pd3dDevice->CreateTexture3D(&descTex, &initData, &g_pTexVolume);

	// Create a resource view of the texture
	g_pd3dDevice->CreateShaderResourceView(g_pTexVolume, NULL, &g_pTexVolumeRV);

	free(buffer);

	return true;
}


//render the volume reconstruction to device context
bool VolumeClass::Render(ID3D11DeviceContext* deviceContext, XMFLOAT2 topLeft, XMFLOAT2 bottomRight, bool AP)
{
	g_pImmediateContext = deviceContext;
	float clearColor[4] = { 1.f, 0.1f, 0.2f, 1.f };	// red, green, blue, alpha

	UINT stride = sizeof(VertexType);
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &g_pVertexBufferCube, &stride, &offset);
	//set index buffer
	deviceContext->IASetIndexBuffer(g_pIndexBufferCube, DXGI_FORMAT_R16_UINT, 0);
	//set topology
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//layout
	deviceContext->IASetInputLayout(g_pVLModelPosition);

	//load world / view / projection matrix and update the buffer
	CBEveryFrameVS cb;
	cb.mWVP = g_mViewProjection;
	g_pImmediateContext->UpdateSubresource(g_pCBEveryFrameVS, 0, NULL, &cb, 0, 0);

	//render to position textures

	//set vertex shader
	g_pImmediateContext->VSSetShader(g_pVSModelPosition, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBEveryFrameVS);

	//set pixel shader
	g_pImmediateContext->PSSetShader(g_pPSModelPosition, NULL, 0);

	//front-face culling
	g_pImmediateContext->RSSetState(g_pRasterizerStateCullFront);
	g_pImmediateContext->ClearRenderTargetView(g_pTexPositionRTV[1], clearColor);
	g_pImmediateContext->OMSetRenderTargets(1, &g_pTexPositionRTV[1], NULL);
	g_pImmediateContext->DrawIndexed(36, 0, 0);	

	// Back-face culling
	g_pImmediateContext->RSSetState(g_pRasterizerStateCullBack);
	g_pImmediateContext->ClearRenderTargetView(g_pTexPositionRTV[0], clearColor);
	g_pImmediateContext->OMSetRenderTargets(1, &g_pTexPositionRTV[0], NULL);
	g_pImmediateContext->DrawIndexed(36, 0, 0);		
	

	// Ray-casting

	// Set the input layout
	g_pImmediateContext->IASetInputLayout(g_pVLModelPosition);

	//Render to output texture
	float c[4] = { 1.f, 1.f, 1.f, 1.f };	//debug - make the render texture blue
	int t = AP ? 0 : 1;
	g_pImmediateContext->ClearRenderTargetView(g_pTexOutRTV[t], c);
	g_pImmediateContext->OMSetRenderTargets(1, &g_pTexOutRTV[t], NULL);

	// Set the vertex shader
	g_pImmediateContext->VSSetShader(g_pVSRayCast, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBEveryFrameVS);
		
	//update cbuffer for pixel shader
	CBEveryFramePS cb_ps;
	cb_ps.exp = AP ? g_II_AP_exposure : g_II_lat_exposure;	//set correct exposure
	g_pImmediateContext->UpdateSubresource(g_pCBEveryFramePS, 0, NULL, &cb_ps, 0, 0);

	// Set the pixel shader
	g_pImmediateContext->PSSetShader(g_pPSRayCast, NULL, 0);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBImmutable);
	g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pCBEveryFramePS);

	// Set textures
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTexVolumeRV);
	g_pImmediateContext->PSSetShaderResources(1, 1, &g_pTexPositionRV[0]);
	g_pImmediateContext->PSSetShaderResources(2, 1, &g_pTexPositionRV[1]);

	// Set texture sampler
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	// Draw the cube
	g_pImmediateContext->DrawIndexed(36, 0, 0);

	//restore the depth stencil view to current render target
	g_pImmediateContext->OMSetRenderTargets(1, &g_pTexOutRTV[t],d3d->GetDepthStencilView());


	// Un-bind textures
	ID3D11ShaderResourceView *nullRV[3] = { NULL, NULL, NULL };
	g_pImmediateContext->PSSetShaderResources(0, 3, nullRV);

	return true;
}


ID3D11ShaderResourceView* VolumeClass::GetTexture(int i)
{
	if (i < 0) i = 0;
	if (i > 1) i = 1;

	return g_pTexOutRV[i];
}

ID3D11Resource* VolumeClass::GetResource(int i)
{
	if (i < 0) i = 0; if (i > 1) i = 1;
	return g_pTexOut[i];
}

ID3D11ShaderResourceView* VolumeClass::GetTextureFront()
{
	return g_pTexPositionRV[0];
}

ID3D11ShaderResourceView* VolumeClass::GetTextureBack()
{
	return g_pTexPositionRV[1];
}