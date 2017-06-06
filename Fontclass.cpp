///////////////////////////////////////////////////////////////////////////////
// Filename: fontclass.cpp
///////////////////////////////////////////////////////////////////////////////
#include "fontclass.h"

FontClass::FontClass()
{
	m_Font = 0;
	m_Texture = 0;
}


FontClass::FontClass(const FontClass& other)
{
}


FontClass::~FontClass()
{
}

bool FontClass::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* fontFilename, char* textureFilename)
{
	bool result;


	// Load in the text file containing the font data.
	result = LoadFontData(fontFilename);
	if (!result)
	{
		return false;
	}

	// Load the texture that has the font characters on it.
	result = LoadTexture(device, deviceContext, textureFilename);
	if (!result)
	{
		return false;
	}

	return true;
}

void FontClass::Shutdown()
{
	// Release the font texture.
	ReleaseTexture();

	// Release the font data.
	ReleaseFontData();

	return;
}

bool FontClass::LoadFontData(char* filename)
{
	ifstream fin;
	int i;
	char temp;
	//First we create an array of the FontType structure.The size of the array is set to 95 as that is the number of characters in the texture and hence the number of indexes in the fontdata.txt file.

	// Create the font spacing buffer.
	m_Font = new FontType[95];
	if (!m_Font)
	{
		return false;
	}

	//Now we open the file and read each line into the array m_Font.We only need to read in the texture TU left and right coordinates as well as the pixel size of the character.

	// Read in the font size and spacing between chars.
	fin.open(filename);
	if (fin.fail())
	{
		return false;
	}



	int a = 0;
	while (!fin.eof())
	{
		char tmp_line[256];
		fin.getline(tmp_line, 256);
		size_t t = string(tmp_line).find("id=", 0);
		if ((string(tmp_line).find("char", 0) == 0) && (t > 0) && (t < 256))
		{
			//found a line so split into bits using space

			stringstream ss;
			ss.str(tmp_line);
			string item;

			vector<string> elems;
			while (getline(ss, item, ' ')) {
				if (item.length() > 0)	elems.push_back(item);
			}

			int ascii = 0;
			int xpos = 0, ypos = 0;
			int height = 0, width = 0;
			
			if (elems.size() == 11)
			{
				//probably have a valid line so use this
				//get pos of equal sign
				
				//get ascii code
				size_t pos = elems[1].find('=', 0);
				if ((pos > 0) && (pos < 20)) ascii = stoi(elems[1].substr(pos + 1));

				//get x
				pos = elems[2].find('=', 0);
				if ((pos > 0) && (pos < 20)) xpos = stoi(elems[2].substr(pos + 1));

				//get y
				pos = elems[3].find('=', 0);
				if ((pos > 0) && (pos < 20)) ypos = stoi(elems[3].substr(pos + 1));

				//get width
				pos = elems[4].find('=', 0);
				if ((pos > 0) && (pos < 20)) width = stoi(elems[4].substr(pos + 1));

				//get height
				pos = elems[5].find('=', 0);
				if ((pos > 0) && (pos < 20)) height = stoi(elems[5].substr(pos + 1));

				ascii -= 32;
				if ((ascii > -1) && (ascii < 96))
				{
					m_Font[ascii].left = (float)xpos / 256.f;
					m_Font[ascii].right = m_Font[ascii].left + (float)width / 256.f;
					m_Font[ascii].top = (float)ypos / 256.f;
					m_Font[ascii].bottom = m_Font[ascii].top + (float)height / 256.f;
					m_Font[ascii].size = width;
					m_Font[ascii].height = height;
				}
			}
			//clear vector
			elems.clear();
			ss.clear();

			a++;
		}
	}

	// Close the file.
	//fin.close();

	return true;
}

void FontClass::ReleaseFontData()
{
	// Release the font data array.
	if (m_Font)
	{
		delete[] m_Font;
		m_Font = 0;
	}

	return;
}

bool FontClass::LoadTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* fname)
{
	bool result;


	// Create the texture object.
	m_Texture = new TextureClass;
	if (!m_Texture)
	{
		return false;
	}

	// Initialize the texture object.
	result = m_Texture->Initialize(device, deviceContext, fname);
	if (!result)
	{
		return false;
	}

	return true;
}

void FontClass::ReleaseTexture()
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

ID3D11ShaderResourceView* FontClass::GetTexture()
{
	return m_Texture->GetTexture();
}

void FontClass::BuildVertexArray(void* vertices, char* sentence, float drawX, float drawY)
{
	VertexType* vertexPtr;
	int numLetters, index, i, letter;


	// Coerce the input vertices into a VertexType structure.
	vertexPtr = (VertexType*)vertices;

	// Get the number of letters in the sentence.
	numLetters = (int)strlen(sentence);

	// Initialize the index to the vertex array.
	index = 0;
	//The following loop will now build the vertex and index arrays.It takes each character from the sentence and creates two triangles for it.It then maps the character from the font texture onto those two triangles using the m_Font array which has the TU texture coordinates and pixel size.Once the polygon for that character has been created it then updates the X coordinate on the screen of where to draw the next character.

	// Draw each letter onto a quad.
		for (i = 0; i<numLetters; i++)
		{
			letter = ((int)sentence[i]) - 32;

			// If the letter is a space then just move over three pixels.
			if (letter == 0)
			{
				drawX = drawX + 3.0f;
			}
			else
			{
				// First triangle in quad.
				vertexPtr[index].position = XMFLOAT3(drawX, drawY, 0.0f);  // Top left.
				vertexPtr[index].texture = XMFLOAT2(m_Font[letter].left, m_Font[letter].top);
				index++;

				vertexPtr[index].position = XMFLOAT3((drawX + m_Font[letter].size), (drawY - m_Font[letter].height), 0.0f);  // Bottom right.
				vertexPtr[index].texture = XMFLOAT2(m_Font[letter].right, m_Font[letter].bottom);
				index++;

				vertexPtr[index].position = XMFLOAT3(drawX, (drawY - m_Font[letter].height), 0.0f);  // Bottom left.
				vertexPtr[index].texture = XMFLOAT2(m_Font[letter].left, m_Font[letter].bottom);
				index++;

				// Second triangle in quad.
				vertexPtr[index].position = XMFLOAT3(drawX, drawY, 0.0f);  // Top left.
				vertexPtr[index].texture = XMFLOAT2(m_Font[letter].left, m_Font[letter].top);
				index++;

				vertexPtr[index].position = XMFLOAT3(drawX + m_Font[letter].size, drawY, 0.0f);  // Top right.
				vertexPtr[index].texture = XMFLOAT2(m_Font[letter].right, m_Font[letter].top);
				index++;

				vertexPtr[index].position = XMFLOAT3((drawX + m_Font[letter].size), (drawY - m_Font[letter].height), 0.0f);  // Bottom right.
				vertexPtr[index].texture = XMFLOAT2(m_Font[letter].right, m_Font[letter].bottom);
				index++;

				// Update the x location for drawing by the size of the letter and one pixel.
				drawX = drawX + m_Font[letter].size + 1.0f;
			}
		}

	return;
}