#pragma once
////////////////////////////////////////////////////////////////////////////////
// Filename: modelclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _MODELCLASS_H_
#define _MODELCLASS_H_


//////////////
// INCLUDES //
//////////////
#include <limits>	//for min / max / infinity

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

#include "textureclass.h"

#include <vector>

#include <fstream>
using namespace std;


#define MODEL_FEMUR 1
#define MODEL_WIRE 2
#define MODEL_SPHERE 3
#define MODEL_LINE 4
#define MODEL_RECT 5
#define MODEL_SCREW 6
#define MODEL_CANNULATEDSCREW 7

enum MODEL_TYPE {EMPT, FEMUR,WIRE,SPHERE,LINE,RECTANGLE,SCREW};

struct VertexType
{
	XMFLOAT3 position;
	XMFLOAT2 texture;
	XMFLOAT3 normal;
};

////////////////////////////////////////////////////////////////////////////////
// Class name: ModelClass
////////////////////////////////////////////////////////////////////////////////
class ModelClass
{
public:	


	struct ModelVertexType
	{
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
	};

	float width;
	float height;
	float depth;

public:
	ModelClass();
	ModelClass(const ModelClass&);
	~ModelClass();

	bool Initialize(ID3D11Device * device, ID3D11DeviceContext * deviceContext, int model_type, char * filename);

	void Shutdown();
	void Render(ID3D11DeviceContext*);

	int GetIndexCount();
	ID3D11ShaderResourceView* GetTexture();

private:
	bool InitializeBuffers(ID3D11Device*);
	void ShutdownBuffers();
	void RenderBuffers(ID3D11DeviceContext*);


	bool LoadTexture(ID3D11Device*, ID3D11DeviceContext*, char*);
	void ReleaseTexture();

	bool LoadFemur(char * filename, bool flip_y);
	bool LoadMarker();
	bool LoadWire();
	bool LoadScrew();
	bool LoadSmallScrew();
	bool LoadSphere();
	bool LoadModel(char*, bool);
	bool LoadStl(char*, bool);
	void ReleaseModel();

private:
	ID3D11Buffer *m_vertexBuffer, *m_indexBuffer;
	int m_vertexCount, m_indexCount;
	TextureClass* m_Texture;

	ModelVertexType* m_model;
	XMFLOAT4 m_colour;
};

#endif
