// Copyright 2024-2026 Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the misc/licenses/gplv2.txt file included.

#pragma once

#include <string>

namespace Common {

constexpr char help_string[] =
    "Usage: {} [options] <file path>\n"
    "-c  [path]                  Z3DS compress a ROM located at the given path\n"
    "                              (optionally provide '-o [path]' for output directory)\n"
    "-d, --dump-video [path]     Dump video recording of emulator playback to the given file path\n"
    "-f, --fullscreen            Start in fullscreen mode\n"
    "-g, --gdbport [port]        Enable gdb stub on the given port\n"
    "-h, --help                  Display this help and exit\n"
    "    --hide-ui               Hide menu bar and status bar on startup (toggle with F12)\n"
    "-i, --install [path]        Install a CIA file at the given path\n"
    "-m, --multiplayer [nick[:password]@address[:port]]\n"
    "                            Connect to a multiplayer room on startup\n"
    "-p, --movie-play [path]     Play a TAS movie located at the given path\n"
    "-r, --movie-record [path]   Record a TAS movie to the given file path\n"
    "-a, --movie-record-author [author]   Set the author for the recorded TAS movie (to be used "
    "alongside --movie-record)\n"
#ifdef ENABLE_ROOM
    "    --room                  Utilize dedicated multiplayer room functionality (equivalent to "
    "the old citra-room executable)\n"
#endif
    "    --room-address [host]   Multiplayer room host/IP address to connect to on startup\n"
    "    --room-port [port]      Multiplayer room port (default: 24872)\n"
    "    --room-nickname [name]  Nickname for multiplayer room (default: 3DS system username)\n"
    "    --room-password [pass]  Password for multiplayer room (optional)\n"
    "-u, --user [path]           Specify a custom user directory path\n"
    "-v, --version               Output version information and exit\n"
    "-w, --windowed              Start in windowed mode\n"
    "-x  [path]                  Decompress a Z3DS compressed ROM located at the given path\n"
    "                              (optionally provide '-o [path]' for output directory)";

} // namespace Common
