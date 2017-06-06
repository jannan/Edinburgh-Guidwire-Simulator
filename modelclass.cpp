////////////////////////////////////////////////////////////////////////////////
// Filename: modelclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "modelclass.h"




ModelClass::ModelClass()
{
	m_vertexBuffer = 0;
	m_indexBuffer = 0;
	m_Texture = 0;
	m_model = 0;
}

ModelClass::ModelClass(const ModelClass& other)
{
}

ModelClass::~ModelClass()
{
}

bool ModelClass::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int model_type, char* filename)
{
	bool result;

	char* texture = "";

	if (model_type == MODEL_FEMUR)
	{
		//femur
		texture = "data\\textures\\bone.tga";
		if (!LoadFemur(filename,true)) 
			return false;
	}
	else if (model_type == MODEL_WIRE)
	{
		texture = "data\\textures\\blue.tga";
		if (!LoadWire()) 
			return false;
	}
	else if (model_type == MODEL_SCREW)
	{
		texture = "data\\textures\\blue.tga";
		if (!LoadScrew())
			return false;
	}
	else if (model_type == MODEL_CANNULATEDSCREW)
	{
		texture = "data\\textures\\blue.tga";
		if (!LoadSmallScrew())
			return false;
	}
	else if (model_type == MODEL_SPHERE)
	{
		texture = "data\\textures\\blue.tga";
		if (!LoadSphere()) return false;
	}
	else if (model_type == MODEL_LINE)
	{
		texture = "data\\textures\\blue.tga";
		if (!LoadStl("data\\models\\linemodel.stl", false)) return false;
	}


	// Initialize the vertex and index buffers.
	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}

	// Load the texture for this model.
	result = LoadTexture(device, deviceContext, texture);
	if (!result)
	{
		return false;
	}


	return true;
}

void ModelClass::Shutdown()
{
	// Release the model texture.
	ReleaseTexture();

	// Shutdown the vertex and index buffers.
	ShutdownBuffers();

	// Release the model data.
	ReleaseModel();

	return;
}

void ModelClass::Render(ID3D11DeviceContext* deviceContext)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(deviceContext);

	return;
}

int ModelClass::GetIndexCount()
{
	return m_indexCount;
}

ID3D11ShaderResourceView* ModelClass::GetTexture()
{
	return m_Texture->GetTexture();
}

bool ModelClass::InitializeBuffers(ID3D11Device* device)
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int i;

	// Create the vertex array.
	vertices = new VertexType[m_vertexCount];
	if (!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = new unsigned long[m_indexCount];
	if (!indices)
	{
		return false;
	}


	// Load the vertex array and index array with data.
	for (i = 0; i<m_vertexCount; i++)
	{
		vertices[i].position = XMFLOAT3(m_model[i].x, m_model[i].y, m_model[i].z);
		vertices[i].texture = XMFLOAT2(m_model[i].tu, m_model[i].tv);
		vertices[i].normal = XMFLOAT3(m_model[i].nx, m_model[i].ny, m_model[i].nz);

		indices[i] = i;
	}



	/*
					//now find all duplicated vertices, delete the duplicates and redo normals
					//also redo indices to reflect changes

					//make a list of duplicates
					int *duplicate = (int*)malloc(sizeof(int) * m_vertexCount);
					for (int i = 0; i < m_vertexCount; i++) { duplicate[i] = -1; }	//clear duplicate flags

					int num_unique_vertices = 0;

					for (int i = 0; i < m_vertexCount - 1; i++)
					{
						if (duplicate[i]<0)
						{
							num_unique_vertices++;
							int duplicate_count = 0;
							float nx, ny, nz;
							nx = m_model[i].nx; ny = m_model[i].ny; nz = m_model[i].nz;		//initialise normal
							for (int j = i + 1; j < m_vertexCount; j++)
							{
								//is this vertex different to i?
								if ((m_model[i].x == m_model[j].x) && (m_model[i].y == m_model[j].y) && (m_model[i].z == m_model[j].z))
								{
									//got a duplicate
									duplicate_count++;
									duplicate[j] = i;	//set corresponding unique index (for use once this is deleted)
									nx += m_model[j].nx; ny += m_model[j].ny;  nz += m_model[j].nz;	//add normal to average
								}
							}
							if (duplicate_count > 0)
							{
								//this was a duplicated vertex so average normals
								m_model[i].nx = nx / duplicate_count;
								m_model[i].nz = ny / duplicate_count;
								m_model[i].nz = nz / duplicate_count;
							}
						}
					}

					//now know which are duplicates.  Can build a new array removing them
					ModelType* model2 = new ModelType[num_unique_vertices];
					int j = 0;

					for (int i = 0; i < m_vertexCount; i++)		//for each old vertex
					{
						if (duplicate[i] < 0)		//only if this was not a duplicate
						{
							//add to new list
							model2[j] = m_model[i];	j++;

							//update the index

						}
						else
						{
							//this was a duplicate.
							//dont add to list but still need to update the index list.
						}
					}



	*/


	// Set up the description of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	return true;
}

