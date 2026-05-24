#pragma once
#include <string>

namespace LauncherCommon {

// Ищет путь к установленной игре Kenshi (Steam) через реестр Windows
// Возвращает путь типа "D:\SteamLibrary\steamapps\common\Kenshi" или пустую строку
std::wstring FindKenshiPath();

}