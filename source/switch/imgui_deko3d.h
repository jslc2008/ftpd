#pragma once

namespace imgui
{
namespace deko3d
{
/// \brief Initialize deko3d
void init ();
/// \brief Deinitialize deko3d
void exit ();

/// \brief Prepare deko3d for a new frame
void newFrame ();
/// \brief Render ImGui draw list
void render ();

/// \brief Draw deko3d logo
/// \todo move into client code
void test ();
}
}
