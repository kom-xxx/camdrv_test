#ifndef RENDER_H
#define RENDER_H

#include <vector>
#include <wchar.h>

#define NR_BUFFERS 2
#define NR_TEXTURE NR_BUFFERS
#define NR_THREADS 2

extern size_t bb_count;

struct render_environment {
    HWND window;
    wchar_t *class_name;
    ComPtr<IDXGIFactory2> factory;
    ComPtr<IDXGIAdapter> adapter;
};

struct render_input {
    ComPtr<ID3D12Resource> vertex;
    D3D12_VERTEX_BUFFER_VIEW vbv;
    ComPtr<ID3D12Resource> index;
    D3D12_INDEX_BUFFER_VIEW ibv;
};

struct render_texture {
    std::vector<std::vector<uint32_t>> image; /* VirtualAlloc()'ed mem or
                                                 texture image in vector */
    std::vector<ComPtr<ID3D12Resource>> upload_buffer{nullptr, nullptr};
    std::vector<ComPtr<ID3D12Resource>> texture_buffer{nullptr, nullptr};
    ComPtr<ID3D12DescriptorHeap> desc_heap;
    
};

struct render_command {
    ComPtr<ID3D12CommandAllocator> alloc;
    ComPtr<ID3D12GraphicsCommandList> list;
};

struct render_cmd_queue {
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12Fence> fence;
    uint64_t fvalue;
};

struct render_output {
    ComPtr<IDXGISwapChain1> schain;
    ComPtr<ID3D12DescriptorHeap> rtvheap;
    std::vector<ComPtr<ID3D12Resource>> back_buff{nullptr, nullptr};
    std::vector<ComPtr<ID3D12Resource>> readback_buffer{nullptr, nullptr};
};

struct render_pipeline {
    ComPtr<ID3D12RootSignature> sig;
    ComPtr<ID3D12PipelineState> pipe;
    ComPtr<ID3DBlob> vs_code;
    ComPtr<ID3DBlob> ps_code;
    D3D12_INPUT_ELEMENT_DESC *input_layout;
    uint32_t layout_count;
    D3D12_VIEWPORT view;
    D3D12_RECT scissor;
};

struct render_heap {
    ComPtr<ID3D12Heap> heap;
    size_t offset;
    size_t count;
};

struct render {
    ComPtr<ID3D12Device> dev;
    render_heap ul_heap;
    render_heap tex_heap;
    render_heap dl_heap;
    render_environment env;
    render_input in;
    render_texture tex;
    render_command cmd;
    render_cmd_queue queue;
    render_pipeline pipe;
    render_output out;
};

#endif	/* !RENDER_H */
