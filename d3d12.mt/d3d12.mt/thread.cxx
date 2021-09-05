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

thread_arg thread_arg_list[NR_THREADS];
uint32_t tid_list[NR_THREADS];

thread_resources *
init_thread_resources(render_global *global,
                      uint32_t cam_id, size_t width, size_t height, bool sc)
{
    ID3D12Device *dev = global->dev.Get();
    thread_resources *thr_res = new thread_resources();

    create_command_list(dev, &thr_res->cmd);

    create_vbv(dev, &global->ul_heap, &thr_res->in);
    create_ibv(dev, &global->ul_heap, &thr_res->in);

    setup_texture(dev, &global->ul_heap, &global->tex_heap,
                  &thr_res->cmd, &global->queue, &thr_res->tex, width, height);
    create_srv(dev, &thr_res->tex);

    acquire_target_buffers(dev, &global->tgt_heap, &thr_res->out,
                           width, height);
    acquire_download_buffers(dev, &global->dl_heap, &thr_res->out,
                             width * height * sizeof (uint32_t));
    IDXGISwapChain *schain = nullptr;
    if (sc)
        schain = global->schain.Get();

    create_rtv(dev, schain, &thr_res->out);

    compile_hlsl(L"shaders.hlsl", "vs_main", "vs_5_0",
                 &thr_res->pipe.vs_code);
    compile_hlsl(L"shaders.hlsl", "ps_main", "ps_5_0",
                 &thr_res->pipe.ps_code);
    define_input_layout(&thr_res->pipe);
    create_rootsignature(dev, &thr_res->pipe);
    create_pipeline(dev, &thr_res->pipe);

    setup_viewport(&thr_res->pipe.view, width, height);
    setup_scissor(&thr_res->pipe.scissor, width, height);

    return thr_res;
}


uint32_t
thread_main(void *arg_)
{
    uint64_t frame = 0;
    struct thread_arg *arg = (struct thread_arg *)arg_;

    fprintf(stderr, "%s: caid:%d width:%lld height:%lld\n",
            __func__, arg->cam_id, arg->width, arg->height);

    struct thread_resources
        *tres = init_thread_resources(arg->global, arg->cam_id,
                                      arg->width, arg->height, arg->schain);

    fprintf(stderr, "%s: init_thread_resources: DONE\n", __func__);

    while (!arg->exit) {
        render_core(arg, arg->global, tres, frame);
        frame += 1;
    }

    return 0;
}

HANDLE
thread_create(uint32_t camid, render_global *global,
              size_t width, size_t height)
{
    HANDLE h;

    bool schain = camid == SCHAIN_THREAD ? true : false;
    thread_arg arg = {global, camid, width, height, schain, false};
    thread_arg_list[camid] = arg;

    h = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)thread_main,
                     &thread_arg_list[camid], 0, (LPDWORD)&tid_list[camid]);

    return h;
}

void
thread_stop(HANDLE wh, uint32_t camid)
{
    thread_arg_list[camid].exit = true;
}
