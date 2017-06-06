////////////////////////////////////////////////////////////////////////////////
// Filename: IIViewClass.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>

#include "textureclass.h"
#include "VolumeClass.h"


using namespace std;
using namespace DirectX;



class IIViewClass
{

public:
	IIViewClass();
	~IIViewClass();
	bool Initialise();
	void Shutdown();

	void RenderAP();
	void RenderLat();

private:
	VolumeClass* m_volume = NULL;
	TextureClass* m_text[1];			//this is to store the textures used to build the II image

};