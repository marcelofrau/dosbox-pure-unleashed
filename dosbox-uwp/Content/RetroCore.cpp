#include "pch.h"
#include "RetroCore.h"
#include "libretro.h"
#include <vfs/vfs.h>
#include <vfs/vfs_implementation.h>

#include <cstring>
#include <algorithm>

static retro_vfs_interface uwp_vfs_iface = {
    reinterpret_cast<retro_vfs_get_path_t>(retro_vfs_file_get_path_impl),
    reinterpret_cast<retro_vfs_open_t>(retro_vfs_file_open_impl),
    reinterpret_cast<retro_vfs_close_t>(retro_vfs_file_close_impl),
    reinterpret_cast<retro_vfs_size_t>(retro_vfs_file_size_impl),
    reinterpret_cast<retro_vfs_tell_t>(retro_vfs_file_tell_impl),
    reinterpret_cast<retro_vfs_seek_t>(retro_vfs_file_seek_impl),
    reinterpret_cast<retro_vfs_read_t>(retro_vfs_file_read_impl),
    reinterpret_cast<retro_vfs_write_t>(retro_vfs_file_write_impl),
    reinterpret_cast<retro_vfs_flush_t>(retro_vfs_file_flush_impl),
    reinterpret_cast<retro_vfs_remove_t>(retro_vfs_file_remove_impl),
    reinterpret_cast<retro_vfs_rename_t>(retro_vfs_file_rename_impl),
    reinterpret_cast<retro_vfs_truncate_t>(retro_vfs_file_truncate_impl),
    reinterpret_cast<retro_vfs_stat_t>(retro_vfs_stat_impl),
    reinterpret_cast<retro_vfs_mkdir_t>(retro_vfs_mkdir_impl),
    reinterpret_cast<retro_vfs_opendir_t>(retro_vfs_opendir_impl),
    reinterpret_cast<retro_vfs_readdir_t>(retro_vfs_readdir_impl),
    reinterpret_cast<retro_vfs_dirent_get_name_t>(retro_vfs_dirent_get_name_impl),
    reinterpret_cast<retro_vfs_dirent_is_dir_t>(retro_vfs_dirent_is_dir_impl),
    reinterpret_cast<retro_vfs_closedir_t>(retro_vfs_closedir_impl)
};

using namespace dosbox_uwp;

RetroVideoFrame RetroCore::s_lastFrame;
std::mutex RetroCore::s_frameMutex;
std::vector<int16_t> RetroCore::s_audioBuffer;
std::mutex RetroCore::s_audioMutex;
bool RetroCore::s_keyboardState[256] = {};
int RetroCore::s_mouseRelX = 0;
int RetroCore::s_mouseRelY = 0;
float RetroCore::s_ptrX = 0;
float RetroCore::s_ptrY = 0;
bool RetroCore::s_ptrDown = false;

RetroCore::RetroCore() {}
RetroCore::~RetroCore() { Shutdown(); }

static bool retro_env_wrap(unsigned cmd, void* data)
{
    return RetroCore::retro_env(cmd, data) != 0;
}

bool RetroCore::Init()
{
    OutputDebugStringA("[dosbox-uwp] RetroCore::Init enter\n");
    s_lastFrame.valid = false;

    OutputDebugStringA("[dosbox-uwp] retro_set_environment\n");
    retro_set_environment(retro_env_wrap);
    OutputDebugStringA("[dosbox-uwp] retro_set_video_refresh\n");
    retro_set_video_refresh(&RetroCore::retro_video);
    OutputDebugStringA("[dosbox-uwp] retro_set_audio_sample_batch\n");
    retro_set_audio_sample_batch(&RetroCore::retro_audio);
    OutputDebugStringA("[dosbox-uwp] retro_set_input_poll\n");
    retro_set_input_poll(&RetroCore::retro_input_poll);
    OutputDebugStringA("[dosbox-uwp] retro_set_input_state\n");
    retro_set_input_state(&RetroCore::retro_input_state);

    OutputDebugStringA("[dosbox-uwp] retro_init call\n");
    retro_init();
    m_initialized = true;

    OutputDebugStringA("[dosbox-uwp] RetroCore::Init exit OK\n");
    return true;
}

