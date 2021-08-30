#ifndef D3D12_FUNCS_H
#define D3D12_FUNCS_H

#include "render.h"

void enable_debug_layer(void);
void create_device(render_environment *, ID3D12Device **);
void create_swap_chain(render_environment *, render_cmd_queue *, size_t, size_t,
		       IDXGISwapChain1 **);
void
create_command_queue(ID3D12Device *dev, render_cmd_queue *queue,
                     D3D12_COMMAND_LIST_TYPE
                         type = D3D12_COMMAND_LIST_TYPE_DIRECT,
                     D3D12_COMMAND_QUEUE_FLAGS
                         flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
                     D3D12_COMMAND_QUEUE_PRIORITY
                         prio = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL);
void create_command_list(ID3D12Device *, render_command *,
                         D3D12_COMMAND_LIST_TYPE
                             type = D3D12_COMMAND_LIST_TYPE_DIRECT);
void create_fence(ID3D12Device *, render_cmd_queue *);

D3D12_RESOURCE_BARRIER
transition_barrier(ID3D12Resource *res,
		   D3D12_RESOURCE_STATES before
		   	= D3D12_RESOURCE_STATE_COMMON,
		   D3D12_RESOURCE_STATES after
		   	= D3D12_RESOURCE_STATE_COMMON,
		   D3D12_RESOURCE_BARRIER_FLAGS flags
		   	= D3D12_RESOURCE_BARRIER_FLAG_NONE,
		   uint32_t subres = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
void transition_barrier_state(D3D12_RESOURCE_BARRIER *,
			      D3D12_RESOURCE_STATES, D3D12_RESOURCE_STATES);
void create_upload_heap(ID3D12Device *, render_heap *, size_t);
void create_upload_buffer(ID3D12Device *,
                          render_heap *, size_t, ID3D12Resource **);
void create_texture_heap(ID3D12Device *, render_heap *, size_t);
void create_texture_buffer(ID3D12Device *,
                           render_heap *, size_t, size_t, ID3D12Resource **);
void create_target_heap(ID3D12Device *, render_heap *, size_t);
void create_target_buffer(ID3D12Device *, render_heap *, size_t, size_t,
			  ID3D12Resource **);
void create_readback_heap(ID3D12Device *, render_heap *, size_t);
void upload_texture(ID3D12Device *, render_command *, render_cmd_queue *,
                    ID3D12Resource *, ID3D12Resource *);
void setup_texture(ID3D12Device *, render_heap *, render_heap *,
                   render_command *, render_cmd_queue *,
                   render_texture *, size_t, size_t);
void create_readback_buffer(ID3D12Device *,
                            render_heap *, size_t, ID3D12Resource **);
void acquire_download_buffers(ID3D12Device *, render_heap *, render_output *,
                              size_t);
void create_vbv(ID3D12Device *, render_heap *, render_input *);
void create_ibv(ID3D12Device *, render_heap *, render_input *);
void create_rtv(ID3D12Device *, render_environment *, render_output *);
void create_srv(ID3D12Device *, render_texture *);
void compile_hlsl(const wchar_t *, const char *, const char *, ID3DBlob **);
void define_input_layout(render_pipeline *);
void create_rootsignature(ID3D12Device *, render_pipeline *);
void create_pipeline(ID3D12Device *, render_pipeline *);
void setup_viewport(D3D12_VIEWPORT *, uint32_t, uint32_t);
void setup_scissor(D3D12_RECT *, size_t, size_t);
void render_core(struct render *, uint32_t);

#endif	/* !D3D12_FUNCS_H */
