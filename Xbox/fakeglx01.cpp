/*
Copyright (C) 2000 Jack Palevich.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// FakeGLx01 - Uses Direct3D to implement a subset of OpenGL.

#ifdef _USEFAKEGL01

#include "fakeglx01.h"
#include "xgraphics.h"
#include <vector>

// TODO Fix these warnings instead of disableing them
#pragma warning( disable : 4273 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4820 )

// Utilities
#define	RELEASENULL(object) if (object) { object->Release(); }
// fd: Macros are evil!
//#define BYTE_CLAMP(i) (int) ((((i) > 255) ? 255 : (((i) < 0) ? 0 : (i))))
__inline byte BYTE_CLAMP(int n)
{
	if (n&(~0xFF))
		return (-n)>>31;

	return n;
}

//#define _PROFILE
#ifdef _PROFILE
#pragma pack(push, 8)	// Make sure structure packing is set properly
#include <d3d8perf.h>
#pragma pack(pop)
#endif

// Some DX7 helper functions that we're still using with DX8
#ifdef D3DRGBA
#undef D3DRGBA
#endif
#define D3DRGBA                                 D3DCOLOR_COLORVALUE

#define D3DRGB(_r,_g,_b)                        D3DCOLOR_COLORVALUE(_r,_g,_b,1.f)

#define RGBA_MAKE                               D3DCOLOR_RGBA

#define TEXTURE0_SGIS							0x835E
#define TEXTURE1_SGIS							0x835F
#define D3D_TEXTURE_MAXANISOTROPY				0xf70001

struct subImage_s
{
	int iTextureNum;
	LPDIRECT3DSURFACE8 pTexture;
};

void LocalDebugBreak()
{
	//Not needed atm
	//DebugBreak();
}

// Globals
bool g_force16bitTextures = false;
DWORD gWidth = 640;
DWORD gHeight = 480;
int bytes = 4;

//0 = interlaced 480i
//1 = progressive ("HD") 480p, 720p, depends on dashboard settings
int gVideoMode = 0;

class FakeGL;
static FakeGL* gFakeGL;

#define TEXARRAYSIZE 2000

class TextureEntry
{
public:
	TextureEntry()
	{
		m_id = -1;
		m_mipMap = NULL;
		m_format = D3DFMT_UNKNOWN;
		m_internalFormat = 0;

		m_glTexParameter2DMinFilter = GL_LINEAR_MIPMAP_LINEAR;	//set up trilinear filtering
		m_glTexParameter2DMagFilter = GL_LINEAR;
		m_glTexParameter2DWrapS = GL_CLAMP;						//FakeGL 2009 sets it to WRAP -> CLAMP
		m_glTexParameter2DWrapT = GL_CLAMP;
		m_maxAnisotropy = 4.0;									//we also can bump up the anisotropy level to make things look nicer
	}
	~TextureEntry()
	{
	}

	void Release()
	{
		RELEASENULL(m_mipMap);
	}

	GLuint m_id;
	IDirect3DTexture8* m_mipMap;
	D3DFORMAT m_format;
	GLint m_internalFormat;

	GLint m_glTexParameter2DMinFilter;
	GLint m_glTexParameter2DMagFilter;
	GLint m_glTexParameter2DWrapS;
	GLint m_glTexParameter2DWrapT;
	float m_maxAnisotropy;
};

class TextureEntryLinked : public TextureEntry
{
public:
	TextureEntryLinked::TextureEntryLinked(){ m_pNext = NULL; };
	TextureEntryLinked::~TextureEntryLinked(){};

	TextureEntryLinked *m_pNext;
};

class TextureTable 
{
public:
	TextureTable()
	{
		m_count = 0;
		m_size = 0;
		m_texturesOverflow = NULL;
		m_currentTexture = 0;
		m_currentID = 0;
		BindTexture(0);
	
		m_bSubImagesCached = true;
		m_bSubImage = false;
	}
	~TextureTable()
	{
		// All textures released from call in wglDeleteContext
	}

	void BindTexture(GLuint id)
	{
		m_currentID = id;
	
		if(id < TEXARRAYSIZE)
		{
			if(m_textureArray[id].m_id != -1)
			{
				m_currentTexture = &m_textureArray[id];
				return;
			}
		}
		else // If we have overflowed check the slower linked list
		{
			for (TextureEntryLinked* tex = m_texturesOverflow; tex; tex = tex->m_pNext)
			{
				if (tex->m_id == id)
				{
					m_currentTexture = tex;
					return;
				}
			}
		}

		// If we don't have the texture loaded already

		if(id < TEXARRAYSIZE)
		{
			m_textureArray[id].m_id = id;
			m_textureArray[id].m_mipMap = NULL;

			m_currentTexture = &m_textureArray[id];
		}
		else // If we have overflowed add to the slower linked list
		{
			// Find a free texture
			for (TextureEntryLinked* tex = m_texturesOverflow; tex; tex = tex->m_pNext)
			{
				// Checked if flagged for reuse
				if (tex->m_id == -1)
				{
					tex->m_id = id;
					m_currentTexture = tex;
					return;
				}
			}
			
			// Nothing for reuse?
			tex = new TextureEntryLinked;

			// Link it in
			tex->m_pNext = m_texturesOverflow;
			m_texturesOverflow = tex;

			tex->m_id = id;
			tex->m_mipMap = NULL;

			m_currentTexture = tex;
		}

		// This should never happen 
		if (!m_currentTexture) LocalDebugBreak(); //glBindTexture: out of textures!!!
	}

	void ReleaseAllTextures() // Release when quitting the game
	{
		// Free the main texture array
		for(int i = 0; i < TEXARRAYSIZE; i++)
		{
			if(m_textureArray[i].m_id != -1)
			{
				DeleteSubImageCache(m_textureArray[i].m_id);

				if(m_textureArray[i].m_mipMap)
				{
					m_textureArray[i].m_mipMap->Release();
					m_textureArray[i].m_mipMap = NULL;
					m_textureArray[i].m_id = -1;
				}
			}
		}

		// Now free the overflow linked list
		TextureEntryLinked* tmpNode;

		while(m_texturesOverflow != NULL)
		{
			tmpNode = m_texturesOverflow;
			m_texturesOverflow = m_texturesOverflow->m_pNext;

			if(tmpNode)
			{
				DeleteSubImageCache(tmpNode->m_id);
				tmpNode->m_mipMap = NULL;
				delete tmpNode;
			}
		}
	}

	void DeleteTextures(GLsizei n, const GLuint *textures)
	{
		for (int i = 0; i < n; i++)
		{
			DeleteSubImageCache(textures[i]);

			if(textures[i] < TEXARRAYSIZE)
			{
				if(m_textureArray[textures[i]].m_mipMap)
				{
					m_textureArray[textures[i]].m_mipMap->Release();
					m_textureArray[textures[i]].m_mipMap = NULL;
					m_textureArray[textures[i]].m_id = -1;
				}
			}
			else // It's in the linked list
			{
				for (TextureEntryLinked* tex = m_texturesOverflow; tex; tex = tex->m_pNext)
				{
					if (tex->m_id == textures[i])
					{
						tex->m_id = -1;
						tex->Release();
						tex->m_mipMap = NULL;
						break;
					}
				}
			}
		}
	}

	subImage_s* GetSubImageCache(IDirect3DDevice8* pDevice, int iNum)
	{
#if 0
		D3DFORMAT d3dFormat = D3DFMT_A8R8G8B8;
#endif
		for(int i = 0; i < (int)m_SubImageCache.size(); i++)
		{
			subImage_s* pCurrentTex = m_SubImageCache[i];
		
			if(pCurrentTex->iTextureNum == iNum)
				return pCurrentTex;
		}

		subImage_s* pNewSubImage = new subImage_s;
		pNewSubImage->iTextureNum = iNum;
#if 0		
		if( g_force16bitTextures )
			d3dFormat = D3DFMT_A4R4G4B4;
#endif		
		//MARTY FIXME: Don't hardcode these values (ok for now tho, as this is only used for lightmaps)
		pDevice->CreateImageSurface(128/*BLOCK_SIZE*/, 128/*BLOCK_SIZE*/, D3DFMT_A8R8G8B8, &pNewSubImage->pTexture);

		m_SubImageCache.push_back(pNewSubImage);

		return pNewSubImage;
	}

	void DeleteSubImageCache(int iNum)
	{
		for(int i = 0; i < (int)m_SubImageCache.size(); i++)
		{
			subImage_s* pCurrentTex = m_SubImageCache[i];

			if(pCurrentTex->iTextureNum == iNum)
			{
				pCurrentTex->iTextureNum = 0;
				pCurrentTex->pTexture->Release();
	
				delete pCurrentTex;

				pCurrentTex = NULL;

				m_bSubImagesCached = true;

				m_SubImageCache.erase(m_SubImageCache.begin()+i);
			}
		}
	}

	int GetCurrentID() 
	{
		return m_currentID;
	}

	TextureEntry* GetCurrentEntry() 
	{
		return m_currentTexture;
	}

	TextureEntry* GetEntry(GLuint id)
	{
		if (m_currentID == id && m_currentTexture) 
			return m_currentTexture;
		
		return &m_textureArray[id];
	}

	IDirect3DTexture8* GetMipMap()
	{
		if(m_currentTexture)
			return m_currentTexture->m_mipMap;

		return 0;
	}

	IDirect3DTexture8* GetMipMap(int id)
	{
		TextureEntry* entry = GetEntry(id);

		if(entry) 
			return entry->m_mipMap;

		return 0;
	}

	D3DFORMAT GetSurfaceFormat()
	{
		if(m_currentTexture) 
			return m_currentTexture->m_format;

		return D3DFMT_UNKNOWN;
	}

	void SetTexture(IDirect3DTexture8* mipMap, D3DFORMAT d3dFormat, GLint internalFormat)
	{
		if(!m_currentTexture)
			BindTexture(0);

		RELEASENULL(m_currentTexture->m_mipMap);
		m_currentTexture->m_mipMap = mipMap;
		m_currentTexture->m_format = d3dFormat;
		m_currentTexture->m_internalFormat = internalFormat;
	}

	GLint GetInternalFormat() 
	{
		if(m_currentTexture) 
			return m_currentTexture->m_internalFormat;

		return 0;
	}

	void SetSubTexture(bool bSubTex)
	{
		m_bSubImage = bSubTex;
	}

	bool IsSubTexture()
	{
		return m_bSubImage;
	}

	bool SubTexturesCached()
	{
		return m_bSubImagesCached;
	}	

private:
	GLuint m_currentID;
	DWORD m_count;
	DWORD m_size;

	TextureEntry m_textureArray[TEXARRAYSIZE];
	TextureEntryLinked* m_texturesOverflow;
	TextureEntry* m_currentTexture;

	std::vector<subImage_s*> m_SubImageCache;
	bool m_bSubImagesCached;
	bool m_bSubImage;
};

#if 1
#define Clamp(x) (x) // No clamping -- we've made sure the inputs are in the range 0..1
#else
float Clamp(float x) 
{
	if ( x < 0 ) 
	{
		x = 0;
		LocalDebugBreak();
	}
	else if ( x > 1 ) 
	{
		x = 1;
		LocalDebugBreak();
	}
	return x;
}
#endif

static D3DBLEND GLToDXSBlend(GLenum glBlend)
{
	D3DBLEND result = D3DBLEND_ONE;
	switch ( glBlend ) 
	{
		case GL_ZERO: result = D3DBLEND_ZERO; break;
		case GL_ONE: result = D3DBLEND_ONE; break;
		case GL_DST_COLOR: result = D3DBLEND_DESTCOLOR; break;
		case GL_ONE_MINUS_DST_COLOR: result = D3DBLEND_INVDESTCOLOR; break;
		case GL_SRC_ALPHA: result = D3DBLEND_SRCALPHA; break;
		case GL_ONE_MINUS_SRC_ALPHA: result = D3DBLEND_INVSRCALPHA; break;
		case GL_DST_ALPHA: result = D3DBLEND_DESTALPHA; break;
		case GL_ONE_MINUS_DST_ALPHA: result = D3DBLEND_INVDESTALPHA; break;
		case GL_SRC_ALPHA_SATURATE: result = D3DBLEND_SRCALPHASAT; break;
		default: LocalDebugBreak(); break;
	}
	return result;
}

static D3DBLEND GLToDXDBlend(GLenum glBlend)
{
	D3DBLEND result = D3DBLEND_ONE;
	switch ( glBlend )
	{
		case GL_ZERO: result = D3DBLEND_ZERO; break;
		case GL_ONE: result = D3DBLEND_ONE; break;
		case GL_SRC_COLOR: result = D3DBLEND_SRCCOLOR; break;
		case GL_ONE_MINUS_SRC_COLOR: result = D3DBLEND_INVSRCCOLOR; break;
		case GL_SRC_ALPHA: result = D3DBLEND_SRCALPHA; break;
		case GL_ONE_MINUS_SRC_ALPHA: result = D3DBLEND_INVSRCALPHA; break;
		case GL_DST_ALPHA: result = D3DBLEND_DESTALPHA; break;
		case GL_ONE_MINUS_DST_ALPHA: result = D3DBLEND_INVDESTALPHA; break;
		default: LocalDebugBreak(); break;
	}
	return result;
}

static D3DCMPFUNC GLToDXCompare(GLenum func)
{
	D3DCMPFUNC result = D3DCMP_ALWAYS;
	switch ( func ) 
	{
		case GL_NEVER: result = D3DCMP_NEVER; break;
		case GL_LESS: result = D3DCMP_LESS; break;
		case GL_EQUAL: result = D3DCMP_EQUAL; break;
		case GL_LEQUAL: result = D3DCMP_LESSEQUAL; break;
		case GL_GREATER: result = D3DCMP_GREATER; break;
		case GL_NOTEQUAL: result = D3DCMP_NOTEQUAL; break;
		case GL_GEQUAL: result = D3DCMP_GREATEREQUAL; break;
		case GL_ALWAYS: result = D3DCMP_ALWAYS; break;
		default: break;
	}
	return result;
}

/*
   OpenGL                      MinFilter           MipFilter       Comments
   GL_NEAREST                  D3DTFN_POINT        D3DTFP_NONE
   GL_LINEAR                   D3DTFN_LINEAR       D3DTFP_NONE
   GL_NEAREST_MIPMAP_NEAREST   D3DTFN_POINT        D3DTFP_POINT        
   GL_LINEAR_MIPMAP_NEAREST    D3DTFN_LINEAR       D3DTFP_POINT    bilinear
   GL_NEAREST_MIPMAP_LINEAR    D3DTFN_POINT        D3DTFP_LINEAR 
   GL_LINEAR_MIPMAP_LINEAR     D3DTFN_LINEAR       D3DTFP_LINEAR   trilinear
*/

