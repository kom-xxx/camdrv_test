#ifndef RENDER_H
#define RENDER_H

#include <mutex>
#include <vector>
#include <wchar.h>

#define NR_BUFFERS 2
#define NR_TEXTURE NR_BUFFERS
#define NR_THREADS 8
#define SCHAIN_THREAD 0
extern size_t bb_count;

struct render_environment {
    HWND window;
    wchar_t *class_name;
    wrl::ComPtr<IDXGIFactory2> factory;
    wrl::ComPtr<IDXGIAdapter> adapter;
};

struct render_input {
    wrl::ComPtr<ID3D12Resource> vertex;
    D3D12_VERTEX_BUFFER_VIEW vbv;
    wrl::ComPtr<ID3D12Resource> index;
    D3D12_INDEX_BUFFER_VIEW ibv;
};

struct render_texture {
	void *image[NR_TEXTURE];    /* VirtualAlloc()'ed mem or
                                   texture image in vector */
    wrl::ComPtr<ID3D12Resource> upload_buffer[NR_BUFFERS];
    wrl::ComPtr<ID3D12Resource> texture_buffer[NR_BUFFERS];
    wrl::ComPtr<ID3D12DescriptorHeap> desc_heap;
    
};

struct render_command {
    wrl::ComPtr<ID3D12CommandAllocator> alloc;
    wrl::ComPtr<ID3D12GraphicsCommandList> list;
};

struct render_cmd_queue {
    wrl::ComPtr<ID3D12CommandQueue> queue;
    wrl::ComPtr<ID3D12Fence> fence;
    uint64_t fvalue;
};

struct render_output {
    wrl::ComPtr<ID3D12DescriptorHeap> rtvheap;
    wrl::ComPtr<ID3D12DescriptorHeap> swcheap;
    wrl::ComPtr<ID3D12Resource> swcbuff[NR_BUFFERS];
	wrl::ComPtr<ID3D12Resource> target[NR_BUFFERS];
    wrl::ComPtr<ID3D12Resource> readback[NR_BUFFERS];
    uint32_t frame_index;
};

struct render_pipeline {
    wrl::ComPtr<ID3D12RootSignature> sig;
    wrl::ComPtr<ID3D12PipelineState> pipe;
    wrl::ComPtr<ID3DBlob> vs_code;
    wrl::ComPtr<ID3DBlob> ps_code;
    D3D12_INPUT_ELEMENT_DESC *input_layout;
    uint32_t layout_count;
    D3D12_VIEWPORT view;
    D3D12_RECT scissor;
};

struct render_heap {
    wrl::ComPtr<ID3D12Heap> heap;
    size_t offset;
    size_t count;
    std::mutex lock;
};

struct render_global {
    wrl::ComPtr<ID3D12Device> dev;
    wrl::ComPtr<IDXGISwapChain1> schain;
    render_heap ul_heap;
    render_heap tex_heap;
	render_heap tgt_heap;
    render_heap dl_heap;
    render_environment env;
    render_cmd_queue queue;
};

struct thread_resources {
    render_global *global;
    render_input in;
    render_texture tex;
    render_command cmd;
    render_pipeline pipe;
    render_output out;
};

#endif	/* !RENDER_H */
