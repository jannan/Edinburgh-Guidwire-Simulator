#pragma once
////////////////////////////////////////////////////////////////////////////////
// Filename: colorshaderclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _COLORSHADERCLASS_H_
#define _COLORSHADERCLASS_H_


//////////////
// INCLUDES //
//////////////
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <fstream>
using namespace DirectX;
using namespace std;


////////////////////////////////////////////////////////////////////////////////
// Class name: ColorShaderClass
////////////////////////////////////////////////////////////////////////////////
class ColorShaderClass
{
private:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

	struct TADBufferType
	{
		XMMATRIX II_lat;
		XMMATRIX II_ap;
		XMFLOAT4 apex;
//		float padding;	//keep multiple of 4
	};

	struct LightBufferType
	{
		XMFLOAT4 diffuseColor;	//4x4 = 16
		XMFLOAT3 lightDirection;  //3x4 = 12
		float padding;	//keep to muliple of 16 -> 12+4 = 16
	};


	struct MaterialBufferType
	{
		XMFLOAT4 colour;
		//may also include specular / highlight for phong rendering
	};

public:
	ColorShaderClass();
	ColorShaderClass(const ColorShaderClass&);
	~ColorShaderClass();

	bool Initialize(ID3D11Device*, HWND);
	void Shutdown();
	bool Render(ID3D11DeviceContext * deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView * texture, XMFLOAT3 lightDirection, XMFLOAT4 diffuseColor, XMFLOAT4 color);
	bool RenderTexture(ID3D11DeviceContext * deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView * texture, XMFLOAT3 lightDirection, XMFLOAT4 diffuseColor, XMFLOAT4 color);
	bool RenderSolid(ID3D11DeviceContext * deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView * texture, XMFLOAT3 lightDirection, XMFLOAT4 diffuseColor);
	bool RenderTAD(ID3D11DeviceContext * deviceContext, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, XMFLOAT3 apex, XMMATRIX II_ap_mat, XMMATRIX II_lat_mat);

private:
	bool InitializeShader(ID3D11Device*, HWND, WCHAR*, WCHAR*);
	void ShutdownShader();
	void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);

	bool SetShaderParameters(ID3D11DeviceContext * deviceContext, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix, ID3D11ShaderResourceView * texture, XMFLOAT3 lightDirection, XMFLOAT4 diffuseColor, XMFLOAT4 colour);
	void RenderShader(ID3D11DeviceContext*, int);
	void RenderTextureShader(ID3D11DeviceContext * deviceContext, int indexCount);
	void RenderShaderSolid(ID3D11DeviceContext * deviceContext, int indexCount);

	void RenderTADColour(ID3D11DeviceContext * deviceContect, int indexCount);

private:
	ID3D11VertexShader* m_vertexShader;
	ID3D11VertexShader* m_vertexTADShader;

	ID3D11PixelShader* m_pixelShader;
	ID3D11PixelShader* m_pixelTextureShader;
	ID3D11PixelShader* m_pixelSolidShader;
	ID3D11PixelShader* m_pixelTADShader;
	
	ID3D11InputLayout* m_layout;
	ID3D11SamplerState* m_sampleState;		//think this is for textures
	ID3D11Buffer* m_matrixBuffer;		//holds world, view and projection matrices
	ID3D11Buffer* m_lightBuffer;		//holds the light information
	ID3D11Buffer* m_materialBuffer;		//holds the material information (colour only at the moment)
	ID3D11Buffer* m_TADBuffer;			//TAD buffer to hold 2 projection matrices

};

#endif