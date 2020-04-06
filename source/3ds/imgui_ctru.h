#pragma once

namespace imgui
{
namespace ctru
{
/// \brief Initialize 3ds platform
bool init ();
/// \brief Deinitialize 3ds platform
void exit ();

/// \brief Prepare 3ds for a new frame
void newFrame ();
}
}