static D3DTEXTUREFILTERTYPE GLToDXMinFilter(GLint filter)
{
	D3DTEXTUREFILTERTYPE result = D3DTEXF_LINEAR;
	switch ( filter ) 
	{
		case GL_NEAREST: result = D3DTEXF_POINT; break;
		case GL_LINEAR: result = D3DTEXF_LINEAR; break;
		case GL_NEAREST_MIPMAP_NEAREST: result = D3DTEXF_POINT; break;
		case GL_LINEAR_MIPMAP_NEAREST: result = D3DTEXF_LINEAR; break;
		case GL_NEAREST_MIPMAP_LINEAR: result = D3DTEXF_POINT; break;
		case GL_LINEAR_MIPMAP_LINEAR: result = D3DTEXF_LINEAR; break;
	default:
		LocalDebugBreak();
		break;
	}
	return result;
}

static D3DTEXTUREFILTERTYPE GLToDXMipFilter(GLint filter)
{
	D3DTEXTUREFILTERTYPE result = D3DTEXF_LINEAR;
	switch ( filter ) 
	{
		case GL_NEAREST: result = D3DTEXF_NONE; break;
		case GL_LINEAR: result = D3DTEXF_NONE; break;
		case GL_NEAREST_MIPMAP_NEAREST: result = D3DTEXF_POINT; break;
		case GL_LINEAR_MIPMAP_NEAREST: result = D3DTEXF_POINT; break;
		case GL_NEAREST_MIPMAP_LINEAR: result = D3DTEXF_LINEAR; break;
		case GL_LINEAR_MIPMAP_LINEAR: result = D3DTEXF_LINEAR; break;
	default:
		LocalDebugBreak();
		break;
	}
	return result;
}

static D3DTEXTUREFILTERTYPE GLToDXMagFilter(GLint filter)
{
	D3DTEXTUREFILTERTYPE result = D3DTEXF_POINT;
	switch ( filter )
	{
		case GL_NEAREST: result = D3DTEXF_POINT; break;
		case GL_LINEAR: result = D3DTEXF_LINEAR; break;
	default:
		LocalDebugBreak();
		break;
	}
	return result;
}

static D3DTEXTUREOP GLToDXTextEnvMode(GLint mode)
{
	D3DTEXTUREOP result = D3DTOP_MODULATE;
	switch ( mode ) 
	{
		case GL_MODULATE: result = D3DTOP_MODULATE; break;
		case GL_DECAL: result = D3DTOP_SELECTARG1; break; // Fix this
		case GL_BLEND: result = D3DTOP_BLENDTEXTUREALPHA; break;
		case GL_REPLACE: result = D3DTOP_SELECTARG1; break;
		default: break;
	}
	return result;
}

#define MAXSTATES 8

class TextureStageState 
{
public:
	TextureStageState()
	{
		m_currentTexture = 0;
		m_glTextEnvMode = GL_MODULATE;
		m_glTexture2D = false;
		m_dirty = true;
	}

	bool GetDirty()
	{
		return m_dirty;
	}

	void SetDirty(bool dirty) 
	{ 
		m_dirty = dirty;
	}

	void DirtyTexture(GLuint textureID)
	{
		if ( textureID == m_currentTexture ) 
		{
			m_dirty = true;
		}
	}

	GLuint GetCurrentTexture() { return m_currentTexture; }
	void SetCurrentTexture(GLuint texture) { m_dirty = true; m_currentTexture = texture; }

	GLfloat GetTextEnvMode() { return m_glTextEnvMode; }
	void SetTextEnvMode(GLfloat mode) { m_dirty = true; m_glTextEnvMode = mode; }

	bool GetTexture2D() { return m_glTexture2D; }
	void SetTexture2D(bool texture2D) { m_dirty = true; m_glTexture2D = texture2D; }

private:
	GLuint m_currentTexture;
	GLfloat m_glTextEnvMode;
	bool m_glTexture2D;
	bool m_dirty;
};

class TextureState
{
public:
	TextureState()
	{
		m_currentStage = 0;
		memset(&m_stage, 0, sizeof(m_stage));
		m_dirty = false;
		m_mainBlend = false;
	}

	void SetMaxStages(int maxStages)
	{
		m_maxStages = maxStages;
		for(int i = 0; i < m_maxStages;i++)
		{
			m_stage[i].SetDirty(true);
		}
		m_dirty = true;
	}

	// Keep track of changes to texture stage state
	void SetCurrentStage(int index)
	{
		m_currentStage = index;
	}

	int GetMaxStages() { return m_maxStages; }

	bool GetDirty() { return m_dirty; }

	void DirtyTexture(int textureID)
	{
		for(int i = 0; i < m_maxStages;i++)
		{
			m_stage[i].DirtyTexture(textureID);
		}
		m_dirty = true;
	}

	void SetMainBlend(bool mainBlend)
	{
		m_mainBlend = mainBlend;
		m_stage[0].SetDirty(true);
		m_dirty = true;
	}
	
	// These methods apply to the current stage

	GLuint GetCurrentTexture() { return Get()->GetCurrentTexture(); }

	void SetCurrentTexture(GLuint texture) { m_dirty = true; Get()->SetCurrentTexture(texture); }

	GLfloat GetTextEnvMode() { return Get()->GetTextEnvMode(); }

	void SetTextEnvMode(GLfloat mode) { m_dirty = true; Get()->SetTextEnvMode(mode); }
	
	bool GetTexture2D() { return Get()->GetTexture2D(); }

	void SetTexture2D(bool texture2D) { m_dirty = true; Get()->SetTexture2D(texture2D); }

	void SetTextureStageState(IDirect3DDevice8* pD3DDev, TextureTable* textures)
	{
		if ( ! m_dirty )
		{
			return;
		}
		static bool firstTime = true;
		if ( firstTime ) 
		{
			firstTime = false;
			for(int i = 0; i < m_maxStages; i++ ) 
			{
				pD3DDev->SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, i);
			}
		}

		m_dirty = false;

		for(int i = 0; i < m_maxStages; i++ )
		{
			if ( ! m_stage[i].GetDirty() ) 
			{
				continue;
			}
			m_stage[i].SetDirty(false);

			if ( m_stage[i].GetTexture2D() ) 
			{
				DWORD color1 = D3DTA_TEXTURE;
				int textEnvMode =  m_stage[i].GetTextEnvMode();
				DWORD colorOp = GLToDXTextEnvMode(textEnvMode);
				if ( i > 0 && textEnvMode == GL_BLEND ) 
				{
					// Assume we're doing multi-texture light mapping.
					// I don't think this is the right way to do this
					// but it works for D3DQuake.
					colorOp = D3DTOP_MODULATE;
					color1 |= D3DTA_COMPLEMENT;
				}
				pD3DDev->SetTextureStageState( i, D3DTSS_COLORARG1, color1);
				pD3DDev->SetTextureStageState( i, D3DTSS_COLORARG2, i == 0 ? D3DTA_DIFFUSE :  D3DTA_CURRENT);
				pD3DDev->SetTextureStageState( i, D3DTSS_COLOROP, colorOp);
				DWORD alpha1 = D3DTA_TEXTURE;
				DWORD alpha2 = D3DTA_DIFFUSE;
				DWORD alphaOp;
				alphaOp = GLToDXTextEnvMode(textEnvMode);
				if (i == 0 && m_mainBlend )
				{
					alphaOp = D3DTOP_MODULATE;	// Otherwise the console is never transparent
				}
				pD3DDev->SetTextureStageState( i, D3DTSS_ALPHAARG1, alpha1);
				pD3DDev->SetTextureStageState( i, D3DTSS_ALPHAARG2, alpha2);
				pD3DDev->SetTextureStageState( i, D3DTSS_ALPHAOP,   alphaOp);

				TextureEntry* entry = textures->GetEntry(m_stage[i].GetCurrentTexture());
				if ( entry ) 
				{
					int minFilter = entry->m_glTexParameter2DMinFilter;
					DWORD dxMinFilter = GLToDXMinFilter(minFilter);
					DWORD dxMipFilter = GLToDXMipFilter(minFilter);
					DWORD dxMagFilter = GLToDXMagFilter(entry->m_glTexParameter2DMagFilter);

					// Avoid setting anisotropic if the user doesn't request it.
					static bool bSetMaxAnisotropy = false;
					if ( entry->m_maxAnisotropy != 1.0f ) 
					{
						bSetMaxAnisotropy = true;
						if ( dxMagFilter == D3DTEXF_LINEAR) 
						{
							dxMagFilter = D3DTEXF_ANISOTROPIC;
						}
						if ( dxMinFilter == D3DTEXF_LINEAR) 
						{
							dxMinFilter = D3DTEXF_ANISOTROPIC;
						}
					}
					if ( bSetMaxAnisotropy ) 
					{
						pD3DDev->SetTextureStageState( i, D3DTSS_MAXANISOTROPY, entry->m_maxAnisotropy);
					}
					pD3DDev->SetTextureStageState( i, D3DTSS_MINFILTER, dxMinFilter );
					pD3DDev->SetTextureStageState( i, D3DTSS_MIPFILTER, dxMipFilter );
					pD3DDev->SetTextureStageState( i, D3DTSS_MAGFILTER,  dxMagFilter);
					IDirect3DTexture8* pTexture = entry->m_mipMap;
					// char buf[100];
					// sprintf(buf,"SetTexture 0x%08x\n", pTexture);
					// OutputDebugString(buf);
					if ( pTexture )
						pD3DDev->SetTexture( i, pTexture);
					else
						LocalDebugBreak();
				}
			}
			else 
			{
				pD3DDev->SetTexture( i, NULL);
				pD3DDev->SetTextureStageState( i, D3DTSS_COLORARG1, D3DTA_TEXTURE);
				pD3DDev->SetTextureStageState( i, D3DTSS_COLORARG2, i == 0 ? D3DTA_DIFFUSE :  D3DTA_CURRENT);
				pD3DDev->SetTextureStageState( i, D3DTSS_COLOROP, D3DTOP_DISABLE);
			}
		}
	}

private:
	TextureStageState* Get() 
	{
		return m_stage + m_currentStage;
	}

	bool m_dirty;
	bool m_mainBlend;
	int m_maxStages;
	int m_currentStage;
	TextureStageState m_stage[MAXSTATES];
};

// This class buffers up all the glVertex calls between
// glBegin and glEnd.

// USE_DRAWINDEXEDPRIMITIVE seems slightly faster (54 fps vs 53 fps) than USE_DRAWPRIMITIVE.
// USE_DRAWINDEXEDPRIMITIVEVB is much slower (30fps vs 54fps), at least on GeForce Win9x 3.75.

// DrawPrimitive works for DX8, the other ones don't work right yet.

#define USE_DRAWPRIMITIVE

#ifdef USE_DRAWPRIMITIVE
class OGLPrimitiveVertexBuffer 
{
public:
	OGLPrimitiveVertexBuffer()
	{
		m_drawMode = -1;
		m_size = 0;
		m_count = 0;
		m_OGLPrimitiveVertexBuffer = 0;
		m_vertexCount = 0;
		m_vertexTypeDesc = 0;
		memset(m_textureCoords, 0, sizeof(m_textureCoords));

		m_pD3DDev = 0;
		m_color = 0xff000000; // Don't know if this is correct
	}

	~OGLPrimitiveVertexBuffer()
	{
#ifdef USE_PUSHBUFFER
		RELEASENULL(m_pushBuffer);
#endif
		delete [] m_OGLPrimitiveVertexBuffer;
	}