bool RetroCore::LoadGame(const std::wstring& uwpPath, const std::vector<uint8_t>& romData)
{
    OutputDebugStringA("[dosbox-uwp] RetroCore::LoadGame enter\n");
    if (!m_initialized)
    {
        OutputDebugStringA("[dosbox-uwp] LoadGame FAILED: not initialized\n");
        return false;
    }

    char buf[512];
    int len = WideCharToMultiByte(CP_UTF8, 0, uwpPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0)
    {
        OutputDebugStringA("[dosbox-uwp] LoadGame FAILED: WideCharToMultiByte len=0\n");
        return false;
    }
    std::string pathUtf8(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, uwpPath.c_str(), -1, &pathUtf8[0], len, nullptr, nullptr);
    sprintf_s(buf, "[dosbox-uwp] LoadGame path: %s data=%zu bytes\n", pathUtf8.c_str(), romData.size());
    OutputDebugStringA(buf);

    retro_game_info info = {};
    info.path = pathUtf8.c_str();
    info.data = romData.empty() ? nullptr : romData.data();
    info.size = romData.size();

    OutputDebugStringA("[dosbox-uwp] retro_load_game call\n");
    if (!retro_load_game(&info))
    {
        OutputDebugStringA("[dosbox-uwp] retro_load_game FAILED\n");
        return false;
    }

    m_loaded = true;
    OutputDebugStringA("[dosbox-uwp] retro_load_game SUCCESS\n");
    return true;
}

void RetroCore::RunFrame()
{
    if (!m_loaded) return;
    static int frameCount = 0;
    frameCount++;
    if ((frameCount % 300) == 0)
    {
        char buf[128];
        sprintf_s(buf, "[dosbox-uwp] RunFrame #%d\n", frameCount);
        OutputDebugStringA(buf);
    }
    retro_run();
}

void RetroCore::UnloadGame()
{
    if (!m_loaded) return;
    OutputDebugStringA("[dosbox-uwp] UnloadGame\n");
    m_loaded = false;
    retro_unload_game();
}

void RetroCore::Shutdown()
{
    OutputDebugStringA("[dosbox-uwp] Shutdown\n");
    UnloadGame();
    if (m_initialized)
    {
        m_initialized = false;
        OutputDebugStringA("[dosbox-uwp] retro_deinit\n");
        retro_deinit();
    }
}

RetroVideoFrame RetroCore::GrabVideoFrame()
{
    std::lock_guard<std::mutex> lock(s_frameMutex);
    RetroVideoFrame frame = s_lastFrame;
    s_lastFrame.valid = false;
    if (frame.valid)
    {
        char buf[128];
        sprintf_s(buf, "[dosbox-uwp] GrabVideoFrame: %ux%u pitch=%u size=%zu\n",
            frame.width, frame.height, frame.pitch, frame.data.size());
        OutputDebugStringA(buf);
    }
    return frame;
}

RetroAudioSamples RetroCore::GrabAudio()
{
    std::lock_guard<std::mutex> lock(s_audioMutex);
    RetroAudioSamples result;
    if (!s_audioBuffer.empty())
    {
        result.samples = s_audioBuffer.data();
        result.frames = s_audioBuffer.size() / 2;
    }
    return result;
}

static const char* retro_env_name(unsigned cmd)
{
    switch (cmd)
    {
    case RETRO_ENVIRONMENT_GET_VFS_INTERFACE: return "GET_VFS_INTERFACE";
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: return "GET_SYSTEM_DIRECTORY";
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: return "GET_SAVE_DIRECTORY";
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: return "SET_PIXEL_FORMAT";
    case RETRO_ENVIRONMENT_SET_HW_RENDER: return "SET_HW_RENDER";
    case RETRO_ENVIRONMENT_SET_MESSAGE_EXT: return "SET_MESSAGE_EXT";
    case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK: return "SET_KEYBOARD_CALLBACK";
    case RETRO_ENVIRONMENT_GET_THROTTLE_STATE: return "GET_THROTTLE_STATE";
    case RETRO_ENVIRONMENT_SHUTDOWN: return "SHUTDOWN";
    default: return "UNKNOWN";
    }
}

