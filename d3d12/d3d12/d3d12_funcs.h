#ifndef D3D12_FUNCS_H
#define D3D12_FUNCS_H

#include "render.h"

void enable_debug_layer();
void create_device(render_environment *, ID3D12Device **);
void create_command_list(ID3D12Device *, render_command *);
void create_swap_chain(render_environment *, render_command *, size_t, size_t,
		       IDXGISwapChain1 **);
void create_rtv(ID3D12Device *, render_environment *, render_output *);
void create_fence(ID3D12Device *, render_output *);

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
void create_upload_heap(ID3D12Device *, render_input *, size_t, ID3D12Heap **);
void create_upload_buffer(ID3D12Device *, render_input *, size_t,
			  ID3D12Resource **);
void create_vertex_buffer_view(ID3D12Device *, render_input *);
void create_index_buffer_view(ID3D12Device *, render_input *);
void compile_hlsl(const wchar_t *, const char *, const char *, ID3DBlob **);
void define_input_layout(render_pipeline *);
void create_rootsignature(ID3D12Device *, render_pipeline *);
void create_pipeline(ID3D12Device *, render_pipeline *);
void setup_viewport(D3D12_VIEWPORT *, uint32_t, uint32_t);
void setup_scisor(D3D12_RECT *, size_t, size_t);
void render_core(render *, uint32_t);

#endif	/* !D3D12_FUNCS_H */