void ModelClass::ShutdownBuffers()
{
	// Release the index buffer.
	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = 0;
	}

	// Release the vertex buffer.
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = 0;
	}

	return;
}

void ModelClass::RenderBuffers(ID3D11DeviceContext* deviceContext)
{
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}

bool ModelClass::LoadTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* filename)
{
	bool result;


	// Create the texture object.
	m_Texture = new TextureClass;
	if (!m_Texture)
	{
		return false;
	}

	// Initialize the texture object.
	result = m_Texture->Initialize(device, deviceContext, filename);
	if (!result)
	{
		return false;
	}

	return true;
}

void ModelClass::ReleaseTexture()
{
	// Release the texture object.
	if (m_Texture)
	{
		m_Texture->Shutdown();
		delete m_Texture;
		m_Texture = 0;
	}

	return;
}

bool ModelClass::LoadFemur(char* filename, bool flip_y)
{
	bool result;

	//load model - flip y axis for femur
	//if (!LoadStl("data\\hip_sim\\hip_sim_v1.stl", true)) {
	if (!LoadStl(filename,flip_y))
		return false;


	//translate to origin
//	float origin_x = 42.0f;
//	float origin_y = -128.0f;
//	float origin_z = -1233.0f;

	float origin_x = 0.f;
	float origin_y = 0.f;
	float origin_z = 0.f;


	//for (int i = 0; i < m_vertexCount; ++i)
	//{
	//	m_model[i].x -= origin_x;
	//	m_model[i].y -= origin_y;
	//	m_model[i].z -= origin_z;
	//}
	
	return true;
}

bool ModelClass::LoadMarker()
{
	bool result;

	//load marker
	if (!LoadStl("data\\models\\marker.stl", false)) return false;

	//translate to origin - this is location of lead point of marker in model coordinates

	float origin_x = 0.0f;
	float origin_y = 0.0f;
	float origin_z = 0.0f;

	for (int i = 0; i < m_vertexCount; ++i)
	{
		m_model[i].x -= origin_x;
		m_model[i].y -= origin_y;
		m_model[i].z -= origin_z;
	}

	return true;
}

bool ModelClass::LoadWire()
{
	bool result;

	//load guidewire
	if (!LoadStl("data\\models\\guidewire.stl", false)) return false;

	//translate to origin
	float origin_x = 0.0f;
	float origin_y = 0.0f;
	float origin_z = 0.0f;

	for (int i = 0; i < m_vertexCount; ++i)
	{
		m_model[i].x -= origin_x;
		m_model[i].y -= origin_y;
		m_model[i].z -= origin_z;
	}

	return true;
}

bool ModelClass::LoadScrew()
{
	//load screw
	if (!LoadStl("data\\models\\dhs screw.stl", false)) return false;

	return true;
}

bool ModelClass::LoadSmallScrew()
{
	//load screw
	if (!LoadStl("data\\models\\cannulated screw.stl", false)) return false;

	return true;
}

bool ModelClass::LoadSphere()
{
	//load sphere
	if (!LoadStl("data\\models\\sphere small.stl", false)) return false;

	return true;
}

