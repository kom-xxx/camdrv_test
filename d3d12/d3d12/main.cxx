#include <stdint.h>
#include <stdio.h>
#include <wchar.h>

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

using namespace Microsoft::WRL;

#include "cdefs.h"
#include "d3d12_funcs.h"
#include "render.h"
#include "win_appl.h"

#if 0
#define WIN_WIDTH 1280
#define WIN_HEIGHT 1024
#else
#define WIN_WIDTH 256
#define WIN_HEIGHT 256
#endif

struct render render;

void
main_loop()
{
    MSG msg = {};
    uint32_t counter = 0;

    while (true) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (msg.message == WM_QUIT)
            break;

        render_core(&render, counter);
        counter += 1;
    }
}

int WINAPI
WinMain(HINSTANCE inst, HINSTANCE _, LPSTR cmd_line, int show_mode)
{
    const wchar_t *class_name;

    debug_cons();

    render.env.window = create_window(WIN_WIDTH, WIN_HEIGHT, &class_name);

    enable_debug_layer();

    create_device(&render.env, &render.dev);

    create_command_queue(render.dev.Get(), &render.queue);

#ifndef WITHOUT_SWAP_CHAIN
    create_swap_chain(&render.env, &render.queue, WIN_WIDTH, WIN_HEIGHT,
                      &render.out.schain);
#endif

    create_upload_heap(render.dev.Get(), &render.ul_heap, MiB(32));
    create_texture_heap(render.dev.Get(), &render.tex_heap, MiB(32));
    create_target_heap(render.dev.Get(), &render.tgt_heap, MiB(64));
    create_readback_heap(render.dev.Get(), &render.dl_heap, MiB(32));

    create_command_list(render.dev.Get(), &render.cmd);

    create_fence(render.dev.Get(), &render.queue);

    create_vbv(render.dev.Get(), &render.ul_heap, &render.in);
    create_ibv(render.dev.Get(), &render.ul_heap, &render.in);

    setup_texture(render.dev.Get(), &render.ul_heap, &render.tex_heap,
                  &render.cmd, &render.queue, &render.tex,
                  WIN_WIDTH, WIN_HEIGHT);
    create_srv(render.dev.Get(), &render.tex);

#ifdef WITHOUT_SWAP_CHAIN
    acquire_target_buffers(render.dev.Get(), &render.tgt_heap, &render.out,
                           WIN_WIDTH, WIN_HEIGHT);
#endif
    acquire_download_buffers(render.dev.Get(), &render.dl_heap, &render.out,
                             WIN_WIDTH * WIN_HEIGHT * sizeof (uint32_t));
    create_rtv(render.dev.Get(), &render.out);

    compile_hlsl(L"shaders.hlsl", "vs_main", "vs_5_0",
                 &render.pipe.vs_code);
    compile_hlsl(L"shaders.hlsl", "ps_main", "ps_5_0",
                 &render.pipe.ps_code);
    define_input_layout(&render.pipe);
    create_rootsignature(render.dev.Get(), &render.pipe);
    create_pipeline(render.dev.Get(), &render.pipe);

    setup_viewport(&render.pipe.view, WIN_WIDTH, WIN_HEIGHT);
    setup_scissor(&render.pipe.scissor, WIN_WIDTH, WIN_HEIGHT);

    ShowWindow(render.env.window, SW_SHOW);

    main_loop();

    UnregisterClass(class_name, GetModuleHandle(nullptr));

    return 0;
}
