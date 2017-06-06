#pragma once

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

#include <vector>
using namespace std;

#include "modelclass.h"
#include "colourshaderclass.h"
#include "lightclass.h"

#define COLOUR_RED {1.f, 0.f, 0.f, 1.f}
#define COLOUR_GREEN {0.f, 1.f, 0.f, 1.f}
#define COLOUR_BLUE {0.f, 0.f, 1.f, 0.5f}
#define COLOUR_CYAN {0.f, 1.f, 1.f, 1.f}
#define COLOUR_YELLOW {1.f, 1.f, 0.f, 1.f}
#define COLOUR_WHITE {1.f, 1.f, 1.f, 1.f}
#define COLOUR_PURPLE {1.f, 0.f, 1.f, 1.f}
#define COLOUR_ORANGE {1.f, .6f, 0.f, 1.f}
#define COLOUR_BONE {1.f, .94f, 0.81f, 1.f}
#define COLOUR_BONE_TRANS {1.f, .94f, 0.81f, 0.05f}

struct primitiveType
{
	XMMATRIX position;
	bool visible;
	float alpha;
	XMFLOAT4 colour;
	//colour
};


struct lineType
{
	XMFLOAT3 p1;
	XMFLOAT3 p2;
	float length;
	XMFLOAT4 colour;
	bool visible;
};

struct rectType
{
	XMFLOAT3 centre;
	XMFLOAT3 dir;
	XMFLOAT3 up;
	float width;
	float height;
	XMFLOAT4 colour;
};

class GeometryClass
{
public:
	GeometryClass();
	virtual ~GeometryClass();
	void Initialise(ID3D11Device * device, ID3D11DeviceContext * deviceContext);
	void Render(ID3D11DeviceContext * deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ColorShaderClass * shader, LightClass * light);

	void AddSphere(XMFLOAT3 pos, float scale, XMFLOAT4 c);
	void AddLine(XMFLOAT3 pnt1, XMFLOAT3 pnt2, XMFLOAT4 c);

	void ClearAll();


private:
	int numSpheres;
	int numLines;
	int numPlanes;
	vector<primitiveType> spheres;
	vector<lineType> lines;

	ID3D11Buffer *m_vertexBufferSphere, *m_indexBufferSphere;	//model data for the sphere model

	ID3D11Buffer *m_vertexBufferLine, *m_indexBufferLine;

	ModelClass* model_Sphere;
	ModelClass* model_Line;

};

