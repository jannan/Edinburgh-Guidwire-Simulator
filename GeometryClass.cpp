#include "GeometryClass.h"



GeometryClass::GeometryClass()
{
}


GeometryClass::~GeometryClass()
{
}



//init the geometry
void GeometryClass::Initialise(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	/* 
		We need:
			Sphere for a point marker
			Lines
			A plane marker
			Arrows etc

		Method to load primitives (from file or compose)
		A vertex and index buffer
		Start and stop vertices for each type of object
		A shader to draw primitives - one for solid, one for wireframe
		Functions to keep track of primitives


		For now in init - load sphere primative into a buffer
	
	*/

	//load a sphere model into m_vertexBufferSphere and m_indexBufferSphere
	model_Sphere = new ModelClass;
	model_Sphere->Initialize(device, deviceContext, MODEL_SPHERE,"");

	//load a line model (unit length 1 in z direction - should be scaled appropriately)
	model_Line = new ModelClass;
	model_Line->Initialize(device, deviceContext, MODEL_LINE,"");

}


void GeometryClass::Render(ID3D11DeviceContext* deviceContext, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ColorShaderClass* shader, LightClass* light)
{
	/* 
		this will render
		all of the geometry primitives
	*/

	//1. Setup shaders - simple shader without any texure - just simple lighting and colour

	//2. Render Spheres:
	if (spheres.size() > 0)		//only if there is actually a sphere to render!
	{
		//a) load vertex / index data for the sphere model
		model_Sphere->Render(deviceContext);

		for each (primitiveType sphere in spheres)
		{
			if (sphere.visible)
			{
				//b) update model matrix to move and scale sphere
				//c) render the model
				shader->Render(deviceContext, model_Sphere->GetIndexCount(), sphere.position, viewMatrix, projectionMatrix, model_Sphere->GetTexture(), light->GetDirection(),
					light->GetDiffuseColor(), sphere.colour);
			}
		}
	}

	//3.  Repeat for other primitives
	if (lines.size() > 0)
	{
		model_Line->Render(deviceContext);

		for each (lineType line in lines)
		{
			if (line.visible)
			{
				XMMATRIX worldMatrix = XMMatrixScaling(2.f, 2.f, line.length);
				//need to rotate line around p1 to point at p2
				XMVECTOR eye = XMVectorSet(0.f, 0.f, 0.f, 0.f);
				XMVECTOR focus = XMVectorSet(line.p2.x, line.p2.y, line.p2.z, 0.f);
				XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
				XMMATRIX rot = XMMatrixLookAtLH(eye, focus, up);
				rot = XMMatrixTranspose(rot);	//this is key to making it work!
				worldMatrix *= rot;
				worldMatrix *= XMMatrixTranslation(line.p1.x, line.p1.y, line.p1.z);
				shader->Render(deviceContext, model_Sphere->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, model_Line->GetTexture(), light->GetDirection(),
					light->GetDiffuseColor(), line.colour);
			}//red
		}


	}

	//4.  Unload buffers
}


//add a new sphere to scene
void GeometryClass::AddSphere(XMFLOAT3 pos, float scale, XMFLOAT4 c) 
{
	numSpheres++;
	primitiveType newPrim;
	newPrim.visible = true;
	XMMATRIX trans = XMMatrixScaling(scale, scale, scale);
	newPrim.position = trans;
	newPrim.position *= XMMatrixTranslation(pos.x, pos.y, pos.z);
	newPrim.colour = c;

	spheres.push_back(newPrim);
}

void GeometryClass::AddLine(XMFLOAT3 pnt1, XMFLOAT3 pnt2, XMFLOAT4 c)
{
	numLines++;
	lineType newline;
	newline.visible = true;
	newline.p1 = pnt1;
	newline.p2 = XMFLOAT3(pnt2.x - pnt1.x, pnt2.y - pnt1.y, pnt2.z - pnt1.z);
	newline.colour = c;
	newline.length = sqrt((newline.p2.x * newline.p2.x) + (newline.p2.y * newline.p2.y) + (newline.p2.z * newline.p2.z));

	lines.push_back(newline);
	//delete(&newline);
}

void GeometryClass::ClearAll()
{
	spheres.clear();
	lines.clear();
	numLines = 0;
	numSpheres = 0;
}