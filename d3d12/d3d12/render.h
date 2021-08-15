#ifndef RENDER_H
#define RENDER_H

#include <vector>
#include <wchar.h>

#define NR_BUFFERS 2

extern size_t bb_count;

struct render_environment {
	HWND window;
	wchar_t *class_name;
	ComPtr<IDXGIFactory2> factory;
	ComPtr<IDXGIAdapter> adapter;
};

struct render_input {
	ComPtr<ID3D12Heap> heap;
	ComPtr<ID3D12Resource> vertex;
	D3D12_VERTEX_BUFFER_VIEW vbv;
	ComPtr<ID3D12Resource> index;
	D3D12_INDEX_BUFFER_VIEW ibv;
	size_t offset;
};

struct render_command {
	ComPtr<ID3D12CommandAllocator> alloc;
	ComPtr<ID3D12GraphicsCommandList> list;
	ComPtr<ID3D12CommandQueue> queue;
};

struct render_output {
	ComPtr<IDXGISwapChain1> schain;
	ComPtr<ID3D12DescriptorHeap> rtvheap;
	std::vector<ComPtr<ID3D12Resource>> back_buff{nullptr, nullptr};
	ComPtr<ID3D12Fence> fence;
	uint64_t fvalue;
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

struct render {
	ComPtr<ID3D12Device> dev;
	render_environment env;
	render_input vsin;
	render_pipeline pipe;
	render_command cmd;
	render_output out;
};

#endif	/* !RENDER_H */
