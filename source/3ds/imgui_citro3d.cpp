#include "imgui_citro3d.h"

#include <citro3d.h>

#include "vshader_shbin.h"

#include "imgui.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace
{
/// \brief 3DS font glyph ranges
std::vector<ImWchar> s_fontRanges;

/// \brief Clear color
constexpr auto CLEAR_COLOR = 0x204B7AFF;

/// \brief Display transfer flags
constexpr auto DISPLAY_TRANSFER_FLAGS =
    GX_TRANSFER_FLIP_VERT (0) | GX_TRANSFER_OUT_TILED (0) | GX_TRANSFER_RAW_COPY (0) |
    GX_TRANSFER_IN_FORMAT (GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT (GX_TRANSFER_FMT_RGB8) |
    GX_TRANSFER_SCALING (GX_TRANSFER_SCALE_NO);

/// \brief Top screen render target
C3D_RenderTarget *s_top = nullptr;
/// \brief Bottom screen render target
C3D_RenderTarget *s_bottom = nullptr;

/// \brief Vertex shader
DVLB_s *s_vsh = nullptr;
/// \brief Vertex shader program
shaderProgram_s s_program;

/// \brief Projection matrix uniform location
int s_projLocation;
/// \brief Top screen projection matrix
C3D_Mtx s_projTop;
/// \brief Bottom screen projection matrix
C3D_Mtx s_projBottom;

/// \brief System font textures
std::vector<C3D_Tex> s_fontTextures;
/// \brief Text scale
float s_textScale;

/// \brief Scissor test bounds
std::uint32_t s_boundScissor[4];
/// \brief Currently bound vertex data
ImDrawVert *s_boundVtxData;
/// \brief Currently bound texture
C3D_Tex *s_boundTexture;

/// \brief Vertex data buffer
ImDrawVert *s_vtxData = nullptr;
/// \brief Size of vertex data buffer
std::size_t s_vtxSize = 0;
/// \brief Index data buffer
ImDrawIdx *s_idxData = nullptr;
/// \brief Size of index data buffer
std::size_t s_idxSize = 0;

/// \brief Get code point from glyph index
/// \param font_ Font to search
/// \param glyphIndex_ Glyph index
std::uint32_t fontCodePointFromGlyphIndex (CFNT_s *const font_, int const glyphIndex_)
{
	for (auto cmap = fontGetInfo (font_)->cmap; cmap; cmap = cmap->next)
	{
		switch (cmap->mappingMethod)
		{
		case CMAP_TYPE_DIRECT:
			assert (cmap->codeEnd >= cmap->codeBegin);
			if (glyphIndex_ >= cmap->indexOffset &&
			    glyphIndex_ <= cmap->codeEnd - cmap->codeBegin + cmap->indexOffset)
				return glyphIndex_ - cmap->indexOffset + cmap->codeBegin;
			break;

		case CMAP_TYPE_TABLE:
			for (int i = 0; i <= cmap->codeEnd - cmap->codeBegin; ++i)
			{
				if (cmap->indexTable[i] == glyphIndex_)
					return cmap->codeBegin + i;
			}
			break;

		case CMAP_TYPE_SCAN:
			for (unsigned i = 0; i < cmap->nScanEntries; ++i)
			{
				assert (cmap->scanEntries[i].code >= cmap->codeBegin);
				assert (cmap->scanEntries[i].code <= cmap->codeEnd);
				if (glyphIndex_ == cmap->scanEntries[i].glyphIndex)
					return cmap->scanEntries[i].code;
			}
			break;
		}
	}

	return 0;
}

/// \brief Setup render state
/// \param screen_ Whether top or bottom screen
void setupRenderState (gfxScreen_t const screen_)
{
	// disable face culling
	C3D_CullFace (GPU_CULL_NONE);

	// configure attributes for user with vertex shader
	auto const attrInfo = C3D_GetAttrInfo ();
	AttrInfo_Init (attrInfo);
	AttrInfo_AddLoader (attrInfo, 0, GPU_FLOAT, 2);         // v0 = inPos
	AttrInfo_AddLoader (attrInfo, 1, GPU_FLOAT, 2);         // v1 = inUv
	AttrInfo_AddLoader (attrInfo, 2, GPU_UNSIGNED_BYTE, 4); // v2 = inColor

	// clear bindings
	std::memset (s_boundScissor, 0xFF, sizeof (s_boundScissor));
	s_boundVtxData = nullptr;
	s_boundTexture = nullptr;

	// bind program
	C3D_BindProgram (&s_program);

	// enable depth test
	C3D_DepthTest (true, GPU_GREATER, GPU_WRITE_COLOR);

	// enable alpha blending
	C3D_AlphaBlend (GPU_BLEND_ADD,
	    GPU_BLEND_ADD,
	    GPU_SRC_ALPHA,
	    GPU_ONE_MINUS_SRC_ALPHA,
	    GPU_SRC_ALPHA,
	    GPU_ONE_MINUS_SRC_ALPHA);

	// apply projection matrix
	if (screen_ == GFX_TOP)
		C3D_FVUnifMtx4x4 (GPU_VERTEX_SHADER, s_projLocation, &s_projTop);
	else
		C3D_FVUnifMtx4x4 (GPU_VERTEX_SHADER, s_projLocation, &s_projBottom);
}
}

void imgui::citro3d::init ()
{
	// setup back-end capabilities flags
	ImGuiIO &io = ImGui::GetIO ();

	io.BackendRendererName = "citro3d";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	// initialize citro3d
	C3D_Init (C3D_DEFAULT_CMDBUF_SIZE);

	// create top screen render target
	s_top = C3D_RenderTargetCreate (240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput (s_top, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	// create bottom screen render target
	s_bottom = C3D_RenderTargetCreate (240, 320, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput (s_bottom, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	// load vertex shader
	s_vsh = DVLB_ParseFile (
	    const_cast<std::uint32_t *> (reinterpret_cast<std::uint32_t const *> (vshader_shbin)),
	    vshader_shbin_size);

	// initialize vertex shader program
	shaderProgramInit (&s_program);
	shaderProgramSetVsh (&s_program, &s_vsh->DVLE[0]);

	// get projection matrix uniform location
	s_projLocation = shaderInstanceGetUniformLocation (s_program.vertexShader, "proj");

	// initialize projection matrices
	Mtx_OrthoTilt (&s_projTop, 0.0f, 800.0f, 480.0f, 0.0f, -1.0f, 1.0f, false);
	Mtx_OrthoTilt (&s_projBottom, 80.0f, 720.0f, 960.0f, 480.0f, -1.0f, 1.0f, false);

	// allocate vertex data buffer
	s_vtxSize = 65536;
	s_vtxData = reinterpret_cast<ImDrawVert *> (linearAlloc (sizeof (ImDrawVert) * s_vtxSize));
	if (!s_vtxData)
		svcBreak (USERBREAK_PANIC);

	// allocate index data buffer
	s_idxSize = 65536;
	s_idxData = reinterpret_cast<ImDrawIdx *> (linearAlloc (sizeof (ImDrawIdx) * s_idxSize));
	if (!s_idxData)
		svcBreak (USERBREAK_PANIC);

	// ensure the shared system font is mapped
	if (R_FAILED (fontEnsureMapped ()))
		svcBreak (USERBREAK_PANIC);

	// load the glyph texture sheets
	auto const font      = fontGetSystemFont ();
	auto const fontInfo  = fontGetInfo (font);
	auto const glyphInfo = fontGetGlyphInfo (font);
	assert (s_fontTextures.empty ());
	s_fontTextures.resize (glyphInfo->nSheets + 1);
	std::memset (s_fontTextures.data (), 0x00, s_fontTextures.size () * sizeof (s_fontTextures[0]));

	s_textScale = 30.0f / glyphInfo->cellHeight;

	// use system font sheets as citro3d textures
	for (unsigned i = 0; i < glyphInfo->nSheets; ++i)
	{
		auto &tex = s_fontTextures[i];
		tex.data  = fontGetGlyphSheetTex (font, i);
		if (!tex.data)
			svcBreak (USERBREAK_PANIC);
		tex.fmt    = static_cast<GPU_TEXCOLOR> (glyphInfo->sheetFmt);
		tex.size   = glyphInfo->sheetSize;
		tex.width  = glyphInfo->sheetWidth;
		tex.height = glyphInfo->sheetHeight;
		tex.param  = GPU_TEXTURE_MAG_FILTER (GPU_LINEAR) | GPU_TEXTURE_MIN_FILTER (GPU_LINEAR) |
		            GPU_TEXTURE_WRAP_S (GPU_REPEAT) | GPU_TEXTURE_WRAP_T (GPU_REPEAT);
		tex.border   = 0xFFFFFFFF;
		tex.lodParam = 0;
	}

	{
		// create texture for ImGui's white pixel
		auto &tex = s_fontTextures[glyphInfo->nSheets];
		C3D_TexInit (&tex, 8, 8, GPU_A4);

		// fill texture with full alpha
		std::uint32_t size;
		auto data = C3D_Tex2DGetImagePtr (&tex, 0, &size);
		if (!data || !size)
			svcBreak (USERBREAK_PANIC);
		std::memset (data, 0xFF, size);
	}

	// get alternate character glyph
	ImWchar alterChar = fontCodePointFromGlyphIndex (font, fontInfo->alterCharIndex);
	if (!alterChar)
		alterChar = '?';

	// collect character map
	std::vector<ImWchar> charSet;
	for (auto cmap = fontInfo->cmap; cmap; cmap = cmap->next)
	{
		switch (cmap->mappingMethod)
		{
		case CMAP_TYPE_DIRECT:
		case CMAP_TYPE_TABLE:
			assert (cmap->codeEnd >= cmap->codeBegin);
			charSet.reserve (charSet.size () + cmap->codeEnd - cmap->codeBegin + 1);
			for (auto i = cmap->codeBegin; i <= cmap->codeEnd; ++i)
				charSet.emplace_back (i);
			break;
		case CMAP_TYPE_SCAN:
			charSet.reserve (charSet.size () + cmap->nScanEntries);
			for (unsigned i = 0; i < cmap->nScanEntries; ++i)
			{
				assert (cmap->scanEntries[i].code >= cmap->codeBegin);
				assert (cmap->scanEntries[i].code <= cmap->codeEnd);
				charSet.emplace_back (cmap->scanEntries[i].code);
			}
			break;
		}
	}

	if (charSet.empty ())
		svcBreak (USERBREAK_PANIC);

	// deduplicate character map
	std::sort (std::begin (charSet), std::end (charSet));
	charSet.erase (std::unique (std::begin (charSet), std::end (charSet)), std::end (charSet));

	// fill in font glyph ranges
	auto it       = std::begin (charSet);
	ImWchar start = *it++;
	ImWchar prev  = start;
	while (it != std::end (charSet))
	{
		if (*it != prev + 1)
		{
			s_fontRanges.emplace_back (start);
			s_fontRanges.emplace_back (prev);

			start = *it;
		}

		prev = *it++;
	}
	s_fontRanges.emplace_back (start);
	s_fontRanges.emplace_back (prev);

	// terminate glyph ranges
	s_fontRanges.emplace_back (0);

	// initialize font atlas
	auto const atlas = ImGui::GetIO ().Fonts;
	atlas->Clear ();
	atlas->TexWidth        = glyphInfo->sheetWidth;
	atlas->TexHeight       = glyphInfo->sheetHeight * glyphInfo->nSheets;
	atlas->TexUvScale      = ImVec2 (1.0f / atlas->TexWidth, 1.0f / atlas->TexHeight);
	atlas->TexUvWhitePixel = ImVec2 (0.5f / 8.0f, glyphInfo->nSheets + 0.5f / 8.0f);
	atlas->TexPixelsAlpha8 = static_cast<unsigned char *> (IM_ALLOC (1)); // dummy allocation

	// initialize font config
	ImFontConfig config;
	config.FontData             = nullptr;
	config.FontDataSize         = 0;
	config.FontDataOwnedByAtlas = true;
	config.FontNo               = 0;
	config.SizePixels           = 14.0f;
	config.OversampleH          = 3;
	config.OversampleV          = 1;
	config.PixelSnapH           = false;
	config.GlyphExtraSpacing    = ImVec2 (0.0f, 0.0f);
	config.GlyphOffset          = ImVec2 (0.0f, 0.0f);
	config.GlyphRanges          = s_fontRanges.data ();
	config.GlyphMinAdvanceX     = 0.0f;
	config.GlyphMaxAdvanceX     = std::numeric_limits<float>::max ();
	config.MergeMode            = false;
	config.RasterizerFlags      = 0;
	config.RasterizerMultiply   = 1.0f;
	config.EllipsisChar         = 0x2026;
	std::memset (config.Name, 0, sizeof (config.Name));

	// create font
	auto const imFont = IM_NEW (ImFont);
	config.DstFont    = imFont;

	// add config and font to atlas
	atlas->ConfigData.push_back (config);
	atlas->Fonts.push_back (imFont);
	atlas->SetTexID (s_fontTextures.data ());

	// initialize font metrics
	imFont->FallbackAdvanceX = fontInfo->defaultWidth.charWidth;
	imFont->FontSize         = fontInfo->lineFeed;

	// add glyphs to font
	fontGlyphPos_s glyphPos;
	for (auto const &code : charSet)
	{
		auto const glyphIndex = fontGlyphIndexFromCodePoint (font, code);
		if (glyphIndex < 0)
			svcBreak (USERBREAK_PANIC);

		// calculate glyph metrics
		fontCalcGlyphPos (&glyphPos,
		    font,
		    glyphIndex,
		    GLYPH_POS_CALC_VTXCOORD | GLYPH_POS_AT_BASELINE,
		    1.0f,
		    1.0f);

		// convert to ImGui font metrics
		ImFontGlyph glyph;
		glyph.Codepoint = code;
		glyph.AdvanceX  = glyphPos.xAdvance;
		glyph.X0        = glyphPos.vtxcoord.left;
		glyph.Y0        = glyphPos.vtxcoord.top;
		glyph.X1        = glyphPos.vtxcoord.right;
		glyph.Y1        = glyphPos.vtxcoord.bottom;
		glyph.U0        = glyphPos.texcoord.left;
		glyph.V0        = glyphPos.sheetIndex + glyphPos.texcoord.top;
		glyph.U1        = glyphPos.texcoord.right;
		glyph.V1        = glyphPos.sheetIndex + glyphPos.texcoord.bottom;

		// add glyph to font
		imFont->Glyphs.push_back (glyph);
		imFont->MetricsTotalSurface +=
		    static_cast<int> ((glyph.U1 - glyph.U0) * atlas->TexWidth + 1.99f) *
		    static_cast<int> ((glyph.V1 - glyph.V0) * atlas->TexHeight + 1.99f);
	}

	// build lookup table
	imFont->BuildLookupTable ();

	// finalize font
	imFont->DisplayOffset.x = 0.0f;
	imFont->DisplayOffset.y = fontInfo->ascent;
	imFont->ContainerAtlas  = atlas;
	imFont->ConfigData      = &atlas->ConfigData[0];
	imFont->ConfigDataCount = 1;
	imFont->FallbackChar    = alterChar;
	imFont->EllipsisChar    = config.EllipsisChar;
	imFont->Scale           = s_textScale;
	imFont->Ascent          = fontInfo->ascent;
	imFont->Descent         = 0.0f;
}

void imgui::citro3d::exit ()
{
	// free vertex/index data buffers
	linearFree (s_idxData);
	linearFree (s_vtxData);

	// delete ImGui white pixel texture
	assert (!s_fontTextures.empty ());
	C3D_TexDelete (&s_fontTextures.back ());

	// free shader program
	shaderProgramFree (&s_program);
	DVLB_Free (s_vsh);

	// free render targets
	C3D_RenderTargetDelete (s_bottom);
	C3D_RenderTargetDelete (s_top);

	// deinitialize citro3d
	C3D_Fini ();
}

void imgui::citro3d::newFrame ()
{
}

void imgui::citro3d::render ()
{
	C3D_FrameBegin (C3D_FRAME_SYNCDRAW);

	// clear frame/depth buffers
	C3D_RenderTargetClear (s_top, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetClear (s_bottom, C3D_CLEAR_ALL, CLEAR_COLOR, 0);

	// get draw data
	auto const drawData = ImGui::GetDrawData ();
	if (drawData->CmdListsCount <= 0)
	{
		C3D_FrameEnd (0);
		return;
	}

	// get framebuffer dimensions
	unsigned width  = drawData->DisplaySize.x * drawData->FramebufferScale.x;
	unsigned height = drawData->DisplaySize.y * drawData->FramebufferScale.y;
	if (width <= 0 || height <= 0)
	{
		C3D_FrameEnd (0);
		return;
	}

	// check if we need to grow vertex data buffer
	if (s_vtxSize < static_cast<std::size_t> (drawData->TotalVtxCount))
	{
		linearFree (s_vtxData);

		// add 10% to avoid growing many frames in a row
		s_vtxSize = drawData->TotalVtxCount * 1.1f;
		s_vtxData = reinterpret_cast<ImDrawVert *> (linearAlloc (sizeof (ImDrawVert) * s_vtxSize));
		if (!s_vtxData)
			svcBreak (USERBREAK_PANIC);
	}

	// check if we need to grow index data buffer
	if (s_idxSize < static_cast<std::size_t> (drawData->TotalIdxCount))
	{
		// add 10% to avoid growing many frames in a row
		s_idxSize = drawData->TotalIdxCount * 1.1f;
		s_idxData = reinterpret_cast<ImDrawIdx *> (linearAlloc (sizeof (ImDrawIdx) * s_idxSize));
		if (!s_vtxData)
			svcBreak (USERBREAK_PANIC);
	}

	// will project scissor/clipping rectangles into framebuffer space
	// (0,0) unless using multi-viewports
	auto const clipOff = drawData->DisplayPos;
	// (1,1) unless using retina display which are often (2,2)
	auto const clipScale = drawData->FramebufferScale;

	// copy data into vertex/index buffers
	std::size_t offsetVtx = 0;
	std::size_t offsetIdx = 0;
	for (int i = 0; i < drawData->CmdListsCount; ++i)
	{
		auto const &cmdList = *drawData->CmdLists[i];

		// double check that we don't overrun vertex/index data buffers
		if (s_vtxSize - offsetVtx < static_cast<std::size_t> (cmdList.VtxBuffer.Size))
			svcBreak (USERBREAK_PANIC);
		if (s_idxSize - offsetIdx < static_cast<std::size_t> (cmdList.IdxBuffer.Size))
			svcBreak (USERBREAK_PANIC);

		// copy vertex/index data into buffers
		std::memcpy (&s_vtxData[offsetVtx],
		    cmdList.VtxBuffer.Data,
		    sizeof (ImDrawVert) * cmdList.VtxBuffer.Size);
		std::memcpy (&s_idxData[offsetIdx],
		    cmdList.IdxBuffer.Data,
		    sizeof (ImDrawIdx) * cmdList.IdxBuffer.Size);

		offsetVtx += cmdList.VtxBuffer.Size;
		offsetIdx += cmdList.IdxBuffer.Size;
	}

	for (auto const &screen : {GFX_TOP, GFX_BOTTOM})
	{
		if (screen == GFX_TOP)
			C3D_FrameDrawOn (s_top);
		else
			C3D_FrameDrawOn (s_bottom);

		setupRenderState (screen);

		offsetVtx = 0;
		offsetIdx = 0;

		// render command lists
		for (int i = 0; i < drawData->CmdListsCount; ++i)
		{
			auto const &cmdList = *drawData->CmdLists[i];
			for (auto const &cmd : cmdList.CmdBuffer)
			{
				if (cmd.UserCallback)
				{
					// user callback, registered via ImDrawList::AddCallback()
					// (ImDrawCallback_ResetRenderState is a special callback value used by the user
					// to request the renderer to reset render state.)
					if (cmd.UserCallback == ImDrawCallback_ResetRenderState)
						setupRenderState (screen);
					else
						cmd.UserCallback (&cmdList, &cmd);
				}
				else
				{
					// project scissor/clipping rectangles into framebuffer space
					ImVec4 clip;
					clip.x = (cmd.ClipRect.x - clipOff.x) * clipScale.x;
					clip.y = (cmd.ClipRect.y - clipOff.y) * clipScale.y;
					clip.z = (cmd.ClipRect.z - clipOff.x) * clipScale.x;
					clip.w = (cmd.ClipRect.w - clipOff.y) * clipScale.y;

					if (clip.x >= width || clip.y >= height || clip.z < 0.0f || clip.w < 0.0f)
						continue;
					if (clip.x < 0.0f)
						clip.x = 0.0f;
					if (clip.y < 0.0f)
						clip.y = 0.0f;
					if (clip.z > width)
						clip.z = width;
					if (clip.w > height)
						clip.z = height;

					if (screen == GFX_TOP)
					{
						// check if clip starts on bottom screen
						if (clip.y > 240.0f)
							continue;

						// convert from framebuffer space to screen space (3DS screen rotation)
						auto const x1 = std::clamp<unsigned> (240.0f - clip.w, 0, 240);
						auto const y1 = std::clamp<unsigned> (400.0f - clip.z, 0, 400);
						auto const x2 = std::clamp<unsigned> (240.0f - clip.y, 0, 240);
						auto const y2 = std::clamp<unsigned> (400.0f - clip.x, 0, 400);

						// check if scissor needs to be updated
						if (s_boundScissor[0] != x1 || s_boundScissor[1] != y1 ||
						    s_boundScissor[2] != x2 || s_boundScissor[3] != y2)
						{
							s_boundScissor[0] = x1;
							s_boundScissor[1] = y1;
							s_boundScissor[2] = x2;
							s_boundScissor[3] = y2;
							C3D_SetScissor (GPU_SCISSOR_NORMAL, x1, y1, x2, y2);
						}
					}
					else
					{
						// check if clip ends on top screen
						if (clip.w < 240.0f)
							continue;

						// check if clip ends before left edge of bottom screen
						if (clip.z < 40.0f)
							continue;

						// check if clip starts after right edge of bottom screen
						if (clip.x > 360.0f)
							continue;

						// convert from framebuffer space to screen space
						// (3DS screen rotation + bottom screen offset)
						auto const x1 = std::clamp<unsigned> (480.0f - clip.w, 0, 240);
						auto const y1 = std::clamp<unsigned> (360.0f - clip.z, 0, 320);
						auto const x2 = std::clamp<unsigned> (480.0f - clip.y, 0, 240);
						auto const y2 = std::clamp<unsigned> (360.0f - clip.x, 0, 320);

						// check if scissor needs to be updated
						if (s_boundScissor[0] != x1 || s_boundScissor[1] != y1 ||
						    s_boundScissor[2] != x2 || s_boundScissor[3] != y2)
						{
							s_boundScissor[0] = x1;
							s_boundScissor[1] = y1;
							s_boundScissor[2] = x2;
							s_boundScissor[3] = y2;
							C3D_SetScissor (GPU_SCISSOR_NORMAL, x1, y1, x2, y2);
						}
					}

					// check if we need to update vertex data binding
					auto const vtxData = &s_vtxData[cmd.VtxOffset + offsetVtx];
					if (vtxData != s_boundVtxData)
					{
						s_boundVtxData     = vtxData;
						auto const bufInfo = C3D_GetBufInfo ();
						BufInfo_Init (bufInfo);
						BufInfo_Add (bufInfo, vtxData, sizeof (ImDrawVert), 3, 0x210);
					}

					// check if we need to update texture binding
					auto tex = static_cast<C3D_Tex *> (cmd.TextureId);
					if (tex == s_fontTextures.data ())
					{
						assert (cmd.ElemCount % 3 == 0);

						// get sheet number from uv coords
						auto const getSheet = [] (auto const vtx_, auto const idx_) {
							unsigned const sheet = std::min (
							    {vtx_[idx_[0]].uv.y, vtx_[idx_[1]].uv.y, vtx_[idx_[2]].uv.y});

							// assert that these three vertices use the same sheet
							for (unsigned i = 0; i < 3; ++i)
								assert (vtx_[idx_[i]].uv.y - sheet <= 1.0f);
							return sheet;
						};

						// initialize texture binding
						unsigned boundSheet = getSheet (&s_vtxData[cmd.VtxOffset + offsetVtx],
						    &s_idxData[cmd.IdxOffset + offsetIdx]);
						C3D_TexBind (0, &s_fontTextures[boundSheet]);

						unsigned offset = 0;

						// update texture environment for non-image drawing
						auto const env = C3D_GetTexEnv (0);
						C3D_TexEnvInit (env);
						C3D_TexEnvSrc (
						    env, C3D_RGB, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
						C3D_TexEnvFunc (env, C3D_RGB, GPU_REPLACE);
						C3D_TexEnvSrc (
						    env, C3D_Alpha, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
						C3D_TexEnvFunc (env, C3D_Alpha, GPU_MODULATE);

						// process one triangle at a time
						for (unsigned i = 3; i < cmd.ElemCount; i += 3)
						{
							// get sheet for this triangle
							unsigned const sheet = getSheet (&s_vtxData[cmd.VtxOffset + offsetVtx],
							    &s_idxData[cmd.IdxOffset + offsetIdx + i]);

							// check if we're changing textures
							if (boundSheet != sheet)
							{
								// draw everything up until now
								C3D_DrawElements (GPU_TRIANGLES,
								    i - offset,
								    C3D_UNSIGNED_SHORT,
								    &s_idxData[cmd.IdxOffset + offsetIdx + offset]);

								// bind texture for next draw call
								boundSheet = sheet;
								offset     = i;
								C3D_TexBind (0, &s_fontTextures[boundSheet]);
							}
						}

						// draw the final set of triangles
						assert ((cmd.ElemCount - offset) % 3 == 0);
						C3D_DrawElements (GPU_TRIANGLES,
						    cmd.ElemCount - offset,
						    C3D_UNSIGNED_SHORT,
						    &s_idxData[cmd.IdxOffset + offsetIdx + offset]);
					}
					else
					{
						// drawing an image; check if we need to change texture binding
						if (tex != s_boundTexture)
						{
							// bind new texture
							C3D_TexBind (0, tex);

							// update texture environment for drawing images
							auto const env = C3D_GetTexEnv (0);
							C3D_TexEnvInit (env);
							C3D_TexEnvSrc (
							    env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
							C3D_TexEnvFunc (env, C3D_Both, GPU_MODULATE);
						}

						// draw triangles
						C3D_DrawElements (GPU_TRIANGLES,
						    cmd.ElemCount,
						    C3D_UNSIGNED_SHORT,
						    &s_idxData[cmd.IdxOffset + offsetIdx]);
					}

					s_boundTexture = tex;
				}
			}

			offsetVtx += cmdList.VtxBuffer.Size;
			offsetIdx += cmdList.IdxBuffer.Size;
		}
	}

	C3D_FrameEnd (0);
}
