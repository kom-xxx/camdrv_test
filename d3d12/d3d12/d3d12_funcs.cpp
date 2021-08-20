#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <Windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgi1_6.h>
#include <wrl.h>

using namespace Microsoft::WRL;
using namespace DirectX;

#include "cdefs.h"
#include "d3d12_funcs.h"
#include "debug.h"
#include "win_appl.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define PCI_VENDOR_NVIDIA 0x10de

#define REQUIRED_D3D12_FEATURE D3D_FEATURE_LEVEL_12_1

#if defined(_DEBUG) && defined(DXGI_DEBUG_LAYER_INSTALLED)
#define DXGI_CREATE_FACTORY_FLAGS DXGI_CREATE_FACTORY_DEBUG
#else
#define DXGI_CREATE_FACTORY_FLAGS 0
#endif

#if 0
struct XMFLOAT2
{
    float x;
    float y;

    XMFLOAT2() = default;

    XMFLOAT2(const XMFLOAT2&) = default;
    XMFLOAT2& operator=(const XMFLOAT2&) = default;

    XMFLOAT2(XMFLOAT2&&) = default;
    XMFLOAT2& operator=(XMFLOAT2&&) = default;

    XM_CONSTEXPR XMFLOAT2(float _x, float _y) : x(_x), y(_y) {}
    explicit XMFLOAT2(_In_reads_(2) const float *pArray) : x(pArray[0]), y(pArray[1]) {}
};

struct XMFLOAT3
{
    float x;
    float y;
    float z;

    XMFLOAT3() = default;

    XMFLOAT3(const XMFLOAT3&) = default;
    XMFLOAT3& operator=(const XMFLOAT3&) = default;

    XMFLOAT3(XMFLOAT3&&) = default;
    XMFLOAT3& operator=(XMFLOAT3&&) = default;

    XM_CONSTEXPR XMFLOAT3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    explicit XMFLOAT3(_In_reads_(3) const float *pArray) : x(pArray[0]), y(pArray[1]), z(pArray[2]) {}
};
#endif

BEGIN_UNNAMED

void
find_nv_adapter(IDXGIFactory2 **factory_, IDXGIAdapter **adapter_)
{
	HRESULT hr;

	ComPtr<IDXGIFactory2> factory;
	hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_FLAGS,
				IID_PPV_ARGS(&factory));
	RCK(hr, "CreateDXGIFactory2:%x");

	ComPtr<IDXGIAdapter> adapter;
	for (uint32_t i = 0;
	     factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND;
	     ++i) {
		DXGI_ADAPTER_DESC desc;
		hr = adapter->GetDesc(&desc);
		RCK(hr, "IDXGIAdapter::GetDesc:%x");

		if (desc.VendorId != PCI_VENDOR_NVIDIA)
			continue;
		hr = D3D12CreateDevice(adapter.Get(), REQUIRED_D3D12_FEATURE,
				       _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr))
			break;
	}
	*factory_ = factory.Detach();
	*adapter_ = adapter.Detach();
}

END_UNNAMED

void
enable_debug_layer()
{
#if defined(_DEBUG) && defined(DXGI_DEBUG_LAYER_INSTALLED)
	HRESULT hr;

	ComPtr<ID3D12Debug> dbg_layer;
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dbg_layer));
	if (SUCCEEDED(hr)) {
		dbg_layer->EnableDebugLayer();
		dbg_layer->Release();
	}
#endif
}

void
create_device(render_environment *env, ID3D12Device **dev)
{
	HRESULT hr;

	find_nv_adapter(&env->factory, &env->adapter);
	if (!env->adapter)
		throw std::runtime_error("create_device: NVIDIA's "
					 "adpter not found");
	hr = D3D12CreateDevice(env->adapter.Get(),
			       REQUIRED_D3D12_FEATURE, IID_PPV_ARGS(dev));
	RCK(hr, "D3D12CreateDevice:%x");
}