	HRESULT Initialize(IDirect3DDevice8* pD3DDev, IDirect3D8* pD3D, bool hardwareTandL, DWORD typeDesc)
	{
		m_pD3DDev = pD3DDev;
		if (m_vertexTypeDesc != typeDesc) 
		{
			m_vertexTypeDesc = typeDesc;
			m_vertexSize = 0;
			if ( m_vertexTypeDesc & D3DFVF_XYZ ) 
			{
				m_vertexSize += 3 * sizeof(float);
			}
			if ( m_vertexTypeDesc & D3DFVF_DIFFUSE )
			{
				m_vertexSize += 4;
			}
			int textureStages = (m_vertexTypeDesc & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
			m_vertexSize += 2 * sizeof(float) * textureStages;
		}

#ifdef USE_PUSHBUFFER
		UINT pbSize = 384*1024; //Only used if RunUsingCpuCopy == false, it's overriden by SetPushBufferSize(), so right now it just sits here for reference
		HRESULT hr;
		hr = pD3DDev->CreatePushBuffer(pbSize, true, &m_pushBuffer);
		if ( FAILED(hr) ) 
			return hr;
#endif
		return S_OK;
	}

	DWORD GetVertexTypeDesc()
	{
		return m_vertexTypeDesc;
	}

	LPVOID GetOGLPrimitiveVertexBuffer()
	{
		return m_OGLPrimitiveVertexBuffer;
	}

	DWORD GetVertexCount()
	{
		return m_vertexCount;
	}

	inline void SetColor(D3DCOLOR color)
	{
		m_color = color;
	}
	
	inline void SetTextureCoord0(float u, float v)
	{
		DWORD* pCoords = (DWORD*) m_textureCoords;
		pCoords[0] = *(DWORD*)& u;
		pCoords[1] = *(DWORD*)& v;
	}

	inline void SetTextureCoord(int textStage, float u, float v)
	{
		DWORD* pCoords = (DWORD*) m_textureCoords + (textStage << 1);
		pCoords[0] = *(DWORD*)& u;
		pCoords[1] = *(DWORD*)& v;
	}

	inline void SetVertex(float x, float y, float z)
	{
		int newCount = m_count + m_vertexSize;
		if (newCount > m_size) 
		{
			Ensure(m_vertexSize);
		}
		DWORD* pFloat = (DWORD*) (m_OGLPrimitiveVertexBuffer + m_count);
		pFloat[0] = *(DWORD*)& x;
		pFloat[1] = *(DWORD*)& y;
		pFloat[2] = *(DWORD*)& z;
		const DWORD* pCoords = (DWORD*) m_textureCoords;
		switch(m_vertexTypeDesc)
		{			
		case (D3DFVF_XYZ | D3DFVF_DIFFUSE | (1 << D3DFVF_TEXCOUNT_SHIFT)):
			pFloat[3] = m_color;
			pFloat[4] = pCoords[0];
			pFloat[5] = pCoords[1];
			break;
		case (D3DFVF_XYZ | D3DFVF_DIFFUSE | (2 << D3DFVF_TEXCOUNT_SHIFT)):
			pFloat[3] = m_color;
			pFloat[4] = pCoords[0];
			pFloat[5] = pCoords[1];
			pFloat[6] = pCoords[2];
			pFloat[7] = pCoords[3];
			break;
		default:
			{
				if ( m_vertexTypeDesc & D3DFVF_DIFFUSE ) 
				{
					*pFloat++ = m_color;
				}
				int textureStages = (m_vertexTypeDesc & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
				for ( int i = 0; i < textureStages; i++ )
				{
					*pFloat++ = *pCoords++;
					*pFloat++ = *pCoords++;
				}
			}
			break;
		}
		m_count = newCount;
		m_vertexCount++;

		// TO DO: Flush vertex buffer if larger than 1000 vertexes.
		// Have to do this modulo vertexes-per-primitive
	}

	inline IsMergableMode(GLenum mode)
	{
		return ( mode == m_drawMode ) && ( mode == GL_QUADS || mode == GL_TRIANGLES );
	}

	inline void Begin(GLuint drawMode)
	{
		m_drawMode = drawMode;
	}

	inline void Append(GLuint drawMode)
	{
	}

	inline void End()
	{
		if ( m_vertexCount == 0 ) // Startup
			return;

		D3DPRIMITIVETYPE dptPrimitiveType;
		switch ( m_drawMode ) 
		{
			case GL_POINTS: dptPrimitiveType = D3DPT_POINTLIST; break;
			case GL_LINES: dptPrimitiveType = D3DPT_LINELIST; break;
			case GL_LINE_STRIP: dptPrimitiveType = D3DPT_LINESTRIP; break;
			case GL_LINE_LOOP:
				dptPrimitiveType = D3DPT_LINESTRIP;
				LocalDebugBreak();  // Need to add one more point
				break;
			case GL_TRIANGLES: dptPrimitiveType = D3DPT_TRIANGLELIST; break;
			case GL_TRIANGLE_STRIP: dptPrimitiveType = D3DPT_TRIANGLESTRIP; break;
			case GL_TRIANGLE_FAN: dptPrimitiveType = D3DPT_TRIANGLEFAN; break;
			case GL_QUADS:
				if ( m_vertexCount <= 4 ) 
					dptPrimitiveType = D3DPT_TRIANGLEFAN;
				else 
				{
					dptPrimitiveType = D3DPT_TRIANGLELIST;
					ConvertQuadsToTriangles();
				}
				break;
			case GL_QUAD_STRIP:
				if ( m_vertexCount <= 4 ) 
					dptPrimitiveType = D3DPT_TRIANGLEFAN;
				else 
				{
					dptPrimitiveType = D3DPT_TRIANGLESTRIP;
					ConvertQuadStripToTriangleStrip();
				}
				break;

			case GL_POLYGON:
				dptPrimitiveType = D3DPT_TRIANGLEFAN;
				if ( m_vertexCount < 3) 
					goto exit;
				// How is this different from GL_TRIANGLE_FAN, other than
				// that polygons are planar?
				break;
			default:
				LocalDebugBreak();
				goto exit;
		}
		{
			DWORD primCount;
			switch ( dptPrimitiveType ) 
			{
				default:
				case D3DPT_TRIANGLESTRIP: primCount = m_vertexCount - 2; break;
				case D3DPT_TRIANGLEFAN: primCount = m_vertexCount - 2; break;
				case D3DPT_TRIANGLELIST: primCount = m_vertexCount / 3; break;
			}

#ifdef USE_PUSHBUFFER
			m_pD3DDev->BeginPushBuffer(m_pushBuffer);
#endif
			m_pD3DDev->SetVertexShader(m_vertexTypeDesc);
     		m_pD3DDev->DrawPrimitiveUP(dptPrimitiveType, primCount, m_OGLPrimitiveVertexBuffer, m_vertexSize);

#ifdef USE_PUSHBUFFER
			m_pD3DDev->EndPushBuffer();
			m_pD3DDev->RunPushBuffer(m_pushBuffer, NULL);
#endif
		}
exit:
		m_vertexCount = 0;
		m_count = 0;
	}

private:
	void ConvertQuadsToTriangles()
	{
		int quadCount = m_vertexCount / 4;
		int addedVerticies = 2 * quadCount;
		int addedDataSize = addedVerticies * m_vertexSize;
		Ensure( addedDataSize );

		// A quad is v0, v1, v2, v3
		// The corresponding triangle pair is v0 v1 v2 , v0 v2 v3
		for(int i = quadCount-1; i >= 0; i--)
		{
			int startOfQuad = i * m_vertexSize * 4;
			int startOfTrianglePair = i * m_vertexSize * 6;
			// Copy the last two verticies of the second triangle
			memcpy(m_OGLPrimitiveVertexBuffer + startOfTrianglePair + 4 * m_vertexSize,
				m_OGLPrimitiveVertexBuffer + startOfQuad + m_vertexSize * 2, m_vertexSize * 2);
			// Copy the first vertex of the second triangle
			memcpy(m_OGLPrimitiveVertexBuffer + startOfTrianglePair + 3 * m_vertexSize,
				m_OGLPrimitiveVertexBuffer + startOfQuad, m_vertexSize);
			// Copy the first triangle
			if ( i > 0 ) 
			{
				memcpy(m_OGLPrimitiveVertexBuffer + startOfTrianglePair, m_OGLPrimitiveVertexBuffer + startOfQuad, 3 * m_vertexSize);
			}
		}
		m_count += addedDataSize;
		m_vertexCount += addedVerticies;
	}

	void ConvertQuadStripToTriangleStrip()
	{
		int vertexPairCount = m_vertexCount / 2;

		// Doesn't add any points, but does reorder the vertices.
		// Swap each pair of verticies.

		for(int i = 0; i < vertexPairCount; i++) 
		{
			int startOfPair = i * m_vertexSize * 2;
			int middleOfPair = startOfPair + m_vertexSize;
			for(int j = 0; j < m_vertexSize; j++) 
			{
				int c = m_OGLPrimitiveVertexBuffer[startOfPair + j];
				m_OGLPrimitiveVertexBuffer[startOfPair + j] = m_OGLPrimitiveVertexBuffer[middleOfPair + j];
				m_OGLPrimitiveVertexBuffer[middleOfPair + j] = c;
			}
		}
	}

	void Ensure(int size)
	{
		if (( m_count + size ) > m_size ) 
		{
			int newSize = m_size * 2;
			if ( newSize < m_count + size ) newSize = m_count + size;
			char* newVB = new char[newSize];
			if ( m_OGLPrimitiveVertexBuffer )
			{
				memcpy(newVB, m_OGLPrimitiveVertexBuffer, m_count);
			}
			delete[] m_OGLPrimitiveVertexBuffer;
			m_OGLPrimitiveVertexBuffer = newVB;
			m_size = newSize;
		}

		/*
		char buf[512];
		sprintf(buf, "Current size %d\n", m_size);
		OutputDebugString(buf);
		*/
	}

	GLuint m_drawMode;
	DWORD  m_vertexTypeDesc;
	int m_vertexSize; // in bytes

	IDirect3DDevice8* m_pD3DDev;
	char* m_OGLPrimitiveVertexBuffer;
	int m_size;  // bytes size of buffer
	int m_count; // bytes used
	DWORD m_vertexCount;
	D3DCOLOR m_color;
	float m_textureCoords[MAXSTATES*2];
	IDirect3DPushBuffer8* m_pushBuffer;
};

#endif // USE_DRAWPRIMITIVE

class FakeGL
{
private:
	IDirect3DDevice8* m_pD3DDev;
    D3DSURFACE_DESC m_d3dsdBackBuffer;   // Surface desc of the backbuffer
	LPDIRECT3D8 m_pD3D;
	
	IDirect3DTexture8* m_pPrimary;
	bool m_hardwareTandL;
	
    bool m_bD3DXReady;
	
	bool m_glRenderStateDirty;

	bool m_glAlphaStateDirty;
	GLenum m_glAlphaFunc;
	GLclampf m_glAlphaFuncRef;
	bool m_glAlphaTest;

	bool m_glBlendStateDirty;
	bool m_glBlend;
	GLenum m_glBlendFuncSFactor;
	GLenum m_glBlendFuncDFactor;

	bool m_glCullStateDirty;
	bool m_glCullFace;
	GLenum m_glCullFaceMode;
	GLenum m_glCullFrontFaceMode;
	
	bool m_PolyOffsetEnabled;
	bool m_PolyOffsetSwitched;
	float m_PolyOffsetFactor;

	bool m_bFogEnabled;
	bool m_bFogSwitched;

	bool m_glDepthStateDirty;
	bool m_glDepthTest;
	GLenum m_glDepthFunc;
	bool m_glDepthMask;

	GLclampd m_glDepthRangeNear;
	GLclampd m_glDepthRangeFar;

	GLenum m_glMatrixMode;

	GLenum m_glPolygonModeFront;
	GLenum m_glPolygonModeBack;

	bool m_glShadeModelStateDirty;
	GLenum m_glShadeModel;

	bool m_bViewPortDirty;
	GLint m_glViewPortX;
	GLint m_glViewPortY;
	GLsizei m_glViewPortWidth;
	GLsizei m_glViewPortHeight;

	TextureState m_textureState;
	TextureTable m_textures;

	bool m_modelViewMatrixStateDirty;
	bool m_projectionMatrixStateDirty;
	bool m_textureMatrixStateDirty;
	bool* m_currentMatrixStateDirty; // an alias to one of the preceeding stacks

	ID3DXMatrixStack* m_modelViewMatrixStack;
	ID3DXMatrixStack* m_projectionMatrixStack;
	ID3DXMatrixStack* m_textureMatrixStack;
	ID3DXMatrixStack* m_currentMatrixStack; // an alias to one of the preceeding stacks

	bool m_viewMatrixStateDirty;
	D3DXMATRIX m_d3dViewMatrix;

	OGLPrimitiveVertexBuffer m_OGLPrimitiveVertexBuffer;

	bool m_needBeginScene;

	const char* m_vendor;
	const char* m_renderer;
	char m_version[64];
	const char* m_extensions;
	D3DADAPTER_IDENTIFIER8 m_dddi;
	DWORD m_windowHeight;

	char* m_stickyAlloc;
	DWORD m_stickyAllocSize;

	bool m_hintGenerateMipMaps;
	GLenum m_FogHint;

	D3DCAPS8 m_deviceCaps;

	HRESULT ReleaseD3DX()
	{
		m_bD3DXReady = FALSE;
		return S_OK;
	}
	
	HRESULT InitD3DX()
	{
		HRESULT hr;

		if( NULL == ( m_pD3D  = Direct3DCreate8( D3D_SDK_VERSION ) ) )
        return E_FAIL;

		// Set up the structure used to create the D3DDevice.
		D3DPRESENT_PARAMETERS params; 
		ZeroMemory( &params, sizeof(params) );

		// Set up the parameters
		params.EnableAutoDepthStencil = TRUE;
		params.AutoDepthStencilFormat = D3DFMT_D16;
		params.SwapEffect             = D3DSWAPEFFECT_DISCARD; 
		params.BackBufferWidth        = gWidth;
		params.BackBufferHeight       = gHeight;
		params.BackBufferFormat       = D3DFMT_X8R8G8B8;
		params.FullScreen_RefreshRateInHz = 60;
		params.MultiSampleType = D3DMULTISAMPLE_2_SAMPLES_MULTISAMPLE_LINEAR;

#define USE_PUSHBUFFER
#ifdef USE_PUSHBUFFER
		// Set push buffer size -> Only used if parameter RunUsingCpuCopy == true (final size / kickoff size must be greater than 4!)
		// The docs suggest RunUsingCpuCopy to be true if we are using a small buffer (to avoid overhead)
		// This is why we set the push buffer size here (512 kbytes is too low by the way)
		hr = m_pD3D->SetPushBufferSize(/*1536*/768*1024, 128*1024);
		if( FAILED(hr) )
			return hr;
#endif
		// Now check if player is a PAL 60 user
		DWORD videoFlags = XGetVideoFlags();

		if(XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I)
		{
			if(videoFlags & XC_VIDEO_FLAGS_PAL_60Hz)		// PAL 60 user
				params.FullScreen_RefreshRateInHz = 60;
			else
				params.FullScreen_RefreshRateInHz = 50;
		}

		//use progressive mode if possible
		if(XGetAVPack() == XC_AV_PACK_HDTV && gVideoMode > 0)
		{
			/*if(videoFlags & XC_VIDEO_FLAGS_HDTV_1080i && gWidth == 1920 && gHeight == 1080)
			{
				//Out of memory very likely!
				params.Flags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
			} 
			else*/if(videoFlags & XC_VIDEO_FLAGS_HDTV_720p && gWidth == 1280 && gHeight == 720)
			{
				params.Flags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
			}
			else if(videoFlags & XC_VIDEO_FLAGS_HDTV_480p && gWidth == 640 && gHeight == 480)
			{
				params.Flags = D3DPRESENTFLAG_PROGRESSIVE;
			}
			else if(videoFlags & XC_VIDEO_FLAGS_HDTV_480p)
			{
				//Force valid resolution and at least try progressive mode
				gWidth = 640;
				gHeight = 480;
				params.Flags = D3DPRESENTFLAG_PROGRESSIVE;
			}
			else
			{
				//Something went wrong, fall back
				gVideoMode = 0;
			}
		}
		else // No component cables detected
		{
			//Fallback to standard setup (480i)
			params.BackBufferWidth = gWidth = 640;
			params.BackBufferHeight = gHeight = 480;
			params.Flags = D3DPRESENTFLAG_INTERLACED;
		}

		// Create the Direct3D device.
		hr =  m_pD3D->CreateDevice( 0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &params, &m_pD3DDev );
		if( FAILED(hr) )
			return hr;

		// Store render target surface desc
		{
			LPDIRECT3DSURFACE8 pBackBuffer;
			m_pD3DDev->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
			hr = pBackBuffer->GetDesc(&m_d3dsdBackBuffer);
			pBackBuffer->Release();
			if( FAILED(hr) )
				return hr;
		}

		// Apply visual improvements
		m_pD3DDev->SetFlickerFilter(1);
		m_pD3DDev->SetSoftDisplayFilter(true);

		// We are done here
		m_bD3DXReady = TRUE;

		return hr;
	}
	
	void InterpretError(HRESULT hr)
	{
		char errStr[100];
		D3DXGetErrorString(hr, errStr, sizeof(errStr) / sizeof(errStr[0]) );
		OutputDebugString(errStr);
		LocalDebugBreak();
	}


public:
	FakeGL(/*HWND hwndMain*/)
	{
		//m_hwndMain = hwndMain;
		//m_windowHeight = 480;
		m_bD3DXReady = TRUE;

		m_pD3DDev = 0;
		m_pD3D = 0;
		m_pPrimary = 0;
		m_hardwareTandL = false;

		m_glRenderStateDirty = true;

		m_glAlphaStateDirty = true;
		m_glAlphaFunc = GL_ALWAYS;
		m_glAlphaFuncRef = 0;
		m_glAlphaTest = false;

		m_glBlendStateDirty = true;
		m_glBlend = false;
		m_glBlendFuncSFactor = GL_ONE; // Not sure this is the default
		m_glBlendFuncDFactor = GL_ZERO; // Not sure this is the default
	
		m_glCullStateDirty = true;
		m_glCullFace = false;

		m_glCullFaceMode = GL_BACK;
		m_glCullFrontFaceMode = GL_CCW;
		
		m_PolyOffsetEnabled = false;
		m_PolyOffsetSwitched = false;
		m_PolyOffsetFactor = 0;//8; //MARTY - HACK for Xash3D

		m_bFogSwitched = false;
		m_bFogEnabled = false;

		m_glDepthStateDirty = true;
		m_glDepthTest = false;
		m_glDepthMask = true;
		m_glDepthFunc = GL_ALWAYS; // not sure if this is the default

		m_glDepthRangeNear = 0; // not sure if this is the default
		m_glDepthRangeFar = 1.0; // not sure if this is the default

		m_glMatrixMode = GL_MODELVIEW; // Not sure this is the default

		m_glPolygonModeFront = GL_FILL;
		m_glPolygonModeBack = GL_FILL;

		m_glShadeModelStateDirty = true;
		m_glShadeModel = GL_SMOOTH;

		m_bViewPortDirty = true;
		m_glViewPortX = 0;
		m_glViewPortY = 0;
							
		m_glViewPortWidth = gWidth;
		m_glViewPortHeight = gHeight;

		m_vendor = 0;
		m_renderer = 0;
		m_extensions = 0;

		m_hintGenerateMipMaps = false; //MARTY FIXME - Should be true by default
		m_FogHint = GL_NICEST;

		
		HRESULT hr = InitD3DX();
		if ( FAILED(hr) ) 
			InterpretError(hr);
		
		hr = D3DXCreateMatrixStack(0, &m_modelViewMatrixStack);
		hr = D3DXCreateMatrixStack(0, &m_projectionMatrixStack);
		hr = D3DXCreateMatrixStack(0, &m_textureMatrixStack);
		m_currentMatrixStack = m_modelViewMatrixStack;
		m_modelViewMatrixStack->LoadIdentity(); // Not sure this is correct
		m_projectionMatrixStack->LoadIdentity();
		m_textureMatrixStack->LoadIdentity();
		m_modelViewMatrixStateDirty = true;
		m_projectionMatrixStateDirty = true;
		m_textureMatrixStateDirty = true;
		m_currentMatrixStateDirty = &m_modelViewMatrixStateDirty;
		m_viewMatrixStateDirty = true;

		D3DXMatrixIdentity(&m_d3dViewMatrix);

		m_needBeginScene = true;

		m_stickyAlloc = 0;
		m_stickyAllocSize = 0;

		{
			// Check for multitexture.	
			HRESULT hr = m_pD3DDev->GetDeviceCaps(&m_deviceCaps);
			if ( ! FAILED(hr)) 
			{
				// Clamp texture blend stages to 2. Some cards can do eight, but that's more
				// than we need.
				int maxStages = m_deviceCaps.MaxTextureBlendStages;
				if ( maxStages > 2 )
				{
					maxStages = 2;
				}
				m_textureState.SetMaxStages(maxStages);

				m_hardwareTandL = (m_deviceCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0;
			}
		}

		// One-time render state initialization
		m_pD3DDev->SetRenderState( D3DRS_TEXTUREFACTOR, 0x00000000 );
		m_pD3DDev->SetRenderState( D3DRS_DITHERENABLE, 0 ); //FALSE looks worse in 16 bit mode (D3DFMT_X1R5G5B5)
		m_pD3DDev->SetRenderState( D3DRS_SPECULARENABLE, FALSE );
		m_pD3DDev->SetRenderState( D3DRS_LIGHTING, FALSE);
	}
	~FakeGL()
	{
		delete [] m_stickyAlloc;
		ReleaseD3DX();
		RELEASENULL(m_modelViewMatrixStack);
		RELEASENULL(m_projectionMatrixStack);
		RELEASENULL(m_textureMatrixStack);
	}

	LPDIRECT3DDEVICE8 GetD3DDevice()
	{
		if(m_pD3DDev)
			return m_pD3DDev;

		return NULL;
	}

	void glAlphaFunc (GLenum func, GLclampf ref)
	{
		if ( m_glAlphaFunc != func || m_glAlphaFuncRef != ref )
		{
			SetRenderStateDirty();
			m_glAlphaFunc = func;
			m_glAlphaFuncRef = ref;
			m_glAlphaStateDirty = true;
		}
	}
	
	void glBegin (GLenum mode)
	{
		if ( m_needBeginScene )
		{
			m_needBeginScene = false;
			HRESULT hr = m_pD3DDev->BeginScene();
			if ( FAILED(hr) )
			{
				InterpretError(hr);
			}
		}

#if 0
		// statistics
		static int beginCount;
		static int stateChangeCount;
		static int primitivesCount;
		beginCount++;
		if ( m_glRenderStateDirty )
			stateChangeCount++;
		if ( m_glRenderStateDirty || ! m_OGLPrimitiveVertexBuffer.IsMergableMode(mode) )
			primitivesCount++;
#endif
		if ( m_glRenderStateDirty || ! m_OGLPrimitiveVertexBuffer.IsMergableMode(mode) ) 
		{
			internalEnd();
			SetGLRenderState();
			DWORD typeDesc;
			typeDesc = D3DFVF_XYZ | D3DFVF_DIFFUSE;
			typeDesc |= (m_textureState.GetMaxStages() << D3DFVF_TEXCOUNT_SHIFT);

			if ( typeDesc != m_OGLPrimitiveVertexBuffer.GetVertexTypeDesc()) 
			{
				m_OGLPrimitiveVertexBuffer.Initialize(m_pD3DDev, m_pD3D, m_hardwareTandL, typeDesc);
			}
			m_OGLPrimitiveVertexBuffer.Begin(mode);
		}
		else
		{
			m_OGLPrimitiveVertexBuffer.Append(mode);
		}
	}

	void glBindTexture(GLenum target, GLuint texture)
	{
		if ( target != GL_TEXTURE_2D ) 
		{
			LocalDebugBreak();
			return;
		}

		if ( m_textureState.GetCurrentTexture() != texture ) 
		{
			SetRenderStateDirty();
			m_textureState.SetCurrentTexture(texture);
			m_textures.BindTexture(texture);
		}
	}

	void glDeleteTextures (GLsizei n, const GLuint *textures)
	{
		m_textures.DeleteTextures(n, textures);
	}

	inline void glMTexCoord2fSGIS(GLenum target, GLfloat s, GLfloat t)
	{
		int textStage = target - TEXTURE0_SGIS;
		m_OGLPrimitiveVertexBuffer.SetTextureCoord(textStage, s, t);
	}
	
	inline void glSelectTextureSGIS(GLenum target)
	{
		int textStage = target - TEXTURE0_SGIS;
		m_textureState.SetCurrentStage(textStage);
		m_textures.BindTexture(m_textureState.GetCurrentTexture());
		// Does not, by itself, dirty the render state
	}

	void glBlendFunc (GLenum sfactor, GLenum dfactor)
	{
		if ( m_glBlendFuncSFactor != sfactor || m_glBlendFuncDFactor != dfactor ) 
		{
			SetRenderStateDirty();
			m_glBlendFuncSFactor = sfactor;
			m_glBlendFuncDFactor = dfactor;
			m_glBlendStateDirty = true;
		}
	}

	void glFogfv (GLenum pname, const GLfloat *params)
	{
		switch (pname)
		{
			case GL_FOG_COLOR:
			m_pD3DDev->SetRenderState(D3DRS_FOGCOLOR, GLColorToD3D (params[0], params[1], params[2], params[3]));
			break;

			default:
			glFogf (pname, params[0]);
			break;
		}
	}

	void glFogf (GLenum pname, GLfloat param)
	{
		switch (pname)
		{
			case GL_FOG_MODE:
			glFogi (pname, param);
			break;

			case GL_FOG_DENSITY:
			m_pD3DDev->SetRenderState(D3DRS_FOGDENSITY, D3DFloatToDWORD (param));
			break;

			case GL_FOG_START:
			m_pD3DDev->SetRenderState(D3DRS_FOGSTART, D3DFloatToDWORD (param));
			break;

			case GL_FOG_END:
			m_pD3DDev->SetRenderState(D3DRS_FOGEND, D3DFloatToDWORD (param));
			break;

			case GL_FOG_COLOR:
			break;

			default:
			break;
		}
	}

	void glFogi (GLenum pname, GLint param)
	{
		switch (pname)
		{
			case GL_FOG_MODE:
			// TODO - Make these dependent on a glHint...
			if (param == GL_LINEAR)
			{
				if (m_FogHint == GL_NICEST)
				{
					m_pD3DDev->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
//					m_pD3DDev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
				}
				else
				{
					m_pD3DDev->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
//					m_pD3DDev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
				}
			}
			else if (param == GL_EXP)
			{
				if (m_FogHint == GL_NICEST)
				{
					m_pD3DDev->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_EXP);
//					m_pD3DDev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
				}
				else
				{
					m_pD3DDev->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
//					m_pD3DDev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_EXP);
				}
			}
			else
			{
				if (m_FogHint == GL_NICEST)
				{
				m_pD3DDev->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_EXP2);
//				m_pD3DDev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
			}
			else
			{
				m_pD3DDev->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
//				m_pD3DDev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_EXP2);
			}
		}
		break;

	default:
		glFogf (pname, param);
		break;
	}
}

	inline void glClear (GLbitfield mask)
	{
		internalEnd();
		SetGLRenderState();
		DWORD clearMask = 0;

		if ( mask & GL_COLOR_BUFFER_BIT )
			clearMask |= D3DCLEAR_TARGET;

		if ( mask & GL_DEPTH_BUFFER_BIT ) 
			clearMask |= D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL; 

		// see Depth-Buffer Compression and Performance Implications in the docs!
		// Quake does not use a stencil buffer, but we need to clear it anyways else
		// the performance will go down

		if ( mask & GL_STENCIL_BUFFER_BIT )
			clearMask |= D3DCLEAR_STENCIL;

		m_pD3DDev->Clear(0, NULL, clearMask, 0, 1.0f, 0 );
	}

	void glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
	{
		D3DCOLOR clearColor = D3DRGBA(Clamp(red), Clamp(green), Clamp(blue), Clamp(alpha));
	}

	inline void glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
	{
		DWORD mask = 0;

		if (red) mask |= D3DCOLORWRITEENABLE_RED;
		if (green) mask |= D3DCOLORWRITEENABLE_GREEN;
		if (blue) mask |= D3DCOLORWRITEENABLE_BLUE;
		if (alpha) mask |= D3DCOLORWRITEENABLE_ALPHA;

		m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, mask);
	}

