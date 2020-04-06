#pragma once

namespace imgui
{
namespace citro3d
{
/// \brief Initialize citro3d
void init ();
/// \brief Deinitialize citro3d
void exit ();

/// \brief Prepare citro3d for a new frame
void newFrame ();
/// \brief Render ImGui draw list
void render ();
}
}