void
create_command_list(ID3D12Device *dev, render_command *cmd)
{
	HRESULT hr;

	hr = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
					 IID_PPV_ARGS(&cmd->alloc));
	RCK(hr, "CreateCommandAllocator:%x");

	hr = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
				    cmd->alloc.Get(), nullptr,
				    IID_PPV_ARGS(&cmd->list));
	RCK(hr, "CreateCommandList:%x");

	D3D12_COMMAND_QUEUE_DESC qdesc = {
		D3D12_COMMAND_LIST_TYPE_DIRECT,	     /* type */
		D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, /* priority */
		D3D12_COMMAND_QUEUE_FLAG_NONE,	     /* flags */
		0				     /* nodemask */
	};
	hr = dev->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&cmd->queue));
	RCK(hr, "CreateCommandQueue:%x");
}

void
create_swap_chain(render_environment *env, render_command *cmd,
		  size_t width, size_t height, IDXGISwapChain1 **swap_chain)
{
	HRESULT hr;

	DXGI_SWAP_CHAIN_DESC1 sc_desc = {
		(uint32_t)width,	       /* width */
		(uint32_t)height,	       /* height */
		DXGI_FORMAT_R8G8B8A8_UNORM,    /* format */
		false,			       /* streo */
		{1, 0},			       /* sample desc */
		DXGI_USAGE_BACK_BUFFER,	       /* buffer usage */
		2,			       /* buffer count */
		DXGI_SCALING_STRETCH,	       /* scaling */
		DXGI_SWAP_EFFECT_FLIP_DISCARD, /* swap effect */
		DXGI_ALPHA_MODE_UNSPECIFIED,	       /* alpha mode */
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH /* flags */
	};
	hr = env->factory->CreateSwapChainForHwnd(cmd->queue.Get(), env->window,
						  &sc_desc, nullptr, nullptr,
						  swap_chain);
	RCK(hr, "CreateSwapChainForHwnd:%x");
}

void
create_rtv(ID3D12Device *dev, render_environment *env, render_output *out)
{
	HRESULT hr;

	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV, /* type */
		2,				/* # of descriptor */
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,/* flags */
		0				/* node mask */
	};
	hr = dev->CreateDescriptorHeap(&rtv_heap_desc,
				       IID_PPV_ARGS(&out->rtvheap));
	RCK(hr, "CreateDescriptorHeap:%x");

	DXGI_SWAP_CHAIN_DESC sw_desc;
	hr = out->schain->GetDesc(&sw_desc);
	RCK(hr, "IDXGISwapChain1::GetDesc:%x");

	D3D12_CPU_DESCRIPTOR_HANDLE handle =
		out->rtvheap->GetCPUDescriptorHandleForHeapStart();
	const D3D12_DESCRIPTOR_HEAP_TYPE heap_type =
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	for (size_t i = 0; i < sw_desc.BufferCount; ++i) {
		hr = out->schain->GetBuffer((uint32_t)i,
					    IID_PPV_ARGS(&out->back_buff[i]));
		RCK(hr, "SwapChain::GetBuffer:%x");

		dev->CreateRenderTargetView(out->back_buff[i].Get(),
					    nullptr, handle);
		handle.ptr += dev->GetDescriptorHandleIncrementSize(heap_type);
	}
}

void
create_fence(ID3D12Device *dev, render_output *out)
{
	HRESULT hr;

	out->fvalue = 0;

	hr = dev->CreateFence(out->fvalue, D3D12_FENCE_FLAG_NONE,
			      IID_PPV_ARGS(&out->fence));
	RCK(hr, "CreateFence:%x");
}

D3D12_RESOURCE_BARRIER *
transition_barrier(ID3D12Resource *res,
		   D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after,
		   D3D12_RESOURCE_BARRIER_FLAGS flags, uint32_t subres)
{
	D3D12_RESOURCE_BARRIER *barrier = new D3D12_RESOURCE_BARRIER;
	barrier->Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier->Flags = flags;
	barrier->Transition.pResource = res;
	barrier->Transition.Subresource = subres;
	barrier->Transition.StateBefore = before;
	barrier->Transition.StateAfter = after;

	return barrier;
}

