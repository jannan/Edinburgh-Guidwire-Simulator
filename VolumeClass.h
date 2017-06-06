#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>

#include "textureclass.h"
#include "ScenarioFileClass.h"
#include "d3dclass.h"


using namespace std;
using namespace DirectX;


struct CBEveryFrameVS
{
	XMMATRIX mWVP;
};

struct CBEveryFramePS
{
	//float invSize[2];
	//float topLeft[2];
	float exp;			//set the exposure
	float dummy[3];
};


struct CBImmutable
{
	float fInvWindowSize[2];
	float dummy[2];		// byte width must be a multiple of 16
};

class VolumeClass
{
public:
	VolumeClass();
	~VolumeClass();
	bool Initialise(ID3D11Device * device, ID3D11DeviceContext * devicecontext, UINT width, UINT height, ScenarioFileClass * sc, D3DClass* d3dclass);
	void Exposure_Increase(bool AP);
	void Exposure_Decrease(bool AP);
	void setSize(float width, float height);
	void setRotation(float rotation, bool AP, float z_rot);
	XMMATRIX getProjectionMatrix();
	XMMATRIX getViewMatrix();
	XMFLOAT3 getWorld2CT();
	bool load_raw(char * vtk, char * raw);
	bool load_vtk(char * vtk, char * raw);
	bool Render(ID3D11DeviceContext * deviceContext, XMFLOAT2 topLeft, XMFLOAT2 bottomRight, bool AP);
	ID3D11ShaderResourceView * GetTexture(int i);
	ID3D11Resource * GetResource(int i);
	ID3D11ShaderResourceView * GetTextureFront();
	ID3D11ShaderResourceView * GetTextureBack();

private:
	struct VertexType
	{
		XMFLOAT3 position;
		XMFLOAT3 texture;
	};

	D3DClass* d3d;
	ScenarioFileClass*	scenario;

	HWND								g_hWnd = NULL;
	IDXGISwapChain*						g_pSwapChain = NULL;
	ID3D11RenderTargetView*				g_pRenderTargetView = NULL;
	UINT								g_iWindowWidth = 0;
	UINT								g_iWindowHeight = 0;
	ID3D11Device*						g_pd3dDevice = NULL;
	ID3D11DeviceContext*				g_pImmediateContext = NULL;

	// Render front- and back-face positions to texture
	ID3D11VertexShader*					g_pVSModelPosition = NULL;
	ID3D11InputLayout*					g_pVLModelPosition = NULL;
	ID3D11PixelShader*					g_pPSModelPosition = NULL;
	// Main ray-casting algorithm
	ID3D11VertexShader*					g_pVSRayCast = NULL;
	ID3D11InputLayout*					g_pVLRayCast = NULL;
	ID3D11PixelShader*					g_pPSRayCast = NULL;

	// Show position textures
	ID3D11PixelShader*					g_pPSShowFrontPos = NULL;
	ID3D11PixelShader*					g_pPSShowBackPos = NULL;
	ID3D11PixelShader*					g_pPSShowDirection = NULL;

	// Position textures: front and back side
	ID3D11Texture2D*					g_pTexPosition[2] = { NULL, NULL };
	ID3D11ShaderResourceView*			g_pTexPositionRV[2] = { NULL, NULL };
	ID3D11RenderTargetView*				g_pTexPositionRTV[2] = { NULL, NULL };

	//output position textures - render II image to these textures (one for AP, one for lateral)
	ID3D11Texture2D*					g_pTexOut[2] = { NULL, NULL };
	ID3D11ShaderResourceView*			g_pTexOutRV[2] = { NULL, NULL };
	ID3D11RenderTargetView*				g_pTexOutRTV[2] = { NULL, NULL };
	ID3D11Resource*						g_pTexOutRes[2] = { NULL, NULL };


	//II mask texture
	TextureClass*						g_pTexIIMask = NULL;

	// Volume texture
	ID3D11Texture3D*					g_pTexVolume = NULL;
	ID3D11ShaderResourceView*			g_pTexVolumeRV = NULL;

	// Samplers
	ID3D11SamplerState*					g_pSamplerLinear = NULL;

	// Rasterizer states
	ID3D11RasterizerState*				g_pRasterizerStateCullBack = NULL;
	ID3D11RasterizerState*				g_pRasterizerStateCullFront = NULL;

	// Vertex and index buffers
	ID3D11Buffer*						g_pVertexBufferCube = NULL;
	ID3D11Buffer*						g_pIndexBufferCube = NULL;

	// Constant buffers
	ID3D11Buffer*						g_pCBEveryFrameVS = NULL;
	ID3D11Buffer*						g_pCBEveryFramePS = NULL;
	ID3D11Buffer*						g_pCBImmutable = NULL;

	XMMATRIX							g_mViewProjection;

	const UINT							g_iVolumeSize = 256;	// voxel volume width, height and depth


	float								g_ct_width;
	float								g_ct_height;
	float								g_ct_depth;
	float								g_ct_vox_x;
	float								g_ct_vox_y;
	float								g_ct_vox_z;
	float								g_ct_org_x;
	float								g_ct_org_y;
	float								g_ct_org_z;

	XMVECTOR							g_ct_offset;	//hold offset point for ct scan to match model

	XMMATRIX							g_m_view;
	XMMATRIX							g_m_project;

	float								g_II_AP_exposure = 0.04f;
	float								g_II_lat_exposure = 0.02f;

	XMFLOAT3							g_II_focus;

};