	inline void glColor3f (GLfloat red, GLfloat green, GLfloat blue)
	{
		// Note: On x86 architectures this function will chew up a lot of time
		// converting floating point to integer by calling _ftol
		// unless the /QIfist flag is specified.
		m_OGLPrimitiveVertexBuffer.SetColor(D3DRGB(Clamp(red), Clamp(green), Clamp(blue)));
	}

	inline void glColor3ubv (const GLubyte *v)
	{
		m_OGLPrimitiveVertexBuffer.SetColor(RGBA_MAKE(v[0], v[1], v[2], 0xff));
	}

	inline void glColor4ubv (const GLubyte *v)
	{
		m_OGLPrimitiveVertexBuffer.SetColor(RGBA_MAKE(v[0], v[1], v[2], v[3]));
	}

	inline void glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
	{
		// Note: On x86 architectures this function will chew up a lot of time
		// converting floating point to integer by calling _ftol
		// unless the /QIfist flag is specified.
		m_OGLPrimitiveVertexBuffer.SetColor(D3DRGBA(Clamp(red), Clamp(green), Clamp(blue), Clamp(alpha)));
	}

	inline void glColor4fv (const GLfloat *v)
	{
		// Note: On x86 architectures this function will chew up a lot of time
		// converting floating point to integer by calling _ftol
		// unless the /QIfist flag is specified.
		m_OGLPrimitiveVertexBuffer.SetColor(D3DRGBA(Clamp(v[0]), Clamp(v[1]), Clamp(v[2]), Clamp(v[3])));
	}

	inline void glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
	{
		m_OGLPrimitiveVertexBuffer.SetColor(RGBA_MAKE(red, green, blue, alpha));
	}
	
	inline void glPointSize (const GLfloat v)
	{
		m_pD3DDev->SetRenderState(D3DRS_POINTSIZE, (DWORD)v);
	}

	void glCullFace (GLenum mode)
	{
		if ( m_glCullFaceMode != mode ) 
		{
			SetRenderStateDirty();
			m_glCullFaceMode = mode;
			m_glCullStateDirty = true;
		}
	}

	void glFrontFace (GLenum mode)
	{
		if ( m_glCullFrontFaceMode != mode ) 
		{
			SetRenderStateDirty();
			m_glCullFrontFaceMode = mode;
			m_glCullStateDirty = true;
		}
	}

	void glPolygonOffset (GLfloat factor, GLfloat units)
	{
		// need to switch polygon offset
		m_PolyOffsetSwitched = false;

		// just use the units here as we're going to fudge it using D3DRS_ZBIAS
		// 0 is furthest; 16 is nearest; d3d default is 0 so our default is an intermediate value (8)
		// so that we can do both push back and pull forward
		// negative values come nearer; positive values go further, so invert the sense

		m_PolyOffsetFactor = /*8*/4 /*-*/+ units; // MARTY - HACK This is a nasty hack for Xash3D only 
												  // Polygons look very glitchy if we dont do this!
		// clamp to d3d scale
		if (m_PolyOffsetFactor < 0) m_PolyOffsetFactor = 0;
		if (m_PolyOffsetFactor > 16) m_PolyOffsetFactor = 16;
	}

	void glDepthFunc (GLenum func)
	{
		if ( m_glDepthFunc != func ) 
		{
			SetRenderStateDirty();
			m_glDepthFunc = func;
			m_glDepthStateDirty = true;
		}
	}

	void glDepthMask (GLboolean flag)
	{
		if ( m_glDepthMask != (flag != 0) ) 
		{
			SetRenderStateDirty();
			m_glDepthMask = flag != 0 ? true : false;
			m_glDepthStateDirty = true;
		}
	}

	void glDepthRange (GLclampd zNear, GLclampd zFar)
	{
		if ( m_glDepthRangeNear != zNear || m_glDepthRangeFar != zFar ) 
		{
			SetRenderStateDirty();
			m_glDepthRangeNear = zNear;
			m_glDepthRangeFar = zFar;
			m_bViewPortDirty = true;
		}
	}

	void glEnable (GLenum cap)
	{
		glEnableDisableSet(cap, true); 
	}

	void glDisable (GLenum cap)
	{
		glEnableDisableSet(cap, false);
	}

	void glDrawBuffer (GLenum /* mode */)
	{
		// Do nothing (can DirectX render to the front buffer at all?)
	}

	void glGetIntegerv (GLenum pname, GLint *params)
	{
		// here we only bother getting the values that glquake uses
		switch (pname)
		{
			case GL_MAX_TEXTURE_SIZE:
				// D3D allows both to be different so return the lowest
				params[0] = (m_deviceCaps.MaxTextureWidth > m_deviceCaps.MaxTextureHeight ? m_deviceCaps.MaxTextureHeight : m_deviceCaps.MaxTextureWidth);
				break;

			case GL_VIEWPORT:
				params[0] = m_glViewPortX;
				params[1] = m_glViewPortY;
				params[2] = m_glViewPortWidth;
				params[3] = m_glViewPortHeight;
				break;

			case GL_STENCIL_BITS:
/*				if (params.AutoDepthStencilFormat == D3DFMT_D24S8)
				params[0] = 8;
				else if (params.AutoDepthStencilFormat == D3DFMT_D24X4S4)
				params[0] = 4;
				else params[0] = 0;
*/				break;

			default:
				params[0] = 0;
				return;
		}
	}