void
transition_barrier_state(D3D12_RESOURCE_BARRIER *barrier,
			 D3D12_RESOURCE_STATES before,
			 D3D12_RESOURCE_STATES after)
{
	barrier->Transition.StateBefore = before;
	barrier->Transition.StateAfter = after;
}

void
create_upload_heap(ID3D12Device *dev, render_input *in,
		   size_t size, ID3D12Heap **heap)
{
	HRESULT hr;

	D3D12_HEAP_DESC desc = {
		MiB(1),
		{		/* heap properties */
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0,
			0
		},
		0,		/* alignment */
		D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS
	};
	hr = dev->CreateHeap(&desc, IID_PPV_ARGS(heap));
	RCK(hr, "CreateHeap(upload):%x");
	in->offset = 0;
}

void
create_upload_buffer(ID3D12Device *dev, render_input *in,
		     size_t size, ID3D12Resource **res)
{
	HRESULT hr;

	D3D12_RESOURCE_DESC res_desc = {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,		/* alignment */
		size,		/* width */
		1,		/* height */
		1,		/* depth or array size */
		1,		/* mip levels */
		DXGI_FORMAT_UNKNOWN,
		{1, 0},		/* sample desc */
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		D3D12_RESOURCE_FLAG_NONE
	};
	hr = dev->CreatePlacedResource(in->heap.Get(), in->offset, &res_desc,
				       D3D12_RESOURCE_STATE_GENERIC_READ,
				       nullptr, IID_PPV_ARGS(res));
	RCK(hr, "CreatePlacedResource(upload):%x");

	D3D12_RESOURCE_ALLOCATION_INFO alloc_info =
		dev->GetResourceAllocationInfo(0, 1, &res_desc);
	in->offset += alloc_info.SizeInBytes;
}

void
create_vertex_buffer_view(ID3D12Device *dev, render_input *in)
{
	HRESULT hr;

	struct vertex {
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	};
	vertex vertexes[] = {
		{{-0.4f, -0.7f, 0.0f}, {0.0f, 1.0f}}, /* left, bottom */
		{{-0.4f,  0.7f, 0.0f}, {0.0f, 0.0f}}, /* left, top */
		{{ 0.4f, -0.7f, 0.0f}, {1.0f, 1.0f}}, /* right, bottom */
		{{ 0.4f,  0.7f, 0.0f}, {1.0f, 0.0f}}  /* right, top */
	};
	create_upload_buffer(dev, in, sizeof vertexes, &in->vertex);

	void *map;
	hr = in->vertex->Map(0, nullptr, &map);
	memcpy(map, vertexes, sizeof vertexes);
	in->vertex->Unmap(0, nullptr);

	/* set vertex bufffer view */
	in->vbv.BufferLocation = in->vertex->GetGPUVirtualAddress();
	in->vbv.SizeInBytes = sizeof vertexes;
	in->vbv.StrideInBytes = sizeof vertexes[0];
}

void
create_index_buffer_view(ID3D12Device *dev, render_input *in)
{
	uint16_t indexes[] = {
		0, 1, 2,
		2, 1, 3
	};
	create_upload_buffer(dev, in, sizeof indexes, &in->index);

	void *map;
	in->index->Map(0, nullptr, &map);
	memcpy(map, indexes, sizeof indexes);
	in->index->Unmap(0, nullptr);

	in->ibv.BufferLocation = in->index->GetGPUVirtualAddress();
	in->ibv.SizeInBytes = sizeof indexes;
	in->ibv.Format = DXGI_FORMAT_R16_UINT;
}

void
compile_hlsl(const wchar_t *filename, const char *entry, const char *target,
	     ID3DBlob **code)
{
	HRESULT hr;
	ComPtr<ID3DBlob> error;

#ifdef _DEBUG
	uint32_t opt1 = D3DCOMPILE_DEBUG| D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	uint32_t opt1 = 0;
#endif
	hr = D3DCompileFromFile(filename, nullptr,
				D3D_COMPILE_STANDARD_FILE_INCLUDE,
				entry, target, opt1, 0, code, &error);
	if (FAILED(hr))
		if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			RCK(hr, "D3DCompileFromFile:%x");
		} else {
			std::string errstr;
			errstr.resize(error->GetBufferSize());
			std::copy_n((char*)error->GetBufferPointer(),
				    error->GetBufferSize(), errstr.begin());
			throw std::runtime_error(errstr.c_str());
		}
}

