#include "imgui_ctru.h"

#include "imgui.h"

#include "fs.h"

#include <3ds.h>

#include <chrono>
#include <cstring>
#include <functional>
#include <string>
#include <tuple>
using namespace std::chrono_literals;

namespace
{
constexpr auto SCREEN_WIDTH  = 400.0f;
constexpr auto SCREEN_HEIGHT = 480.0f;

std::string s_clipboard;

char const *getClipboardText (void *const userData_)
{
	(void)userData_;
	return s_clipboard.c_str ();
}

void setClipboardText (void *const userData_, char const *const text_)
{
	(void)userData_;
	s_clipboard = text_;
}

void updateTouch (ImGuiIO &io_)
{
	if (!((hidKeysDown () | hidKeysHeld ()) & KEY_TOUCH))
	{
		io_.MousePos     = ImVec2 (-10.0f, -10.0f);
		io_.MouseDown[0] = false;
		return;
	}

	touchPosition pos;
	hidTouchRead (&pos);

	// transform to bottom-screen space
	io_.MousePos     = ImVec2 ((pos.px + 40.0f) * 2.0f, (pos.py + 240.0f) * 2.0f);
	io_.MouseDown[0] = true;
}

void updateGamepads (ImGuiIO &io_)
{
	std::memset (io_.NavInputs, 0, sizeof (io_.NavInputs));

	auto const buttonMapping = {
	    std::make_pair (KEY_A, ImGuiNavInput_Activate),
	    std::make_pair (KEY_B, ImGuiNavInput_Cancel),
	    std::make_pair (KEY_X, ImGuiNavInput_Input),
	    std::make_pair (KEY_Y, ImGuiNavInput_Menu),
	    std::make_pair (KEY_L, ImGuiNavInput_FocusPrev),
	    std::make_pair (KEY_L, ImGuiNavInput_TweakSlow),
	    std::make_pair (KEY_R, ImGuiNavInput_FocusNext),
	    std::make_pair (KEY_R, ImGuiNavInput_TweakFast),
	    std::make_pair (KEY_DUP, ImGuiNavInput_DpadUp),
	    std::make_pair (KEY_DRIGHT, ImGuiNavInput_DpadRight),
	    std::make_pair (KEY_DDOWN, ImGuiNavInput_DpadDown),
	    std::make_pair (KEY_DLEFT, ImGuiNavInput_DpadLeft),
	};

	auto const keys = hidKeysHeld ();
	for (auto const &[in, out] : buttonMapping)
	{
		if (keys & in)
			io_.NavInputs[out] = 1.0f;
	}

	circlePosition cpad;
	auto const analogMapping = {
	    std::make_tuple (std::ref (cpad.dx), ImGuiNavInput_LStickLeft, -0.3f, -0.9f),
	    std::make_tuple (std::ref (cpad.dx), ImGuiNavInput_LStickRight, +0.3f, +0.9f),
	    std::make_tuple (std::ref (cpad.dy), ImGuiNavInput_LStickUp, +0.3f, +0.9f),
	    std::make_tuple (std::ref (cpad.dy), ImGuiNavInput_LStickDown, -0.3f, -0.9f),
	};

	hidCircleRead (&cpad);
	for (auto const &[in, out, min, max] : analogMapping)
	{
		auto const value   = in / static_cast<float> (0x9C);
		auto const v       = std::min (1.0f, (value - min) / (max - min));
		io_.NavInputs[out] = std::max (io_.NavInputs[out], v);
	}
}
}

bool imgui::ctru::init ()
{
	ImGuiIO &io = ImGui::GetIO ();

	io.IniFilename = nullptr;

	io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

	io.BackendPlatformName = "3ds";

	io.MouseDrawCursor = false;

	io.SetClipboardTextFn = setClipboardText;
	io.GetClipboardTextFn = getClipboardText;
	io.ClipboardUserData  = nullptr;

	return true;
}

void imgui::ctru::newFrame ()
{
	ImGuiIO &io = ImGui::GetIO ();

	IM_ASSERT (io.Fonts->IsBuilt () &&
	           "Font atlas not built! It is generally built by the renderer back-end. Missing call "
	           "to renderer _NewFrame() function?");

	io.DisplaySize             = ImVec2 (SCREEN_WIDTH * 2.0f, SCREEN_HEIGHT * 2.0f);
	io.DisplayFramebufferScale = ImVec2 (0.5f, 0.5f);

	// Setup time step
	static auto const start = svcGetSystemTick ();
	static auto prev        = start;
	auto const now          = svcGetSystemTick ();

	io.DeltaTime = (now - prev) / static_cast<float> (SYSCLOCK_ARM11);
	prev         = now;

	updateTouch (io);
	updateGamepads (io);
}

void imgui::ctru::exit ()
{
}