	GLenum glGetError (void) // MARTY - Dummy function
	{
		return GL_NO_ERROR;
	}

	void glEnableDisableSet(GLenum cap, bool value)
	{
		switch ( cap ) 
		{
		case GL_ALPHA_TEST:
			if ( m_glAlphaTest != value ) 
			{
				SetRenderStateDirty();
				m_glAlphaTest = value;
				m_glAlphaStateDirty = true;
			}
			break;
		case GL_BLEND:
			if ( m_glBlend != value )
			{
				SetRenderStateDirty();
				m_textureState.SetMainBlend(value); 
				m_glBlend = value;
				m_glBlendStateDirty = true;
			}
			break;
		case GL_CULL_FACE:
			if ( m_glCullFace != value )
			{
				SetRenderStateDirty();
				m_glCullFace = value;
				m_glCullStateDirty = true;
			}
			break;
		case GL_DEPTH_TEST:
			if ( m_glDepthTest != value ) 
			{
				SetRenderStateDirty();
				m_glDepthTest = value;
				m_glDepthStateDirty = true;
			}
		break;
		case GL_TEXTURE_2D:
			if ( m_textureState.GetTexture2D() != value )
			{
				SetRenderStateDirty();
				m_textureState.SetTexture2D(value);
			}
			break;
		case GL_POLYGON_OFFSET_FILL:
		case GL_POLYGON_OFFSET_LINE:
			if ( m_PolyOffsetEnabled != value )
			{	
				SetRenderStateDirty();
				m_PolyOffsetEnabled = value;
				m_PolyOffsetSwitched = false;
			}
			break;
		case GL_FOG:
			if ( m_bFogEnabled != value )
			{	
				SetRenderStateDirty();
				m_bFogEnabled = value;
				m_bFogSwitched = false;
			}
			break;
		default:
			LocalDebugBreak();
			break;
		}
	}

	void glEnd (void)
	{
		// internalEnd();
	}

	void internalEnd()
	{
		m_OGLPrimitiveVertexBuffer.End();
	}

	void glFinish (void)
	{
		// To Do: This is supposed to flush all pending commands
		internalEnd();
	}
	
	void glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
	{
		SetRenderStateDirty();
		D3DXMATRIX m;
		// Note that D3D takes top, bottom arguments in opposite order
		D3DXMatrixPerspectiveOffCenterRH(&m, left, right, bottom, top, zNear, zFar);
		m_currentMatrixStack->MultMatrixLocal(&m);
		*m_currentMatrixStateDirty = true;
	}

	void glGetFloatv (GLenum pname, GLfloat *params)
	{
		switch(pname)
		{
		case GL_MODELVIEW_MATRIX:
			memcpy(params,m_modelViewMatrixStack->GetTop(), sizeof(D3DMATRIX));
			break;
		default:
			LocalDebugBreak();
			break;
		}
	}

	const GLubyte * glGetString (GLenum name)
	{
		const char* result = "";
		EnsureDriverInfo();
		switch ( name )
		{
		case GL_VENDOR:
			result = m_vendor;
			break;
		case GL_RENDERER:
			result = m_renderer;
			break;
		case GL_VERSION:
			result = m_version;
			break;
		case GL_EXTENSIONS:
			result = m_extensions;
			break;
		default:
			break;
		}
		return (const GLubyte *) result;
	}

	void glHint (GLenum  target , GLenum mode)
	{
		switch (target)
		{
			case GL_FOG_HINT:
			m_FogHint = mode;
			break;

			default:
			// ignore other driver hints
			break;
		}
	}

	GLboolean glIsEnabled(GLenum cap)
	{
		switch(cap)
		{
		case GL_ALPHA_TEST:
			return  m_glAlphaFunc != GL_ALWAYS;
		case GL_FOG:
			return m_bFogEnabled;
		default:
			return FALSE;
		}
	}

	void glLoadIdentity (void)
	{
		SetRenderStateDirty();
		m_currentMatrixStack->LoadIdentity();
		*m_currentMatrixStateDirty = true;
	}

	void glLoadMatrixf (const GLfloat *m)
	{
		SetRenderStateDirty();
		m_currentMatrixStack->LoadMatrix((D3DXMATRIX*) m);
		*m_currentMatrixStateDirty = true;
	}

	void glMatrixMode (GLenum mode)
	{
		m_glMatrixMode = mode;
		switch ( mode ) 
		{
		case GL_MODELVIEW:
			m_currentMatrixStack = m_modelViewMatrixStack;
			m_currentMatrixStateDirty = &m_modelViewMatrixStateDirty;
			break;
		case GL_PROJECTION:
			m_currentMatrixStack = m_projectionMatrixStack;
			m_currentMatrixStateDirty = &m_projectionMatrixStateDirty;
			break;
		case GL_TEXTURE:
			m_currentMatrixStack = m_textureMatrixStack;
			m_currentMatrixStateDirty = &m_textureMatrixStateDirty;
			break;
		default:
			LocalDebugBreak();
			break;
		}
	}

	void glOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
	{
		SetRenderStateDirty();
		D3DXMATRIX m;
		D3DXMatrixOrthoOffCenterRH(&m, left, right, top, bottom, zNear, zFar);
		m_currentMatrixStack->MultMatrixLocal(&m);
		*m_currentMatrixStateDirty = true;
	}

	void glPolygonMode (GLenum face, GLenum mode)
	{
		SetRenderStateDirty();
		switch ( face )
		{
		case GL_FRONT:
			m_glPolygonModeFront = mode;
			break;
		case GL_BACK:
			m_glPolygonModeBack = mode;
			break;
		case GL_FRONT_AND_BACK:
			m_glPolygonModeFront = mode;
			m_glPolygonModeBack = mode;
			break;
		default:
			LocalDebugBreak();
			break;
		}
	}

	void glPopMatrix (void)
	{
		SetRenderStateDirty();
		m_currentMatrixStack->Pop();
		*m_currentMatrixStateDirty = true;
	}

	void glPushMatrix (void)
	{
		m_currentMatrixStack->Push();
		// Doesn't dirty matrix state
	}

	void glReadBuffer (GLenum /* mode */)
	{
		// Not that we allow reading from various buffers anyway.
	}

	void glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
	{
		if ( format != GL_RGB || type != GL_UNSIGNED_BYTE) 
		{
			LocalDebugBreak();
			return;
		}
		internalEnd();

		unsigned char *srcdata;
		unsigned char *dstdata = (unsigned char *) pixels;
		LPDIRECT3DSURFACE8 bbsurf;
		LPDIRECT3DSURFACE8 locksurf;
		D3DLOCKED_RECT lockrect;
		D3DSURFACE_DESC desc;

		m_pD3DDev->GetBackBuffer ( -1, D3DBACKBUFFER_TYPE_MONO, &bbsurf);

		if (!bbsurf)
		{
			LocalDebugBreak();
			return;
		}

		// because we don't have a lockable backbuffer we instead copy it off to an image surface
		// this will also handle translation between different backbuffer formats
		bbsurf->GetDesc(&desc);
		m_pD3DDev->CreateImageSurface(desc.Width, desc.Height, /*D3DFMT_R8G8B8*/D3DFMT_LIN_X8R8G8B8, &locksurf); //CHECKME: Linear/Compressed format ?
		D3DXLoadSurfaceFromSurface (locksurf, NULL, NULL, bbsurf, NULL, NULL, D3DX_FILTER_NONE, 0);

		// now we have a surface we can lock
		locksurf->LockRect(&lockrect, NULL, D3DLOCK_READONLY);
		srcdata = (unsigned char *) lockrect.pBits;

		for (int row = y; row < (y + height); row++)
		{
			for (int col = x; col < (x + width); col++)
			{
				int srcpos = row * width + col;

				dstdata[0] = srcdata[srcpos * 4 + 0];
				dstdata[1] = srcdata[srcpos * 4 + 1];
				dstdata[2] = srcdata[srcpos * 4 + 2];

				dstdata += 3;
			}
		}

		locksurf->UnlockRect();

		RELEASENULL(locksurf);
		RELEASENULL(bbsurf);
	}

	inline void glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
	{
		SetRenderStateDirty();
		D3DXMATRIX m;
		D3DXVECTOR3 v;
		v.x = x;
		v.y = y;
		v.z = z;
		// GL uses counterclockwise degrees, DX uses clockwise radians
		float dxAngle = angle * 3.14159265359 / 180;
		m_currentMatrixStack->RotateAxisLocal(&v, dxAngle);
		*m_currentMatrixStateDirty = true;
	}

	inline void glScalef (GLfloat x, GLfloat y, GLfloat z)
	{
		SetRenderStateDirty();
		D3DXMATRIX m;
		D3DXMatrixScaling(&m, x, y, z);
		m_currentMatrixStack->MultMatrixLocal(&m);
		*m_currentMatrixStateDirty = true;
	}

	void glShadeModel (GLenum mode)
	{
		if ( m_glShadeModel != mode )
		{
			SetRenderStateDirty();
			m_glShadeModel = mode;
			m_glShadeModelStateDirty = true;
		}
	}

	inline void glTexCoord2f (GLfloat s, GLfloat t)
	{
		m_OGLPrimitiveVertexBuffer.SetTextureCoord0(s, t);
	}

	void glTexEnvf (GLenum /* target */, GLenum /* pname */, GLfloat param)
	{
		// ignore target, which must be GL_TEXTURE_ENV
		// ignore pname, which must be GL_TEXTURE_ENV_MODE
		if ( m_textureState.GetTextEnvMode() != param ) 
		{
			SetRenderStateDirty();
			m_textureState.SetTextEnvMode(param);
		}
	}

	static int MipMapSize(DWORD width, DWORD height)
	{
		DWORD n = width < height? width : height;
		DWORD result = 1;
		while (n > (DWORD) (1 << result) ) 
		{
			result++;
		}
		return result;
	}

#define LOAD_OURSELVES

	void glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width,
		GLsizei height, GLint /* border */, GLenum format, GLenum type, const GLvoid *pixels, int iSubImage)
	{
		HRESULT hr;
		if ( target != GL_TEXTURE_2D || type != GL_UNSIGNED_BYTE) 
		{
			InterpretError(E_FAIL);
			return;
		}

		DWORD dxWidth = width;
		DWORD dxHeight = height;

		D3DFORMAT srcPixelFormat = GLToDXPixelFormat(internalformat, format);
		D3DFORMAT destPixelFormat = srcPixelFormat;
		// Can the surface handle that format?
		hr = m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_d3dsdBackBuffer.Format,
			0, D3DRTYPE_TEXTURE, destPixelFormat);
		if ( FAILED(hr) ) 
		{
			if ( g_force16bitTextures ) 
			{
				destPixelFormat = D3DFMT_A4R4G4B4;
				hr = m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_d3dsdBackBuffer.Format,
					0, D3DRTYPE_TEXTURE, destPixelFormat);
				if ( FAILED(hr) ) 
				{
					// Don't know what to do.
					InterpretError(E_FAIL);
					return;
				}
			}
			else 
			{
				destPixelFormat = D3DFMT_A8R8G8B8;
				hr = m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_d3dsdBackBuffer.Format,
					0, D3DRTYPE_TEXTURE, destPixelFormat);
				if ( FAILED(hr) ) 
				{
					// The card can't handle this pixel format. Switch to D3DX_SF_A4R4G4B4
					destPixelFormat = D3DFMT_A4R4G4B4;
					hr = m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_d3dsdBackBuffer.Format,
						0, D3DRTYPE_TEXTURE, destPixelFormat);
					if ( FAILED(hr) )
					{
						// Don't know what to do.
						InterpretError(E_FAIL);
						return;
					}
				}
			}
		}

#ifdef LOAD_OURSELVES

		char* goodSizeBits = (char*) pixels;
		if ( dxWidth != (DWORD) width || dxHeight != (DWORD) height )
		{
			// Most likely this is because there is a 256 x 256 limit on the texture size
			goodSizeBits = new char[sizeof(DWORD) * dxWidth * dxHeight]; 
			DWORD* dest = ((DWORD*) goodSizeBits);
			for ( DWORD y = 0; y < dxHeight; y++) 
			{
				DWORD sy = y * height / dxHeight;
				for(DWORD x = 0; x < dxWidth; x++) 
				{
					DWORD sx = x * width / dxWidth;
					DWORD* source = ((DWORD*) pixels) + sy * dxWidth + sx;
					*dest++ = *source;
				}
			}
			width = dxWidth;
			height = dxHeight;
		}
		// To do: Convert the pixels on the fly while copying into the DX texture.
		char* compatablePixels;
		DWORD compatablePixelsPitch;

		hr = ConvertToCompatablePixels(internalformat, width, height, format,
				type, destPixelFormat, goodSizeBits, &compatablePixels, &compatablePixelsPitch);

		if ( goodSizeBits != pixels ) 
		{
			delete [] goodSizeBits;
		}
		if ( FAILED(hr))
		{
			InterpretError(hr);
			return;
		}

#endif

		IDirect3DTexture8* pMipMap = m_textures.GetMipMap();
		if ( pMipMap )
		{
			// DX8 textures don't know much. Always reset texture for level zero.
			if ( level == 0 ) 
			{
				m_textures.SetTexture(NULL, D3DFMT_UNKNOWN, 0);
				pMipMap = 0;
			}
			// For non-square textures, OpenGL uses more MIPMAP levels than DirectX does.
			else if ( level >= (int)pMipMap->GetLevelCount() ) 
			{
				return;
			}
		}

		if(!pMipMap) 
		{
			int levels = 1;
			if ( m_hintGenerateMipMaps )
				levels = MipMapSize(width, height);

			hr = m_pD3DDev->CreateTexture(width, height, levels, 0, destPixelFormat, D3DPOOL_MANAGED, &pMipMap);

			if ( FAILED(hr) )
			{
				InterpretError(hr);
				return;
			}
			m_textures.SetTexture(pMipMap, destPixelFormat, internalformat);
		}

		if( m_textures.SubTexturesCached() && m_textures.IsSubTexture() )
		{ 
			glTexSubImage2D_Imp(pMipMap, level, 0, 0, width, height, format, type, compatablePixels, compatablePixelsPitch, true);

			m_textures.SetSubTexture(false);
		}
		else
			glTexSubImage2D_Imp(pMipMap, level, 0, 0, width, height, format, type, compatablePixels, compatablePixelsPitch, false);

  		if ( FAILED(hr) ) 
		{
			InterpretError(hr);
			return;
		}
	}

	void glTexParameterf (GLenum target, GLenum pname, GLfloat param)
	{
		switch(target)
		{
		case GL_TEXTURE_2D:
			{
				SetRenderStateDirty();
				TextureEntry* current = m_textures.GetCurrentEntry();
				m_textureState.DirtyTexture(m_textures.GetCurrentID());
				
				switch(pname)
				{
				case GL_TEXTURE_MIN_FILTER:
					current->m_glTexParameter2DMinFilter = param;
					break;
				case GL_TEXTURE_MAG_FILTER:
					current->m_glTexParameter2DMagFilter = param;
					break;
				case GL_TEXTURE_WRAP_S:
					current->m_glTexParameter2DWrapS = param;
					break;
				case GL_TEXTURE_WRAP_T:
					current->m_glTexParameter2DWrapT = param;
					break;
				case D3D_TEXTURE_MAXANISOTROPY:
					current->m_maxAnisotropy = param;
					break;
				default:
					LocalDebugBreak();
				}
			}
			break;
		default:
			LocalDebugBreak();
			break;
		}
	}

	void glTexSubImage2D (GLenum target, GLint level,
		GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
		GLenum format, GLenum type, const GLvoid *pixels)
	{
		if ( target != GL_TEXTURE_2D ) 
		{
			LocalDebugBreak();
			return;
		}

		if ( width <= 0 || height <= 0 )
			return;

		IDirect3DTexture8* pTexture = m_textures.GetMipMap();
		if ( ! pTexture ) 
			return;

		internalEnd(); // We may have a pending drawing using the old texture state.

		// To do: Convert the pixels on the fly while copying into the DX texture.

		char* compatablePixels = 0;
		DWORD compatablePixelsPitch;

		if ( FAILED(ConvertToCompatablePixels(m_textures.GetInternalFormat(),
				width, height,
				format, type, m_textures.GetSurfaceFormat(),
				pixels, &compatablePixels, &compatablePixelsPitch))) 
		{
			LocalDebugBreak();
			return;
		}
		
		glTexSubImage2D_Imp(pTexture, level, xoffset, yoffset, width, height, format, type, compatablePixels, compatablePixelsPitch, true);
	}

	char* StickyAlloc(DWORD size)
	{
		if ( m_stickyAllocSize < size ) 
		{
			delete [] m_stickyAlloc;
			m_stickyAlloc = new char[size];
			m_stickyAllocSize = size;
		}
		return m_stickyAlloc;
	}

