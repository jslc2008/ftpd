#include "platform.h"

#include "ftpServer.h"

#include "imgui.h"

#include <cstdio>
#include <cstdlib>

int main (int argc_, char *argv_[])
{
	IMGUI_CHECKVERSION ();
	ImGui::CreateContext ();

	if (!platform::init ())
	{
		ImGui::DestroyContext ();
		return EXIT_FAILURE;
	}

	auto &style          = ImGui::GetStyle ();
	style.WindowRounding = 0.0f;

#ifdef _3DS
	// citro3d logo doesn't quite show with the default transparency
	style.Colors[ImGuiCol_WindowBg].w = 0.8f;
#endif

	auto server = FtpServer::create (5000);

	while (platform::loop ())
	{
		server->draw ();

		platform::render ();
	}

	server.reset ();
	platform::exit ();
	ImGui::DestroyContext ();
}