bool ModelClass::LoadModel(char* filename, bool flipy)
{
	//check extension of filename
	//launch stl reader if appropriate
	return LoadStl(filename, flipy);
	

	ifstream fin;
	char input;
	int i;


	// Open the model file.
	fin.open(filename);

	// If it could not open the file then exit.
	if (fin.fail())
	{
		return false;
	}

	// Read up to the value of vertex count.
	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}

	// Read in the vertex count.
	fin >> m_vertexCount;

	// Set the number of indices to be the same as the vertex count.
	m_indexCount = m_vertexCount;

	// Create the model using the vertex count that was read in.
	m_model = new ModelVertexType[m_vertexCount];
	if (!m_model)
	{
		return false;
	}

	// Read up to the beginning of the data.
	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}
	fin.get(input);
	fin.get(input);

	// Read in the vertex data.
	for (i = 0; i<m_vertexCount; i++)
	{
		fin >> m_model[i].x >> m_model[i].y >> m_model[i].z;
		fin >> m_model[i].tu >> m_model[i].tv;
		fin >> m_model[i].nx >> m_model[i].ny >> m_model[i].nz;
	}

	// Close the model file.
	fin.close();

	return true;
}

bool ModelClass::LoadStl(char* filename, bool flipy)
{
	ifstream fin;
	char input;

	fin.open(filename, ifstream::binary);
	// If it could not open the file then exit.
	if (fin.fail())
	{
		return false;
	}


	for (int i = 0; i < 80; i++)
	{
		//read header 80 bytes and discard
		fin.get(input);
	}

	//all little endian

	//num facets
	//unsigned char facecount[sizeof(unsigned int)]
	//fin.read(reinterpret_cast<char*>(facecount), sizeof(unsigned int));

	unsigned int facecount;
	fin.read((char*)&facecount, sizeof(unsigned int)).gcount() == sizeof(facecount);

	m_vertexCount = facecount * 3;
	m_indexCount = m_vertexCount;
	m_model = new ModelVertexType[m_vertexCount];

	XMFLOAT3 av = XMFLOAT3(0.0f,0.0f,0.0f);

	float inf = std::numeric_limits<float>::infinity();
	float minimum = std::numeric_limits<float>::lowest();
	
	float minx = inf, maxx = minimum;
	float miny = inf, maxy = minimum;
	float minz = inf, maxz = minimum;

	for (int i = 0; i < m_vertexCount; i++)
	{
		float tmp;

		float x1, y1, z1, x2, y2, z2, x3, y3, z3, nx, ny, nz;
		//read data normals
		fin.read((char*)&nx, sizeof(nx)).gcount() == sizeof(nx);
		fin.read((char*)&ny, sizeof(ny)).gcount() == sizeof(ny);
		fin.read((char*)&nz, sizeof(nz)).gcount() == sizeof(nz);

		fin.read((char*)&x1, sizeof(x1)).gcount() == sizeof(x1);
		fin.read((char*)&y1, sizeof(y1)).gcount() == sizeof(y1);
		fin.read((char*)&z1, sizeof(z1)).gcount() == sizeof(z1);

		fin.read((char*)&x2, sizeof(x1)).gcount() == sizeof(x1);
		fin.read((char*)&y2, sizeof(y1)).gcount() == sizeof(y1);
		fin.read((char*)&z2, sizeof(z1)).gcount() == sizeof(z1);

		fin.read((char*)&x3, sizeof(x1)).gcount() == sizeof(x1);
		fin.read((char*)&y3, sizeof(y1)).gcount() == sizeof(y1);
		fin.read((char*)&z3, sizeof(z1)).gcount() == sizeof(z1);

		if (flipy)
		{
			y1 = -y1;
			y2 = -y2;
			y3 = -y3;
		}

		//add to average
		av.x += x1 + x2 + x3;
		av.y += y1 + y2 + y3;
		av.z += z1 + z2 + z3;

		m_model[i].x = x1;
		m_model[i].y = y1;
		m_model[i].z = z1;


		//tests for bounding box
		if (x1 < minx) minx = x1;
		if (x2 < minx) minx = x2;
		if (x3 < minx) minx = x3;
		if (x1 > maxx) maxx = x1;
		if (x2 > maxx) maxx = x2;
		if (x3 > maxx) maxx = x3;

		if (y1 < miny) miny = y1;
		if (y2 < miny) miny = y2;
		if (y3 < miny) miny = y3;
		if (y1 > maxy) maxy = y1;
		if (y2 > maxy) maxy = y2;
		if (y3 > maxy) maxy = y3;

		if (z1 < minz) minz = z1;
		if (z2 < minz) minz = z2;
		if (z3 < minz) minz = z3;
		if (z1 > maxz) maxz = z1;
		if (z2 > maxz) maxz = z2;
		if (z3 > maxz) maxz = z3;

		//if flipped the y then also need to reorder the faces to keep normals correct
		if (flipy)
		{
			m_model[i + 2].x = x2;
			m_model[i + 2].y = y2;
			m_model[i + 2].z = z2;

			m_model[i + 1].x = x3;
			m_model[i + 1].y = y3;
			m_model[i + 1].z = z3;
		}
		else
		{
			m_model[i + 1].x = x2;
			m_model[i + 1].y = y2;
			m_model[i + 1].z = z2;

			m_model[i + 2].x = x3;
			m_model[i + 2].y = y3;
			m_model[i + 2].z = z3;
		}

		i += 2;

		//calculate vertex normals

		XMVECTOR v1 = XMLoadFloat3(&XMFLOAT3(m_model[i - 2].x, m_model[i - 2].y, m_model[i - 2].z));
		XMVECTOR v2 = XMLoadFloat3(&XMFLOAT3(m_model[i - 1].x, m_model[i - 1].y, m_model[i - 1].z));
		XMVECTOR v3 = XMLoadFloat3(&XMFLOAT3(m_model[i].x, m_model[i].y, m_model[i].z));

		XMVECTOR n1 = XMVector3Cross(XMVectorSubtract(v2, v1), XMVectorSubtract(v3, v1));
		XMVECTOR n2 = XMVector3Cross(XMVectorSubtract(v3, v2), XMVectorSubtract(v1, v2));
		XMVECTOR n3 = XMVector3Cross(XMVectorSubtract(v1, v3), XMVectorSubtract(v2, v3));

		XMVector3Normalize(n1);
		XMVector3Normalize(n2);
		XMVector3Normalize(n3);

		XMFLOAT3 norm1, norm2, norm3;
		XMStoreFloat3(&norm1, n1);
		XMStoreFloat3(&norm2, n2);
		XMStoreFloat3(&norm3, n3);

		m_model[i - 2].nx = norm1.x;
		m_model[i - 2].ny = norm1.y;
		m_model[i - 2].nz = norm1.z;

		m_model[i - 1].nx = norm2.x;
		m_model[i - 1].ny = norm2.y;
		m_model[i - 1].nz = norm2.z;

		m_model[i].nx = norm3.x;
		m_model[i].ny = norm3.y;
		m_model[i].nz = norm3.z;

		//make up some texture coordinates
		m_model[i - 2].tu = 0;
		m_model[i - 2].tv = 0;
		m_model[i - 1].tu = 1;
		m_model[i - 1].tv = 0;
		m_model[i].tu = 1;
		m_model[i].tv = 1;


		fin.get(input); fin.get(input);	//2 byte spacer before next vertex
	}

	//find average
	av.x /= m_vertexCount;
	av.y /= m_vertexCount;
	av.z /= m_vertexCount;

	//save bounding box
	width = maxx - minx;
	height = maxy - miny;
	depth = maxz - minz;

	//apply translation
	for (int i = 0; i < m_vertexCount; i++)
	{
//		m_model[i].x -= av.x;
//		m_model[i].y -= av.y;
//		m_model[i].z -= av.z;

	}

	fin.close();

	return true;
}

void ModelClass::ReleaseModel()
{
	if (m_model)
	{
		delete[] m_model;
		m_model = 0;
	}

	return;
}
