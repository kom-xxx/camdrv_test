
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

namespace wrl = Microsoft::WRL;

#include "cdefs.h"
#include "d3d12_funcs.h"
#include "render.h"
#include "thread.h"
#include "win_appl.h"

#if 0
#define WIN_WIDTH 1280
#define WIN_HEIGHT 1024
#else
# if 0
#define WIN_WIDTH 256
#define WIN_HEIGHT 256
# endif
#define WIDTH_DELTA 256
#define HEIGHT_DELTA 256
#define DISP_WIDTH win_sizes[SCHAIN_THREAD][0]
#define DISP_HEIGHT win_sizes[SCHAIN_THREAD][1]
#endif

size_t win_sizes[NR_THREADS][2];

void
make_window_sizes(void)
{
    for (size_t i = 0; i < NR_THREADS; ++i) {
        win_sizes[i][0] = WIDTH_DELTA;
        win_sizes[i][1] = HEIGHT_DELTA * (i + 1);
    }
}

int WINAPI
WinMain(HINSTANCE inst, HINSTANCE _, LPSTR cmd_line, int show_mode)
{
    const wchar_t *class_name;
    render_global global;
    HANDLE threads[NR_THREADS];

    debug_cons();

    make_window_sizes();

    global.env.window = create_window(DISP_WIDTH, DISP_HEIGHT, &class_name);

    enable_debug_layer();

    create_device(&global.env, &global.dev);
    create_command_queue(global.dev.Get(), &global.queue);
    create_fence(global.dev.Get(), &global.queue);

    create_swap_chain(&global.env, &global.queue, DISP_WIDTH, DISP_HEIGHT,
                      &global.schain);

    create_upload_heap(global.dev.Get(), &global.ul_heap, MiB(32));
    create_texture_heap(global.dev.Get(), &global.tex_heap, MiB(32));
    create_target_heap(global.dev.Get(), &global.tgt_heap, MiB(64));
    create_readback_heap(global.dev.Get(), &global.dl_heap, MiB(32));

    ShowWindow(global.env.window, SW_SHOW);

    /**
     ** create_threadm
     **/
    for (size_t i = 0; i < NR_THREADS; ++i)
        threads[i] = thread_create((uint32_t)i, &global,
                                   win_sizes[SCHAIN_THREAD][0],
                                   win_sizes[SCHAIN_THREAD][1]);

    Sleep(600000);

    for (size_t i = 0; i < NR_THREADS; ++i)
        thread_stop(threads[i], (uint32_t)i);

    UnregisterClass(class_name, GetModuleHandle(nullptr));

    return 0;
}
