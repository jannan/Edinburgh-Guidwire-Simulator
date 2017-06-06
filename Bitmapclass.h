#pragma once
////////////////////////////////////////////////////////////////////////////////
// Filename: bitmapclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _BITMAPCLASS_H_
#define _BITMAPCLASS_H_

//////////////
// INCLUDES //
//////////////
#include <d3d11.h>
//#include <d3dx11math.h>
#include <directxmath.h>
using namespace DirectX;

///////////////////////
// MY CLASS INCLUDES //
///////////////////////
#include "textureclass.h"


////////////////////////////////////////////////////////////////////////////////
// Class name: BitmapClass
////////////////////////////////////////////////////////////////////////////////
class BitmapClass
{
private:
	struct VertexType
	{
		XMFLOAT3 position;
		XMFLOAT2 texture;
	};

public:
	BitmapClass();
	BitmapClass(const BitmapClass&);
	~BitmapClass();


	bool Initialize(ID3D11Device * device, ID3D11DeviceContext * deviceContext, int screenWidth, int screenHeight, char * textureFilename, int bitmapWidth, int bitmapHeight);
	void Shutdown();
	void Resize(int bitmapWidth, int bitmapHeight);
	bool Render(ID3D11DeviceContext * deviceContext, int positionX, int positionY, int width, int height);

	int GetIndexCount();
	ID3D11ShaderResourceView* GetTexture();

private:
	bool InitializeBuffers(ID3D11Device*);
	void ShutdownBuffers();
	bool UpdateBuffers(ID3D11DeviceContext * deviceContext, int positionX, int positionY, int width, int height);
	void RenderBuffers(ID3D11DeviceContext*);
	bool LoadTexture(ID3D11Device * device, ID3D11DeviceContext * deviceContext, char * filename);
	void ReleaseTexture();

private:
	ID3D11Buffer *m_vertexBuffer, *m_indexBuffer;
	int m_vertexCount, m_indexCount;
	TextureClass* m_Texture;
	int m_screenWidth, m_screenHeight;
	int m_bitmapWidth, m_bitmapHeight;
	int m_previousPosX, m_previousPosY;
};

#endif