void
define_input_layout(render_pipeline *pipe)
{
	static D3D12_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
		 D3D12_APPEND_ALIGNED_ELEMENT,
		 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
		  D3D12_APPEND_ALIGNED_ELEMENT,
		  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
	pipe->input_layout = layout;
	pipe->layout_count = _countof(layout);
}

void
set_sampler_desc(D3D12_STATIC_SAMPLER_DESC *sd)
{
	sd->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sd->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sd->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sd->BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sd->Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sd->MaxLOD = D3D12_FLOAT32_MAX;		    // max MipMap
	sd->MinLOD = 0.0f;			    // min MipMap
	sd->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sd->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
}

void
create_rootsignature(ID3D12Device *dev, render_pipeline *pipe)
{
	HRESULT hr;
	ComPtr<ID3DBlob> error;

	D3D12_DESCRIPTOR_RANGE range = {};
	range.NumDescriptors = 1; // one texture
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; 
	range.BaseShaderRegister = 0;
	range.OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER param = {};
	param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param.DescriptorTable.pDescriptorRanges = &range;
	param.DescriptorTable.NumDescriptorRanges = 1;
	param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC sd = {};
	set_sampler_desc(&sd);

	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	desc.pParameters = &param;
	desc.NumParameters = 1;
	desc.pStaticSamplers = &sd;
	desc.NumStaticSamplers = 1;

	ComPtr<ID3DBlob> serial;
	hr = D3D12SerializeRootSignature(&desc,
					 D3D_ROOT_SIGNATURE_VERSION_1_0,
					 &serial, &error);
	RCK(hr, "D3D12SerializeRootSignature:%x");

	hr = dev->CreateRootSignature(0,
				      serial->GetBufferPointer(),
				      serial->GetBufferSize(),
				      IID_PPV_ARGS(&pipe->sig));
	RCK(hr, "CreateRootSignature:%x");
}                                   

D3D12_RENDER_TARGET_BLEND_DESC
create_blend_desc(void)
{
	D3D12_RENDER_TARGET_BLEND_DESC desc = {};
	desc.BlendEnable = false;
	desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	desc.LogicOpEnable = false;

	return desc;
}

void
create_pipeline(ID3D12Device *dev, render_pipeline *pipe)
{
	HRESULT hr;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = pipe->sig.Get();

	/** SHADERS **/
	desc.VS.pShaderBytecode = pipe->vs_code->GetBufferPointer();
	desc.VS.BytecodeLength = pipe->vs_code->GetBufferSize();
	desc.PS.pShaderBytecode = pipe->ps_code->GetBufferPointer();
	desc.PS.BytecodeLength = pipe->ps_code->GetBufferSize();
	
	desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	/** BLENDING **/
	desc.BlendState.AlphaToCoverageEnable = false;
	desc.BlendState.IndependentBlendEnable = false;
	desc.BlendState.RenderTarget[0] = create_blend_desc();

	/** RASTERIZER **/
	desc.RasterizerState.MultisampleEnable = false;
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	desc.RasterizerState.DepthClipEnable = true;
	desc.RasterizerState.FrontCounterClockwise = false;
	desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	desc.RasterizerState.SlopeScaledDepthBias =
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	desc.RasterizerState.AntialiasedLineEnable = false;
	desc.RasterizerState.ForcedSampleCount = 0;
	desc.RasterizerState.ConservativeRaster =
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	/** DEPTH STENCIL **/
	desc.DepthStencilState.DepthEnable = false;
	desc.DepthStencilState.StencilEnable = false;

	/** INPUT LAYOUT **/
	desc.InputLayout.pInputElementDescs = pipe->input_layout;
	desc.InputLayout.NumElements = pipe->layout_count;

	/** MISCS **/
	desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	hr = dev->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipe->pipe));
	RCK(hr, "CreateGraphicsPipelineState:%x");
}

