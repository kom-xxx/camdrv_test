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

#define WIN_WIDTH 1280
#define WIN_HEIGHT 1024

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
	create_swap_chain(&render.env, &render.queue, WIN_WIDTH, WIN_HEIGHT,
			  &render.out.schain);

	create_upload_heap(render.dev.Get(), &render.upload, MiB(32));
        create_texture_heap(render.dev.Get(), &render.texture, MiB(32));
#if 1
        ComPtr<ID3D12Resource> tex;
        create_texture_buffer(render.dev.Get(), &render.upload,
                              WIN_WIDTH, WIN_HEIGHT, &tex);
#endif
	create_command_list(render.dev.Get(), &render.cmd);

	create_fence(render.dev.Get(), &render.queue);

	create_vbv(render.dev.Get(), &render.upload, &render.in);
	create_ibv(render.dev.Get(), &render.upload, &render.in);
	create_rtv(render.dev.Get(), &render.env, &render.out);
	ShowWindow(render.env.window, SW_SHOW);

	compile_hlsl(L"shaders.hlsl", "vs_main", "vs_5_0",
		     &render.pipe.vs_code);
	compile_hlsl(L"shaders.hlsl", "ps_main", "ps_5_0",
		     &render.pipe.ps_code);
	define_input_layout(&render.pipe);
	create_rootsignature(render.dev.Get(), &render.pipe);
	create_pipeline(render.dev.Get(), &render.pipe);

	setup_viewport(&render.pipe.view, WIN_WIDTH, WIN_HEIGHT);
	setup_scissor(&render.pipe.scissor, WIN_WIDTH, WIN_HEIGHT);

	main_loop();

	UnregisterClass(class_name, GetModuleHandle(nullptr));

	return 0;
}