// Slower than just locking and unlocking. But both are really slow on NVIDIA hardware, due
// to texture swizzleing / unswizzleing.
// #define USE_IMAGE_SURFACE

	void glTexSubImage2D_Imp (IDirect3DTexture8* pMipMap, GLint level,
		GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
		GLenum /* format */, GLenum /* type */, const char* compatablePixels, int compatablePixelsPitch, bool subImage)
	{
	D3DLOCKED_RECT lockedRect;

	D3DLOCKED_RECT lockedRectSubImg;
	subImage_s* cachedSubImage;

	D3DSURFACE_DESC desc;
	pMipMap->GetLevelDesc(level, &desc); 
	
	// Check the formats and change bytesPerPixel accordingly
	switch ( desc.Format ) 
	{
		case D3DFMT_P8:
			
		case D3DFMT_AL8:
		case D3DFMT_A8:
		case D3DFMT_L8: //FIXME: Try workaround? -> convert to X8R8G8B8 / A8R8G8B8 ???
			bytes = 1;
		break;

		case D3DFMT_R5G6B5:
		case D3DFMT_A4R4G4B4:
		case D3DFMT_A8L8:
					
			bytes = 2;
		break;

		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
			bytes = 4;
		break;

		default:
			// This really should not happen, as it's a format we do not know yet
			bytes = 4;
			LocalDebugBreak();
			break;
	}

	if(subImage)
	{
		cachedSubImage = m_textures.GetSubImageCache(m_pD3DDev, m_textures.GetCurrentID());

		if(!cachedSubImage)
			return;

		cachedSubImage->pTexture->LockRect( &lockedRectSubImg, NULL, 0);

		int x, y;
		byte *srcdata = (byte *)compatablePixels;
		byte *dstdata = (byte *)lockedRectSubImg.pBits;
				
		//TODO: setup for when dstbytes and srcbytes !=4 (not used in Xash3D atm)
		for (y = yoffset; y < (yoffset + height); y++)
		{
			for (x = xoffset; x < (xoffset + width); x++)
			{
				dstdata[lockedRectSubImg.Pitch * y + bytes * x ] = srcdata[0];
				dstdata[lockedRectSubImg.Pitch * y + bytes * x + 1] = srcdata[1];
				dstdata[lockedRectSubImg.Pitch * y + bytes * x + 2] = srcdata[2];
				dstdata[lockedRectSubImg.Pitch * y + bytes * x + 3] = srcdata[3];

				srcdata += bytes;
			}
		}
		cachedSubImage->pTexture->UnlockRect();
	}
	else // Normal texture
	{
		pMipMap->LockRect(level, &lockedRect, NULL, 0);

		const char* sp = compatablePixels;
		char* dp = (char*) lockedRect.pBits + yoffset * lockedRect.Pitch;

		if ( compatablePixelsPitch > lockedRect.Pitch )
			LocalDebugBreak();

		if ( compatablePixelsPitch != lockedRect.Pitch ) 
		{
			for(int i = 0; i < height; i++ ) 
			{
				memcpy(dp, sp, compatablePixelsPitch);
				sp += compatablePixelsPitch;
				dp += lockedRect.Pitch;
			}
		}
		else 
		{
			memcpy(dp, sp, compatablePixelsPitch * height);
		}

		pMipMap->UnlockRect(level);
	}

	if(!subImage)
	{
		//At this point we have a complete pMipMap, ready to be swizzled	
		D3DSurface *surface;
		D3DSurface *surfaceTemp;

		//Create temporary surface
		m_pD3DDev->CreateImageSurface(desc.Width, desc.Height, desc.Format, &surfaceTemp);

		//Lock the texture
		D3DLOCKED_RECT lr, lr2;
       	pMipMap->LockRect( level, &lr, NULL, 0 );

		//go down to surface level
		pMipMap->GetSurfaceLevel(level, &surface);

		//copy surf to temp surf
		D3DXLoadSurfaceFromSurface(surfaceTemp, NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE, NULL);

		surfaceTemp->LockRect(&lr2, NULL, NULL);

		// Xbox textures need to be swizzled
		XGSwizzleRect(
				lr2.pBits,		// pSource, 
				lr2.Pitch,		// Pitch,
				NULL,			// pRect,
				lr.pBits,		// pDest,
				desc.Width,		// Width,
				desc.Height,	// Height,
				NULL,			// pPoint,
				bytes);			// BytesPerPixel

			surfaceTemp->UnlockRect();
			surfaceTemp->Release();
			surface->Release();
			pMipMap->UnlockRect(level);
		}
		else
		{
			D3DLOCKED_RECT lr;

			pMipMap->LockRect(level, &lr, NULL, NULL);

			// Xbox textures need to be swizzled
			XGSwizzleRect(
            lockedRectSubImg.pBits,		// pSource, 
		    lockedRectSubImg.Pitch,		// Pitch,
            NULL,						// pRect,
            lr.pBits,					// pDest,
			desc.Width,					// Width,
			desc.Height,				// Height,
            NULL,						// pPoint,
            bytes );					// BytesPerPixel

			pMipMap->UnlockRect(level);
		}
	}

	inline void glTranslatef (GLfloat x, GLfloat y, GLfloat z)
	{
		SetRenderStateDirty();
		D3DXMATRIX m;
		D3DXMatrixTranslation(&m, x, y, z);
		m_currentMatrixStack->MultMatrixLocal(&m);
		*m_currentMatrixStateDirty = true;
	}

	inline void glVertex2f (GLfloat x, GLfloat y)
	{
		m_OGLPrimitiveVertexBuffer.SetVertex(x, y, 0);
	}

	inline void glVertex3f (GLfloat x, GLfloat y, GLfloat z)
	{
		m_OGLPrimitiveVertexBuffer.SetVertex(x, y, z);
	}

	inline void glVertex3fv (const GLfloat *v)
	{
		m_OGLPrimitiveVertexBuffer.SetVertex(v[0], v[1], v[2]);
	}

	void glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
	{
		if ( m_glViewPortX != x || m_glViewPortY != y ||
			m_glViewPortWidth != width || m_glViewPortHeight != height ) 
		{
			SetRenderStateDirty();
			m_glViewPortX = x;
			m_glViewPortY = y;
			m_glViewPortWidth = width;
			m_glViewPortHeight = height;

			m_bViewPortDirty = true;
		}
	}

	void SwapBuffers()
	{
		internalEnd();
		m_pD3DDev->EndScene();
		m_needBeginScene = true;

#ifdef _PROFILE
#define MB	(1024*1024)
#define AddStr(a,b) (pstrOut += wsprintf( pstrOut, a, b ))

		MEMORYSTATUS stat;
		CHAR strOut[1024], *pstrOut;

		// Get the memory status.
		GlobalMemoryStatus( &stat );

		// Setup the output string.
		pstrOut = strOut;
		AddStr( "%4d total MB of virtual memory.\n", stat.dwTotalVirtual / MB );
		AddStr( "%4d  free MB of virtual memory.\n", stat.dwAvailVirtual / MB );
		AddStr( "%4d total MB of physical memory.\n", stat.dwTotalPhys / MB );
		AddStr( "%4d  free MB of physical memory.\n", stat.dwAvailPhys / MB );
		AddStr( "%4d total MB of paging file.\n", stat.dwTotalPageFile / MB );
		AddStr( "%4d  free MB of paging file.\n", stat.dwAvailPageFile / MB );
		AddStr( "%4d  percent of memory is in use.\n", stat.dwMemoryLoad );

		// Output the string.
		OutputDebugString( strOut );

		D3DPERF_SetShowFrameRateInterval( 1000 );
#endif //_PROFILE

        m_pD3DDev->Present(NULL, NULL, NULL, NULL);
	}

	void SetGammaRamp(const unsigned char* gammaTable)
	{
		D3DGAMMARAMP gammaRamp;
		for(int i = 0; i < 256; i++ ) 
		{
			WORD value = gammaTable[i];
			value = value + (value << 8); // * 257
			gammaRamp.red[i] = value;
			gammaRamp.green[i] = value;
			gammaRamp.blue[i] = value;
		}
//		m_pD3DDev->SetGammaRamp(D3DSGR_NO_CALIBRATION, &gammaRamp); //MARTY FIXME
	}

	void Hint_GenerateMipMaps(int value)
	{
		m_hintGenerateMipMaps = value != 0;
	}

	void wglDeleteContext()
	{
		m_textures.ReleaseAllTextures();
	}

	void SetSubTexture()
	{
		m_textures.SetSubTexture(true);
	}