void
setup_viewport(D3D12_VIEWPORT *vp, uint32_t width, uint32_t height)
{
	vp->Width = width;
	vp->Height = height;
	vp->TopLeftX = 0;
	vp->TopLeftY = 0;
	vp->MaxDepth = 1.0f;
	vp->MinDepth = 0.0f;
}

void
setup_scisor(D3D12_RECT *sr, size_t width, size_t height)
{
	sr->top = 0;
	sr->left = 0;
	sr->right = (uint32_t)width;
	sr->bottom = (uint32_t)height;
}

void
render_core(render *render, uint32_t frame)
{
	HRESULT hr;

	ComPtr<IDXGISwapChain3> schain;
	hr = render->out.schain.As(&schain);
	RCK(hr, "ComPtr::AS:%x");
	uint32_t index = schain->GetCurrentBackBufferIndex();

#ifndef RESET_LAST
	render->cmd.alloc->Reset();
	render->cmd.list->Reset(render->cmd.alloc.Get(), nullptr);
#endif

	D3D12_RESOURCE_BARRIER *barrier =
		transition_barrier(render->out.back_buff[index].Get());
	transition_barrier_state(barrier,
				 D3D12_RESOURCE_STATE_PRESENT,
				 D3D12_RESOURCE_STATE_RENDER_TARGET);

	render->cmd.list->ResourceBarrier(1, barrier);
	render->cmd.list->SetPipelineState(render->pipe.pipe.Get());


	D3D12_CPU_DESCRIPTOR_HANDLE handle =
		render->out.rtvheap->GetCPUDescriptorHandleForHeapStart();
	uint32_t incr = (render->dev->GetDescriptorHandleIncrementSize(
				 D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	handle.ptr += (size_t)incr * index;
	render->cmd.list->OMSetRenderTargets(1, &handle, false, nullptr);

	float r, g, b;
	r = (float)((frame >> 16) & 0xff) / 255.0f;
	g = (float)((frame >>  8) & 0xff) / 255.0f;
	b = (float)((frame >>  0) & 0xff) / 255.0f;

	float color[] = {r, g, b, 1.0f}; /* R, G, B, A */
	render->cmd.list->ClearRenderTargetView(handle, color, 0, nullptr);

	render->cmd.list->RSSetViewports(1, &render->pipe.view);
	render->cmd.list->RSSetScissorRects(1, &render->pipe.scissor);
	render->cmd.list->SetGraphicsRootSignature(render->pipe.sig.Get());

	render->cmd.list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	render->cmd.list->IASetVertexBuffers(0, 1, &render->vsin.vbv);
	render->cmd.list->IASetIndexBuffer(&render->vsin.ibv);

	render->cmd.list->DrawIndexedInstanced(6, 1, 0, 0, 0);

	transition_barrier_state(barrier,
				 D3D12_RESOURCE_STATE_RENDER_TARGET,
				 D3D12_RESOURCE_STATE_PRESENT);

	render->cmd.list->ResourceBarrier(1, barrier);

	render->cmd.list->Close();

	ID3D12CommandList *cmd_lists[] = {render->cmd.list.Get()};
	render->cmd.queue->ExecuteCommandLists(1, cmd_lists);
	render->cmd.queue->Signal(render->out.fence.Get(),
				  ++render->out.fvalue);

	if (render->out.fence->GetCompletedValue() != render->out.fvalue) {
		HANDLE ev = CreateEvent(nullptr, false, false, nullptr);
		render->out.fence->SetEventOnCompletion(render->out.fvalue, ev);
		WaitForSingleObject(ev, INFINITE);
		CloseHandle(ev);
	}

#ifdef RESET_LAST
	render->cmd.alloc->Reset();
	render->cmd.list->Reset(render->cmd.alloc.Get(), nullptr);
#endif

	render->out.schain->Present(1, 0);
}
