#pragma once
////////////////////////////////////////////////////////////////////////////////
// Filename: textclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _TEXTCLASS_H_
#define _TEXTCLASS_H_

///////////////////////
// MY CLASS INCLUDES //
///////////////////////
#include "fontclass.h"
//#include "fontshaderclass.h"
#include "Colourshaderclass.h"
#include "GeometryClass.h"	//for colour names

////////////////////////////////////////////////////////////////////////////////
// Class name: TextClass
////////////////////////////////////////////////////////////////////////////////
class TextClass
{

public:

	//SentenceType is the structure that holds the rendering information for each text sentence.
	struct SentenceType
	{
		ID3D11Buffer *vertexBuffer, *indexBuffer;
		int vertexCount, indexCount, maxLength;
		float red, green, blue;
	};

private:
	//The VertexType must match the one in the FontClass.
	struct VertexType
	{
		XMFLOAT3 position;
		XMFLOAT2 texture;
	};

public:
	TextClass();
	TextClass(const TextClass&);
	~TextClass();

	bool Initialize(ID3D11Device * device, ID3D11DeviceContext * deviceContext, ColorShaderClass * shader, HWND hwnd, int screenWidth, int screenHeight, XMMATRIX baseViewMatrix);
	void Shutdown();
	bool Render(ID3D11DeviceContext*, XMMATRIX, XMMATRIX);
	
	SentenceType* getSentence(int i);
	bool UpdateSentence(SentenceType * sentence, char * text, int positionX, int positionY, int width, int height, float red, float green, float blue, ID3D11DeviceContext * deviceContext);

private:
	bool InitializeSentence(SentenceType**, int, ID3D11Device*);



	void ReleaseSentence(SentenceType**);
	bool RenderSentence(ID3D11DeviceContext*, SentenceType*, XMMATRIX, XMMATRIX);

	bool SetBuffers(ID3D11DeviceContext * deviceContext, SentenceType * sentence);

private:
	FontClass* m_Font;
	ColorShaderClass* m_FontShader;
	int m_screenWidth, m_screenHeight;
	XMMATRIX m_baseViewMatrix;
	//We will use two sentences in this tutorial.

	SentenceType* m_sentence1;
	SentenceType* m_sentence2;
};

#endif