private:

	void SetRenderStateDirty()
	{
		if ( !m_glRenderStateDirty )
		{
			internalEnd();
			m_glRenderStateDirty = true;
		}
	}

	HRESULT HandleWindowedModeChanges()
	{
		return S_OK;
	}

	void SetGLRenderState()
	{
		if ( !m_glRenderStateDirty )
			return;

		m_glRenderStateDirty = false;
		HRESULT hr;
		if ( m_glAlphaStateDirty )
		{
			m_glAlphaStateDirty = false;
			// Alpha test
			m_pD3DDev->SetRenderState( D3DRS_ALPHATESTENABLE,
				m_glAlphaTest ? TRUE : FALSE );
			m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC,
				m_glAlphaTest ? GLToDXCompare(m_glAlphaFunc) : D3DCMP_ALWAYS);
			m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, 255 * m_glAlphaFuncRef);
		}
		if ( m_glBlendStateDirty )
		{
			m_glBlendStateDirty = false;
			// Alpha blending
			DWORD srcBlend = m_glBlend ? GLToDXSBlend(m_glBlendFuncSFactor) : D3DBLEND_ONE;
			DWORD destBlend = m_glBlend ? GLToDXDBlend(m_glBlendFuncDFactor) : D3DBLEND_ZERO;
			m_pD3DDev->SetRenderState( D3DRS_SRCBLEND,  srcBlend );
			m_pD3DDev->SetRenderState( D3DRS_DESTBLEND, destBlend );
			m_pD3DDev->SetRenderState( D3DRS_ALPHABLENDENABLE, m_glBlend ? TRUE : FALSE );
		}
	
		// Check polygon offset
		if (!m_PolyOffsetSwitched)
		{
			if (m_PolyOffsetEnabled)
			{
				// setup polygon offset
				m_pD3DDev->SetRenderState(D3DRS_ZBIAS, m_PolyOffsetFactor);
			}
			else
			{
				// no polygon offset - back to normal z bias
				// (see comment in 
				m_pD3DDev->SetRenderState(D3DRS_ZBIAS, /*8*/0); //Marty - HACK for Xash3D
			}

			// we've switched polygon offset now
			m_PolyOffsetSwitched = true;
		}
		
		// Check fog
		if (!m_bFogSwitched)
		{
			if (m_bFogEnabled)
			{
				m_pD3DDev->SetVertexShader( D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1 );
				m_pD3DDev->SetRenderState( D3DRS_RANGEFOGENABLE, TRUE );
				m_pD3DDev->SetRenderState(D3DRS_FOGENABLE, true);
			}
			else
			{
				m_pD3DDev->SetRenderState( D3DRS_RANGEFOGENABLE, FALSE );
				m_pD3DDev->SetRenderState(D3DRS_FOGENABLE, false);
			}
			m_bFogSwitched = true;
		}

		if ( m_glCullStateDirty ) 
		{
			m_glCullStateDirty = false;
			D3DCULL cull = D3DCULL_NONE;
			if ( m_glCullFace ) 
			{
				if (m_glCullFrontFaceMode == GL_CCW)
				{				
					switch(m_glCullFaceMode)
					{
						default:
						case GL_BACK:
							cull = D3DCULL_CW;
							break;

						case GL_FRONT:
							cull = D3DCULL_CCW;
							break;
					}
				}
				else if (m_glCullFrontFaceMode == GL_CW)
				{
					switch(m_glCullFaceMode)
					{
						default:
						case GL_BACK:
							cull = D3DCULL_CCW;
							break;

						case GL_FRONT:
							cull = D3DCULL_CW;
							break;
					}
				}
			}

			hr = m_pD3DDev->SetRenderState(D3DRS_CULLMODE, cull);
			if ( FAILED(hr) )
			{
				InterpretError(hr);
			}

		}
		if ( m_glShadeModelStateDirty )
		{
			m_glShadeModelStateDirty = false;
			// Shade model
			m_pD3DDev->SetRenderState( D3DRS_SHADEMODE, 
				m_glShadeModel == GL_SMOOTH ? D3DSHADE_GOURAUD : D3DSHADE_FLAT );
		}
			
		{
			m_textureState.SetTextureStageState(m_pD3DDev, &m_textures);
		}

		if ( m_glDepthStateDirty ) 
		{
			m_glDepthStateDirty = false;
			m_pD3DDev->SetRenderState( D3DRS_ZENABLE, m_glDepthTest ? D3DZB_TRUE : D3DZB_FALSE);
			m_pD3DDev->SetRenderState( D3DRS_ZWRITEENABLE, m_glDepthMask ? TRUE : FALSE);
			DWORD zfunc = GLToDXCompare(m_glDepthFunc);
			m_pD3DDev->SetRenderState( D3DRS_ZFUNC, zfunc );
		}
		if ( m_modelViewMatrixStateDirty ) 
		{
			m_modelViewMatrixStateDirty = false;
			m_pD3DDev->SetTransform( D3DTS_WORLD, m_modelViewMatrixStack->GetTop() );
		}
		if ( m_viewMatrixStateDirty ) 
		{
			m_viewMatrixStateDirty = false;
			m_pD3DDev->SetTransform( D3DTS_VIEW, & m_d3dViewMatrix );
		}
		if ( m_projectionMatrixStateDirty ) 
		{
			m_projectionMatrixStateDirty = false;
			m_pD3DDev->SetTransform( D3DTS_PROJECTION, m_projectionMatrixStack->GetTop() );
		}
		if ( m_textureMatrixStateDirty )
		{
			m_textureMatrixStateDirty = false;
			m_pD3DDev->SetTransform( D3DTS_TEXTURE0, m_textureMatrixStack->GetTop() );
		}
		if ( m_bViewPortDirty )
		{
			m_bViewPortDirty = false;
			D3DVIEWPORT8 viewData;

			viewData.X = m_glViewPortX;
			viewData.Y = m_glViewPortY;//m_windowHeight - (m_glViewPortY + m_glViewPortHeight);
			viewData.Width  = m_glViewPortWidth;
			viewData.Height = m_glViewPortHeight;
			viewData.MinZ = m_glDepthRangeNear;     
			viewData.MaxZ = m_glDepthRangeFar;
			m_pD3DDev->SetViewport(&viewData);
		}
	}

	void EnsureDriverInfo() 
	{
		if ( ! m_vendor ) 
		{
			m_pD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &m_dddi);
			m_vendor = m_dddi.Driver;
			m_renderer = m_dddi.Description;
			wsprintf(m_version, "%u.%u.%u.%u %u.%u.%u.%u %u", 
				HIWORD(m_dddi.DriverVersion.HighPart),
				LOWORD(m_dddi.DriverVersion.HighPart),
				HIWORD(m_dddi.DriverVersion.LowPart),
				LOWORD(m_dddi.DriverVersion.LowPart),
				m_dddi.VendorId,
				m_dddi.DeviceId,
				m_dddi.SubSysId,
				m_dddi.Revision,
				m_dddi.WHQLLevel
				);
			if ( m_textureState.GetMaxStages() > 1 ) 
			{
				m_extensions = " GL_SGIS_multitexture GL_EXT_texture_object ";
			}
			else 
			{
				m_extensions = " GL_EXT_texture_object ";
			}
		}
	}

	DWORD D3DFloatToDWORD (float f)
	{
		return ((DWORD *) &f)[0];
	}

	DWORD GLColorToD3D (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
	{
		return D3DCOLOR_ARGB
		(
			BYTE_CLAMP (alpha * 255.0f),
			BYTE_CLAMP (red * 255.0f),
			BYTE_CLAMP (green * 255.0f),
			BYTE_CLAMP (blue * 255.0f)
		);
	}

	D3DFORMAT GLToDXPixelFormat(GLint internalformat, GLenum format)
	{
		D3DFORMAT d3dFormat = D3DFMT_UNKNOWN;
		if ( g_force16bitTextures ) 
		{
			switch ( format ) 
			{
			case GL_RGBA:
				switch ( internalformat ) 
				{
				default:
				case 4:
//					d3dFormat = D3DFMT_A1R5G5B5; break;
					d3dFormat = D3DFMT_A4R4G4B4; break;
				case 3:
					d3dFormat = D3DFMT_R5G6B5; break;
				}
				break;
			case GL_COLOR_INDEX: d3dFormat = D3DFMT_P8; break;
			case GL_LUMINANCE: d3dFormat = D3DFMT_L8; break;
			case GL_ALPHA: d3dFormat = D3DFMT_A8; break;
			case GL_INTENSITY: d3dFormat = D3DFMT_L8; break;
			case GL_RGBA4: d3dFormat = D3DFMT_A4R4G4B4; break;
			default:
				InterpretError(E_FAIL);
			}
		}
		else 
		{
			// for
			switch ( format ) 
			{
			case GL_RGBA:
				switch ( internalformat ) 
				{
				default:
				case 4:
					d3dFormat = D3DFMT_A8R8G8B8; break;
				case 3:
					d3dFormat = D3DFMT_X8R8G8B8; break;
				}
				break;
			case GL_COLOR_INDEX: d3dFormat = D3DFMT_P8; break;
			case GL_LUMINANCE: d3dFormat = D3DFMT_L8; break;
			case GL_ALPHA: d3dFormat = D3DFMT_A8; break;
			case GL_INTENSITY: d3dFormat = D3DFMT_L8; break;
			case GL_RGBA4: d3dFormat = D3DFMT_A4R4G4B4; break;
			default:
				InterpretError(E_FAIL);
			}
		}
		return d3dFormat;
	}

// Avoid warning 4061, enumerant 'foo' in switch of enum 'bar' is not explicitly handled by a case label.
#pragma warning( push )
#pragma warning( disable : 4061)

		HRESULT ConvertToCompatablePixels(GLint internalformat,
		GLsizei width, GLsizei height,
		GLenum /* format */, GLenum type,
		D3DFORMAT dxPixelFormat,
		const GLvoid *pixels, char**  compatablePixels,
		DWORD* newPitch){
		HRESULT hr = S_OK;
		if ( type != GL_UNSIGNED_BYTE ) 
		{
			return E_FAIL;
		}
		switch ( dxPixelFormat )
		{
		default:
			LocalDebugBreak();
			break;
		case D3DFMT_P8:
		case D3DFMT_L8:
		case D3DFMT_A8:
			{
				char* copy = StickyAlloc(width*height);
				memcpy(copy,pixels,width * height);
				*compatablePixels = copy;
				if ( newPitch )
					*newPitch = width;
			}
			break;
		case D3DFMT_A4R4G4B4:
			{
				int textureElementSize = 2;
				const unsigned char* glpixels = (const unsigned char*) pixels;
				char* dxpixels = StickyAlloc(textureElementSize * width * height);
				switch ( internalformat )
				{
				default:
					LocalDebugBreak();
					break;
				case 1:
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x++)
							{
								unsigned short* dp = (unsigned short*) (dxpixels + (y*width+x)*textureElementSize);
								const unsigned char* sp = glpixels + (y*width+x);
								unsigned short v;
								unsigned short s = 0xf & (sp[0] >> 4);
								v = s; // blue
								v |= s << 4; // green
								v |= s << 8; // red
								v |= s << 12; // alpha
								*dp = v;
							}
						}
					}
					break;
				case 3:
				case GL_RGB: //MARTY
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x++)
							{
								unsigned short* dp = (unsigned short*) (dxpixels + (y*width+x)*textureElementSize);
								const unsigned char* sp = glpixels + (y*width+x)*4;
								unsigned short v;
								v = (0xf & (sp[2] >> 4)); // blue
								v |= (0xf & (sp[1] >> 4)) << 4; // green
								v |= (0xf & (sp[0] >> 4)) << 8; // red
								v |= 0xf000; // alpha
								*dp = v;
							}
						}
					}
					break;
				case 4:
				case GL_RGBA: //MARTY
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x++)
							{
								unsigned short* dp = (unsigned short*)(dxpixels + (y*width+x)*textureElementSize);
								const unsigned char* sp = glpixels + (y*width+x)*4;
								unsigned short v;
								v = (0xf & (sp[2] >> 4)); // blue
								v |= (0xf & (sp[1] >> 4)) << 4; // green
								v |= (0xf & (sp[0] >> 4)) << 8; // red
								v |= (0xf & (sp[3] >> 4)) << 12; // alpha
								*dp = v;
							}
						}
					}
					break;
				}
				*compatablePixels = dxpixels;
				if ( newPitch )
					*newPitch = 2 * width;
			}
			break;
		case D3DFMT_R5G6B5:
			{
				int textureElementSize = 2;
				const char* glpixels = (const char*) pixels;
				char* dxpixels = StickyAlloc(textureElementSize * width * height);
				switch ( internalformat ) 
				{
				default:
					LocalDebugBreak();
					break;
				case 1:
					{
						for(int y = 0; y < height; y++){
							for(int x = 0; x < width; x++){
								unsigned short* dp = (unsigned short*) (dxpixels + (y*width+x)*textureElementSize);
								const char* sp = glpixels + (y*width+x);
								unsigned short v;
								v = (0x1f & (sp[0] >> 3)); // blue
								v |= (0x3f & (sp[0] >> 2)) << 5; // green
								v |= (0x1f & (sp[0] >> 3)) << 11; // red
								*dp = v;
							}
						}
					}
					break;
				case 3:
				case GL_RGB: //MARTY
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x++)
							{
								unsigned short* dp = (unsigned short*) (dxpixels + (y*width+x)*textureElementSize);
								const char* sp = glpixels + (y*width+x)*4;
								unsigned short v;
								v = (0x1f & (sp[2] >> 3)); // blue
								v |= (0x3f & (sp[1] >> 2)) << 5; // green
								v |= (0x1f & (sp[0] >> 3)) << 11; // red
								*dp = v;
							}
						}
					}
					break;
				case 4:
				case GL_RGBA: //MARTY
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x++)
							{
								unsigned short* dp = (unsigned short*) (dxpixels + (y*width+x)*textureElementSize);
								const char* sp = glpixels + (y*width+x)*4;
								unsigned short v;
								v = (0x1f & (sp[2] >> 3)); // blue
								v |= (0x3f & (sp[1] >> 2)) << 5; // green
								v |= (0x1f & (sp[0] >> 3)) << 11; // red
								*dp = v;
							}
						}
					}
					break;
				}
				*compatablePixels = dxpixels;
				if ( newPitch )
					*newPitch = 2 * width;
			}
			break;
		case D3DFMT_X1R5G5B5:
			{
				int textureElementSize = 2;
				const char* glpixels = (const char*) pixels;
				char* dxpixels = StickyAlloc(textureElementSize * width * height);
				switch ( internalformat ) 
				{
				default:
					LocalDebugBreak();
					break;
				case 1:
					{
#define RGBTOR5G5B5(R, G, B) (0x8000 |  (0x1f & ((B) >> 3)) | ((0x1f & ((G) >> 3)) << 5) | ((0x1f & ((R) >> 3)) << 10))
#define Y5TOR5G5B5(Y) (0x8000 | ((Y) << 10) | ((Y) << 5) | (Y))
						static const unsigned short table[32] = {
							Y5TOR5G5B5(0), Y5TOR5G5B5(1), Y5TOR5G5B5(2), Y5TOR5G5B5(3),
							Y5TOR5G5B5(4), Y5TOR5G5B5(5), Y5TOR5G5B5(6), Y5TOR5G5B5(7),
							Y5TOR5G5B5(8), Y5TOR5G5B5(9), Y5TOR5G5B5(10), Y5TOR5G5B5(11),
							Y5TOR5G5B5(12), Y5TOR5G5B5(13), Y5TOR5G5B5(14), Y5TOR5G5B5(15),
							Y5TOR5G5B5(16), Y5TOR5G5B5(17), Y5TOR5G5B5(18), Y5TOR5G5B5(19),
							Y5TOR5G5B5(20), Y5TOR5G5B5(21), Y5TOR5G5B5(22), Y5TOR5G5B5(23),
							Y5TOR5G5B5(24), Y5TOR5G5B5(25), Y5TOR5G5B5(26), Y5TOR5G5B5(27),
							Y5TOR5G5B5(28), Y5TOR5G5B5(29), Y5TOR5G5B5(30), Y5TOR5G5B5(31)
						};
						unsigned short* dp = (unsigned short*) dxpixels;
						const unsigned char* sp = (const unsigned char*) glpixels;
						int numPixels = height * width;
						int i = numPixels >> 2;
						while(i > 0) {
							*dp++ = table[(*sp++) >> 3];
							*dp++ = table[(*sp++) >> 3];
							*dp++ = table[(*sp++) >> 3];
							*dp++ = table[(*sp++) >> 3];
							--i;
						}

						i = numPixels & 3;
						while(i > 0) {
							*dp++ = table[(*sp++) >> 3];
							--i;
						}
					}
					break;
				case 3:
				case GL_RGB: //MARTY
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x++)
							{
								unsigned short* dp = (unsigned short*) (dxpixels + (y*width+x)*textureElementSize);
								const unsigned char* sp = (const unsigned char*) glpixels + (y*width+x)*4;
								unsigned short v;
								v = (sp[2] >> 3); // blue
								v |= (sp[1] >> 3) << 5; // green
								v |= (sp[0] >> 3) << 10; // red
								v |= 0x8000; // alpha
								*dp = v;
							}
						}
					}
					break;
				case 4:
				case GL_RGBA: //MARTY
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x++)
							{
								unsigned short* dp = (unsigned short*) (dxpixels + (y*width+x)*textureElementSize);
								const unsigned char* sp = (const unsigned char*) glpixels + (y*width+x)*4;
								unsigned short v;
								v = (sp[2] >> 3); // blue
								v |= (sp[1] >> 3) << 5; // green
								v |= (sp[0] >> 3) << 10; // red
								v |= 0x8000; // alpha
								*dp = v;
							}
						}
					}
					break;
				}
				*compatablePixels = dxpixels;
				if ( newPitch ) {
					*newPitch = 2 * width;
				}
			}
			break;
		case D3DFMT_A1R5G5B5:
			{
				int textureElementSize = 2;
				const char* glpixels = (const char*) pixels;
				char* dxpixels = StickyAlloc(textureElementSize * width * height);
				switch ( internalformat ) 
				{
				default:
					LocalDebugBreak();
					break;
				case 1:
					{
						for(int y = 0; y < height; y++){
							for(int x = 0; x < width; x++){
								unsigned short* dp = (unsigned short*) (dxpixels + (y*width+x)*textureElementSize);
								const char* sp = glpixels + (y*width+x);
								unsigned short v;
								v = (0x1f & (sp[0] >> 3)); // blue
								v |= (0x1f & (sp[0] >> 3)) << 5; // green
								v |= (0x1f & (sp[0] >> 3)) << 10; // red
								v |= (0x01 & (sp[0] >> 7)) << 15; // alpha
								*dp = v;
							}
						}
					}
					break;
				case 3:
				case GL_RGB: //MARTY
					{
						for(int y = 0; y < height; y++){
							for(int x = 0; x < width; x++){
								unsigned short* dp = (unsigned short*) (dxpixels + (y*width+x)*textureElementSize);
								const char* sp = glpixels + (y*width+x)*4;
								unsigned short v;
								v = (0x1f & (sp[2] >> 3)); // blue
								v |= (0x1f & (sp[1] >> 3)) << 5; // green
								v |= (0x1f & (sp[0] >> 3)) << 10; // red
								v |= 0x8000; // alpha
								*dp = v;
							}
						}
					}
					break;
				case 4:
				case GL_RGBA: //MARTY
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x++)
							{
								unsigned short* dp = (unsigned short*) (dxpixels + (y*width+x)*textureElementSize);
								const char* sp = glpixels + (y*width+x)*4;
								unsigned short v;
								v = (0x1f & (sp[2] >> 3)); // blue
								v |= (0x1f & (sp[1] >> 3)) << 5; // green
								v |= (0x1f & (sp[0] >> 3)) << 10; // red
								v |= (0x01 & (sp[3] >> 7)) << 15; // alpha
								*dp = v;
							}
						}
					}
					break;
				}
				*compatablePixels = dxpixels;
				if ( newPitch ) {
					*newPitch = 2 * width;
				}
			}
			break;
		case D3DFMT_X8R8G8B8:
		case D3DFMT_A8R8G8B8:
			{
				int textureElementSize = 4;
				const char* glpixels = (const char*) pixels;
				char* dxpixels = StickyAlloc(textureElementSize * width * height);
				switch ( internalformat )
				{
				default:
					LocalDebugBreak();
					break;
				case 1:
					{
						for(int y = 0; y < height; y++){
							for(int x = 0; x < width; x++){
								char* dp = dxpixels + (y*width+x)*textureElementSize;
								const char* sp = glpixels + (y*width+x);
								dp[0] = sp[0]; // blue
								dp[1] = sp[0]; // green
								dp[2] = sp[0]; // red
								dp[3] = sp[0];
							}
						}
					}
					break;
				case 3:
				case GL_RGB: //MARTY
				case GL_RGB8: //MARTY
					{
						for(int y = 0; y < height; y++){
							for(int x = 0; x < width; x++){
								unsigned char* dp = (unsigned char*) dxpixels + (y*width+x)*textureElementSize;
								const unsigned char* sp = (unsigned char*) glpixels + (y*width+x)*4;
								dp[0] = sp[2]; // blue
								dp[1] = sp[1]; // green
								dp[2] = sp[0]; // red
								dp[3] = 0xff;
							}
						}
					}
					break;
				case 4:
				case GL_RGBA: //MARTY
				case GL_RGBA8: //MARTY
					{
						for(int y = 0; y < height; y++)
						{
							for(int x = 0; x < width; x++)
							{
								char* dp = dxpixels + (y*width+x)*textureElementSize;
								const char* sp = glpixels + (y*width+x)*4;
								dp[0] = sp[2]; // blue
								dp[1] = sp[1]; // green
								dp[2] = sp[0]; // red
								dp[3] = sp[3]; // alpha
							}
						}
					}
					break;
				}
				*compatablePixels = dxpixels;
				if ( newPitch ) 
					*newPitch = 4 * width;
			}
		}
		return hr;
	}

