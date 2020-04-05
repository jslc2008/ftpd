#include "platform.h"

#include "ftpServer.h"

#include "imgui.h"

#include <cstdio>
#include <cstdlib>

int main (int argc_, char *argv_[])
{
	if (!platform::init ())
		return EXIT_FAILURE;

	auto &style          = ImGui::GetStyle ();
	style.WindowRounding = 0.0f;

#ifdef _3DS
	style.Colors[ImGuiCol_WindowBg].w = 0.5f;
#endif

	auto server = FtpServer::create (5000);

	while (platform::loop ())
	{
		server->draw ();

		platform::render ();
	}

	platform::exit ();
}