void RetroCore::SetKeyState(unsigned key, bool down)
{
    if (key < 256) s_keyboardState[key] = down;
}

void RetroCore::SetMouseMove(int relX, int relY)
{
    s_mouseRelX += relX;
    s_mouseRelY += relY;
}

void RetroCore::SetPointer(float x, float y, bool down)
{
    s_ptrX = x;
    s_ptrY = y;
    s_ptrDown = down;
}

int RetroCore::retro_env(unsigned cmd, void* data)
{
    char buf[256];
    sprintf_s(buf, "[dosbox-uwp] retro_env cmd=%d(%s)\n", cmd, retro_env_name(cmd));
    OutputDebugStringA(buf);

    switch (cmd)
    {
    case RETRO_ENVIRONMENT_GET_VFS_INTERFACE:
    {
        auto* vfs_info = static_cast<retro_vfs_interface_info*>(data);
        vfs_info->required_interface_version = 3;
        vfs_info->iface = &uwp_vfs_iface;
        OutputDebugStringA("[dosbox-uwp]   VFS interface provided (v3)\n");
        return 1;
    }
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    {
        static std::string sysDir;
        if (sysDir.empty())
        {
            auto localFolder = Windows::Storage::ApplicationData::Current->LocalFolder;
            auto path = localFolder->Path->Data();
            int len = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
            sysDir.resize(len);
            WideCharToMultiByte(CP_UTF8, 0, path, -1, &sysDir[0], len, nullptr, nullptr);
        }
        *static_cast<const char**>(data) = sysDir.c_str();
        sprintf_s(buf, "[dosbox-uwp]   SYSTEM_DIR=%s\n", sysDir.c_str());
        OutputDebugStringA(buf);
        return 1;
    }
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
    {
        static std::string saveDir;
        if (saveDir.empty())
        {
            auto localFolder = Windows::Storage::ApplicationData::Current->LocalFolder;
            auto path = localFolder->Path->Data();
            int len = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
            saveDir.resize(len);
            WideCharToMultiByte(CP_UTF8, 0, path, -1, &saveDir[0], len, nullptr, nullptr);
        }
        *static_cast<const char**>(data) = saveDir.c_str();
        sprintf_s(buf, "[dosbox-uwp]   SAVE_DIR=%s\n", saveDir.c_str());
        OutputDebugStringA(buf);
        return 1;
    }
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
    {
        auto fmt = *static_cast<retro_pixel_format*>(data);
        const char* fmtName = (fmt == RETRO_PIXEL_FORMAT_0RGB1555 ? "0RGB1555" :
            fmt == RETRO_PIXEL_FORMAT_XRGB8888 ? "XRGB8888" :
            fmt == RETRO_PIXEL_FORMAT_RGB565 ? "RGB565" : "UNKNOWN");
        sprintf_s(buf, "[dosbox-uwp]   SET_PIXEL_FORMAT=%s(%d)\n", fmtName, fmt);
        OutputDebugStringA(buf);
        return 1;
    }
    case RETRO_ENVIRONMENT_SET_HW_RENDER:
    {
        OutputDebugStringA("[dosbox-uwp]   SET_HW_RENDER=REJECTED (return 0)\n");
        return 0;
    }
    case RETRO_ENVIRONMENT_SET_MESSAGE_EXT:
    {
        auto* msg = static_cast<const retro_message_ext*>(data);
        OutputDebugStringA("[dosbox-pure] ");
        OutputDebugStringA(msg->msg);
        OutputDebugStringA("\n");
        return 1;
    }
    case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK:
    {
        auto* cb = static_cast<const retro_keyboard_callback*>(data);
        if (cb && cb->callback)
        {
            OutputDebugStringA("[dosbox-uwp]   SET_KEYBOARD_CALLBACK: pushing held keys\n");
            for (unsigned key = 0; key < RETROK_LAST; key++)
            {
                bool down = s_keyboardState[key];
                if (down)
                    cb->callback(down, key, 0, 0);
            }
        }
        return 1;
    }
    case RETRO_ENVIRONMENT_GET_THROTTLE_STATE:
    {
        auto* state = static_cast<retro_throttle_state*>(data);
        state->mode = RETRO_THROTTLE_NONE;
        state->rate = 0.0f;
        OutputDebugStringA("[dosbox-uwp]   THROTTLE_STATE=NONE\n");
        return 1;
    }
    case RETRO_ENVIRONMENT_SHUTDOWN:
        OutputDebugStringA("[dosbox-pure] SHUTDOWN requested\n");
        return 1;
    default:
        sprintf_s(buf, "[dosbox-uwp]   UNSUPPORTED env cmd=%d\n", cmd);
        OutputDebugStringA(buf);
        return 0;
    }
}