#pragma warning( pop )
};

void /*APIENTRY*/ glAlphaFunc (GLenum func, GLclampf ref)
{
	gFakeGL->glAlphaFunc(func, ref);
}

void /*APIENTRY*/ glBegin (GLenum mode)
{
	gFakeGL->glBegin(mode);
}

void /*APIENTRY*/ glBlendFunc (GLenum sfactor, GLenum dfactor)
{
	gFakeGL->glBlendFunc(sfactor, dfactor);
}

void /*APIENTRY*/ glClear (GLbitfield mask)
{
	gFakeGL->glClear(mask);
}

void /*APIENTRY*/ glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	gFakeGL->glClearColor(red, green, blue, alpha);
}

void /*APIENTRY*/ glColor3f (GLfloat red, GLfloat green, GLfloat blue)
{
	gFakeGL->glColor3f(red, green, blue);
}

void /*APIENTRY*/ glColor3ubv (const GLubyte *v)
{
	gFakeGL->glColor3ubv(v);
}

void /*APIENTRY*/ glColor4ubv (const GLubyte *v)
{
	gFakeGL->glColor4ubv(v);
}

void /*APIENTRY*/ glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	gFakeGL->glColor4f(red, green, blue, alpha);
}

void /*APIENTRY*/ glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	gFakeGL->glColor4ub(red, green, blue, alpha);
}

void /*APIENTRY*/ glColor4fv (const GLfloat *v)
{
	gFakeGL->glColor4fv(v);
}

void /*APIENTRY*/ glPointSize (const GLfloat v)
{
	gFakeGL->glPointSize(v);
}

void /*APIENTRY*/ glCullFace (GLenum mode)
{
	gFakeGL->glCullFace(mode);
}

void /*APIENTRY*/ glDepthFunc (GLenum func)
{
	gFakeGL->glDepthFunc(func);
}

void /*APIENTRY*/ glDepthMask (GLboolean flag)
{
	gFakeGL->glDepthMask(flag);
}

void /*APIENTRY*/ glDepthRange (GLclampd zNear, GLclampd zFar)
{
	gFakeGL->glDepthRange(zNear, zFar);
}

void /*APIENTRY*/ glDrawBuffer (GLenum mode)
{
	gFakeGL->glDrawBuffer(mode);
}

void /*APIENTRY*/ glEnable (GLenum cap)
{
	gFakeGL->glEnable(cap);
}

void /*APIENTRY*/ glDisable (GLenum cap)
{
	gFakeGL->glDisable(cap);
}

void /*APIENTRY*/ glEnd (void)
{
	return; // Does nothing
//	gFakeGL->glEnd();
}

void /*APIENTRY*/ glFinish (void)
{
	gFakeGL->glFinish();
}

void /*APIENTRY*/ glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	gFakeGL->glFrustum(left, right, bottom, top, zNear, zFar);
}

void /*APIENTRY*/ glGetFloatv (GLenum pname, GLfloat *params)
{
	gFakeGL->glGetFloatv(pname, params);
}

const GLubyte* /*APIENTRY*/ glGetString (GLenum name)
{
	return gFakeGL->glGetString(name);
}

void /*APIENTRY*/ glHint (GLenum target, GLenum mode)
{
	gFakeGL->glHint(target, mode);
}

GLboolean /*APIENTRY*/ glIsEnabled(GLenum cap)
{
	return gFakeGL->glIsEnabled(cap);
}

void /*APIENTRY*/ glLoadIdentity (void)
{
	gFakeGL->glLoadIdentity();
}

void /*APIENTRY*/ glLoadMatrixf (const GLfloat *m)
{
	gFakeGL->glLoadMatrixf(m);
}

void /*APIENTRY*/ glMatrixMode (GLenum mode)
{
	gFakeGL->glMatrixMode(mode);
}

void /*APIENTRY*/  glOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	gFakeGL->glOrtho(left, right, top, bottom, zNear, zFar);
}

void /*APIENTRY*/ glPolygonMode (GLenum face, GLenum mode)
{
	gFakeGL->glPolygonMode(face, mode);
}

void /*APIENTRY*/ glPopMatrix (void)
{
	gFakeGL->glPopMatrix();
}

void /*APIENTRY*/ glPushMatrix (void)
{
	gFakeGL->glPushMatrix();
}

void /*APIENTRY*/ glReadBuffer (GLenum mode)
{
	gFakeGL->glReadBuffer(mode);
}

void /*APIENTRY*/glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	gFakeGL->glReadPixels(x, y, width, height, format, type, pixels);
}

void /*APIENTRY*/ glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	gFakeGL->glRotatef(angle, x, y, z);
}

void /*APIENTRY*/ glScalef (GLfloat x, GLfloat y, GLfloat z)
{
	gFakeGL->glScalef(x, y, z);
}

void /*APIENTRY*/ glShadeModel (GLenum mode)
{
	gFakeGL->glShadeModel(mode);
}

void /*APIENTRY*/ glTexCoord2f (GLfloat s, GLfloat t)
{
	gFakeGL->glTexCoord2f(s, t);
}

void /*APIENTRY*/ glTexEnvf (GLenum target, GLenum pname, GLfloat param)
{
	gFakeGL->glTexEnvf(target, pname, param);
}

void /*APIENTRY*/ glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	gFakeGL->glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels, 0);
}

void /*APIENTRY*/ glTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
	gFakeGL->glTexParameterf(target, pname, param);
}

void /*APIENTRY*/ glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	gFakeGL->glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void /*APIENTRY*/ glTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
	gFakeGL->glTranslatef(x, y, z);
}

void /*APIENTRY*/ glVertex2f (GLfloat x, GLfloat y)
{
	gFakeGL->glVertex2f(x, y);
}

void /*APIENTRY*/ glVertex3f (GLfloat x, GLfloat y, GLfloat z)
{
	gFakeGL->glVertex3f(x, y, z);
}

void /*APIENTRY*/ glVertex3fv (const GLfloat *v)
{
	gFakeGL->glVertex3fv(v);
}

void /*APIENTRY*/ glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
	gFakeGL->glViewport(x, y, width, height);
}

HGLRC /*APIENTRY*/ wglCreateContext(/*maindc*/)
{
	gFakeGL = new FakeGL(/*mainwindow*/);

	//We don't return a handle on XBox
	if(!gFakeGL)
		return (HGLRC)0; 

	return (HGLRC)1;
}

BOOL /*WINAPI*/ wglDeleteContext(/*HGLRC hglrc*/)
{
	FakeGL* pFakeGL = gFakeGL;//(FakeGL*) hglrc;
	
	if(pFakeGL)
	{
		pFakeGL->wglDeleteContext();

		delete pFakeGL;
	
		pFakeGL = NULL;
		return TRUE;
	}
	return FALSE;
}

//MARTY
/*
HGLRC WINAPI wglGetCurrentContext(VOID)
{
	return gHGLRC;
}

HDC   WINAPI wglGetCurrentDC(VOID)
{ 
	return gHDC;
}
*/

void /*APIENTRY*/ glBindTexture(GLenum target, GLuint texture)
{
	gFakeGL->glBindTexture(target, texture);
}

static void /*APIENTRY*/ BindTextureExt(GLenum target, GLuint texture)
{
	gFakeGL->glBindTexture(target, texture);
}

static void /*APIENTRY*/ MTexCoord2fSGIS(GLenum target, GLfloat s, GLfloat t)
{
	gFakeGL->glMTexCoord2fSGIS(target, s, t);
}

static void /*APIENTRY*/ SelectTextureSGIS(GLenum target)
{
	gFakeGL->glSelectTextureSGIS(target);
}

// type cast unsafe conversion from 
#pragma warning( push )
#pragma warning( disable : 4191)

PROC /*APIENTRY*/ wglGetProcAddress(LPCSTR s)
{
	static LPCSTR kBindTextureEXT = "glBindTextureEXT";
	static LPCSTR kMTexCoord2fSGIS = "glMTexCoord2fSGIS"; // Multitexture
	static LPCSTR kSelectTextureSGIS = "glSelectTextureSGIS";

	if ( strncmp(s, kBindTextureEXT, sizeof(kBindTextureEXT)-1) == 0)
	{
		return (PROC) BindTextureExt;
	}
	else if ( strncmp(s, kMTexCoord2fSGIS, sizeof(kMTexCoord2fSGIS)-1) == 0)
	{
		return (PROC) MTexCoord2fSGIS;
	}
	else if ( strncmp(s, kSelectTextureSGIS, sizeof(kSelectTextureSGIS)-1) == 0)
	{
		return (PROC) SelectTextureSGIS;
	}
	// LocalDebugBreak();
	return 0;
}

#pragma warning( pop )

BOOL /*WINAPI*/ wglMakeCurrent(/*HDC hdc, HGLRC hglrc*/)
{
	/* 
	gHDC = hdc;
	gHGLRC = hglrc;
	gFakeGL = (FakeGL*) hglrc;
	*/
	if(!gFakeGL)
		return FALSE;

	return TRUE;
}

int d3dSetMode(int width, int height/*, int bpp, int zbpp*/, int vmode)
{
	gWidth = width;
	gHeight = height;
	gVideoMode = vmode;

	// Check if HD is possible if selected
	if(gWidth >= 1280 && gHeight >= 720) 
	{
		if(!(XGetAVPack() == XC_AV_PACK_HDTV) || !(XGetVideoFlags() & XC_VIDEO_FLAGS_HDTV_720p))
			return DISP_CHANGE_FAILED;
	}

	return DISP_CHANGE_SUCCESSFUL;
}

void FGL01_SetSubTexture()
{
	gFakeGL->SetSubTexture();
}

BOOL /*WINAPI*/ FakeSwapBuffers()
{
	if (!gFakeGL)
		return FALSE;

	gFakeGL->SwapBuffers();

	return TRUE;
}

void d3dSetGammaRamp(const unsigned char* gammaTable)
{
	gFakeGL->SetGammaRamp(gammaTable);
}

void d3dInitSetForce16BitTextures(int force16bitTextures)
{
	// called before gFakeGL exits. That's why we set a global
	g_force16bitTextures = force16bitTextures != 0; 
}

void d3dHint_GenerateMipMaps(int value)
{
	gFakeGL->Hint_GenerateMipMaps(value);
}

float d3dGetD3DDriverVersion()
{
	return 0.81f;
}

LPDIRECT3DDEVICE8 d3dGetDevice()
{
	return gFakeGL->GetD3DDevice();
}

/*
===================================================================================================================

New Fucntions (only what was missing for Xash3D) - Marty 12/3/17

Used FakeGL09 as reference for how to do things.
Made to do exactly the same things they do in FakeGL09, in some cases nothing.

===================================================================================================================
*/

void /*APIENTRY*/ glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	gFakeGL->glColorMask(red, green, blue, alpha);
}

void /*APIENTRY*/ glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
	// d3d8 doesnt have scissor test so we'll just live without it for now.
}

void /*APIENTRY*/ glTexEnvi (GLenum target, GLenum pname, GLint param)
{
	if (target != GL_TEXTURE_ENV) return;

	glTexEnvf (target, pname, param);
}

void /*APIENTRY*/ glTexParameteri (GLenum target, GLenum pname, GLint param)
{
	if (target != GL_TEXTURE_2D) return;

	glTexParameterf (target, pname, param);
}

void /*APIENTRY*/ glLineWidth (GLfloat width)
{
	//Not implemented - Same as FakeGL09
}

void /*APIENTRY*/ glFogi (GLenum pname, GLint param)
{
	gFakeGL->glFogi(pname, param);
}

void /*APIENTRY*/ glFogf (GLenum pname, GLfloat param)
{
	gFakeGL->glFogf(pname, param);
}

void /*APIENTRY*/ glFogfv (GLenum pname, const GLfloat *params)
{
	gFakeGL->glFogfv(pname, params);
}

void /*APIENTRY*/ glCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	//Not implemented - Same as FakeGL09
}

void /*APIENTRY*/ glFrontFace (GLenum mode)
{
	gFakeGL->glFrontFace(mode);
}

void /*APIENTRY*/ glPolygonOffset (GLfloat factor, GLfloat units)
{
	gFakeGL->glPolygonOffset( factor, units);
}

void /*APIENTRY*/ glDeleteTextures (GLsizei n, const GLuint *textures)
{
	gFakeGL->glDeleteTextures( n, textures);
}

void /*APIENTRY*/ glStencilFunc (GLenum func, GLint ref, GLuint mask)
{
	//Not implemented - Same as FakeGL09
}

void /*APIENTRY*/ glStencilOp (GLenum fail, GLenum zfail, GLenum zpass)
{
	//Not implemented - Same as FakeGL09
}

void /*APIENTRY*/ glGetIntegerv (GLenum pname, GLint *params)
{
	gFakeGL->glGetIntegerv(pname, params);
}

GLenum /*APIENTRY*/ glGetError()
{
	return gFakeGL->glGetError();
}

#endif //_USEFAKEGL01