void RetroCore::retro_video(const void* data, unsigned w, unsigned h, size_t pitch)
{
    if (!data || w == 0 || h == 0 || pitch == 0)
    {
        static int rejectCount = 0;
        rejectCount++;
        if ((rejectCount % 60) == 0)
        {
            char buf[128];
            sprintf_s(buf, "[dosbox-uwp] retro_video REJECTED #%d: data=%p w=%u h=%u pitch=%zu\n",
                rejectCount, data, w, h, pitch);
            OutputDebugStringA(buf);
        }
        return;
    }

    std::lock_guard<std::mutex> lock(s_frameMutex);
    size_t totalSize = (size_t)h * pitch;
    s_lastFrame.data.resize(totalSize);
    memcpy(s_lastFrame.data.data(), data, totalSize);
    s_lastFrame.width = w;
    s_lastFrame.height = h;
    s_lastFrame.pitch = (unsigned)pitch;
    s_lastFrame.valid = true;
}


size_t RetroCore::retro_audio(const int16_t* data, size_t frames)
{
    if (!data || frames == 0) return frames;

    std::lock_guard<std::mutex> lock(s_audioMutex);
    size_t samples = frames * 2;
    s_audioBuffer.resize(samples);
    memcpy(s_audioBuffer.data(), data, samples * sizeof(int16_t));
    return frames;
}



void RetroCore::retro_input_poll(void)
{
}

int16_t RetroCore::retro_input_state(unsigned port, unsigned device, unsigned index, unsigned id)
{
    if (port != 0) return 0;

    if (device == RETRO_DEVICE_KEYBOARD)
    {
        return (id < 256 && s_keyboardState[id]) ? 1 : 0;
    }

    if (device == RETRO_DEVICE_JOYPAD)
    {
        if (id < 256 && s_keyboardState[id])
            return 1;
        return 0;
    }

    if (device == RETRO_DEVICE_MOUSE)
    {
        switch (id)
        {
        case RETRO_DEVICE_ID_MOUSE_X: return s_mouseRelX;
        case RETRO_DEVICE_ID_MOUSE_Y: return s_mouseRelY;
        case RETRO_DEVICE_ID_MOUSE_LEFT: return 0;
        case RETRO_DEVICE_ID_MOUSE_RIGHT: return 0;
        default: return 0;
        }
    }

    if (device == RETRO_DEVICE_POINTER)
    {
        switch (id)
        {
        case RETRO_DEVICE_ID_POINTER_X: return (int16_t)(s_ptrX * 0x7fff);
        case RETRO_DEVICE_ID_POINTER_Y: return (int16_t)(s_ptrY * 0x7fff);
        case RETRO_DEVICE_ID_POINTER_PRESSED: return s_ptrDown ? 1 : 0;
        default: return 0;
        }
    }

    return 0;
}
