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
#include "utilities.h"
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
        throw std::runtime_error("create_device: NVIDIA adpter not found");
    hr = D3D12CreateDevice(env->adapter.Get(),
                           REQUIRED_D3D12_FEATURE, IID_PPV_ARGS(dev));
    RCK(hr, "D3D12CreateDevice:%x");
}

/**
 ** D3D12 UTILITIES
 **/
D3D12_RESOURCE_BARRIER
transition_barrier(ID3D12Resource *res,
                   D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after,
                   D3D12_RESOURCE_BARRIER_FLAGS flags, uint32_t subres)
{
    D3D12_RESOURCE_BARRIER barrier = {
        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        flags,
        {res, subres, before, after}
    };

#if 0
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = flags;
    barrier.Transition.pResource = res;
    barrier.Transition.Subresource = subres;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
#endif

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
execute_command_list(render_cmd_queue *queue, render_command *cmd)
{
    ID3D12CommandList *list[] = {cmd->list.Get()};
    queue->queue->ExecuteCommandLists(1, list);
    queue->queue->Signal(queue->fence.Get(), ++queue->fvalue);

    if (queue->fence->GetCompletedValue() != queue->fvalue) {
        HANDLE ev = CreateEvent(nullptr, false, false, nullptr);
        if (!ev)
            throw std::runtime_error("CreateEvent");

        queue->fence->SetEventOnCompletion(queue->fvalue, ev);
        WaitForSingleObject(ev, INFINITE);
        CloseHandle(ev);
    }
}

/***
 *** COMMAND-QUEUE AND FENCE
 ***/
void
create_command_queue(ID3D12Device *dev, render_cmd_queue *queue,
                     D3D12_COMMAND_LIST_TYPE type,
                     D3D12_COMMAND_QUEUE_FLAGS flags,
                     D3D12_COMMAND_QUEUE_PRIORITY prio)
{
    HRESULT hr;

    D3D12_COMMAND_QUEUE_DESC qdesc = {
        type,                   /* type */
        prio,                   /* priority */
        flags,                  /* flags */
        0                       /* nodemask */
    };
    hr = dev->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&queue->queue));
    RCK(hr, "CreateCommandQueue:%x");
}

void
create_fence(ID3D12Device *dev, render_cmd_queue *queue)
{
    HRESULT hr;

    queue->fvalue = 0;

    hr = dev->CreateFence(queue->fvalue, D3D12_FENCE_FLAG_NONE,
                          IID_PPV_ARGS(&queue->fence));
    RCK(hr, "CreateFence:%x");
}

/***
 *** SWAP-CHAIN
 ***/
void
create_swap_chain(render_environment *env, render_cmd_queue *queue,
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
    hr = env->factory->CreateSwapChainForHwnd(queue->queue.Get(), env->window,
                                              &sc_desc, nullptr, nullptr,
                                              swap_chain);
    RCK(hr, "CreateSwapChainForHwnd:%x");
}

/***
 *** COMMAND-LIST AND BARRIER
 ***/
void
create_command_list(ID3D12Device *dev, render_command *cmd,
                    D3D12_COMMAND_LIST_TYPE type)

{
    HRESULT hr;

    hr = dev->CreateCommandAllocator(type, IID_PPV_ARGS(&cmd->alloc));
    RCK(hr, "CreateCommandAllocator:%x");

    hr = dev->CreateCommandList(0, type, cmd->alloc.Get(), nullptr,
                                IID_PPV_ARGS(&cmd->list));
    RCK(hr, "CreateCommandList:%x");
    cmd->list->Close();
}

/***
 *** UPLOAD
 ***/
void
create_upload_heap(ID3D12Device *dev, render_heap *heap, size_t size)
{
    HRESULT hr;

    D3D12_HEAP_DESC desc = {
       size,
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
    hr = dev->CreateHeap(&desc, IID_PPV_ARGS(&heap->heap));
    RCK(hr, "CreateHeap(upload):%x");
    heap->offset = 0;
}

void
create_upload_buffer(ID3D12Device *dev, render_heap *heap,
		     size_t size, ID3D12Resource **res)
{
    HRESULT hr;

    D3D12_RESOURCE_DESC desc = {
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
    hr = dev->CreatePlacedResource(heap->heap.Get(), heap->offset, &desc,
                                   D3D12_RESOURCE_STATE_GENERIC_READ,
                                   nullptr, IID_PPV_ARGS(res));
    RCK(hr, "CreatePlacedResource(heap):%x");

    D3D12_RESOURCE_ALLOCATION_INFO
        alloc_info = dev->GetResourceAllocationInfo(0, 1, &desc);
    heap->offset += alloc_info.SizeInBytes;
    heap->count += 1;
}

void
copy_to_upload_buffer(ID3D12Resource *dst, void *src, size_t size)
{
    HRESULT hr;

    void *map;
    hr = dst->Map(0, nullptr, &map);
    RCK(hr, "Map:%x");
    memcpy(map, src, size);
    dst->Unmap(0, nullptr);
}

/***
 *** TEXTURE
 ***/
void
create_texture_heap(ID3D12Device *dev, render_heap *heap, size_t size)
{
    HRESULT hr;

    D3D12_HEAP_DESC desc = {
        size,
        {
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            1,
            1
        },
        KiB(64),		/* alignment */
        D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES
    };
    hr = dev->CreateHeap(&desc, IID_PPV_ARGS(&heap->heap));
    RCK(hr, "CreateHeap(texture):%x");
    heap->offset = 0;
}

void
create_texture_buffer(ID3D12Device *dev, render_heap *heap,
                      size_t width, size_t height, ID3D12Resource **res)
{
    HRESULT hr;

    D3D12_RESOURCE_DESC desc = {
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        0,
        width,
        (uint32_t)height,
        1,
        0,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        {1, 0},
	D3D12_TEXTURE_LAYOUT_UNKNOWN,
        D3D12_RESOURCE_FLAG_NONE
    };
    hr = dev->CreatePlacedResource(heap->heap.Get(), heap->offset, &desc,
                                   D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                   IID_PPV_ARGS(res));
    RCK(hr, "CreatePlacedResource:%x");

    D3D12_RESOURCE_ALLOCATION_INFO
        alloc_info = dev->GetResourceAllocationInfo(0, 1, &desc);
    heap->offset += alloc_info.SizeInBytes;
    heap->count += 1;
}

void
upload_texture(ID3D12Device *dev, render_command *cmd, render_cmd_queue *queue,
               ID3D12Resource *dst, ID3D12Resource *src)
{
    HRESULT hr;

    D3D12_RESOURCE_DESC ddesc = dst->GetDesc();
    D3D12_RESOURCE_DESC sdesc = src->GetDesc();
    D3D12_HEAP_PROPERTIES dprops, sprops;
    D3D12_HEAP_FLAGS dflags, sflags;
    hr = dst->GetHeapProperties(&dprops, &dflags);
    hr = src->GetHeapProperties(&sprops, &sflags);
    
    D3D12_TEXTURE_COPY_LOCATION dst_loc = {
        dst,                    /* resource */
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        0
    };

    D3D12_RESOURCE_DESC desc = dst->GetDesc();

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp;
    uint32_t nr_line;
    uint64_t line_len, size;
    dev->GetCopyableFootprints(&desc, 0, 1, 0, &fp, &nr_line, &line_len, &size);

    D3D12_TEXTURE_COPY_LOCATION src_loc = {
        src,
        D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
        fp
    };

    cmd->alloc->Reset();
    cmd->list->Reset(cmd->alloc.Get(), nullptr);

    cmd->list->CopyTextureRegion(&dst_loc, 0, 0, 0, &src_loc, nullptr);

    D3D12_RESOURCE_BARRIER barrier = 
        transition_barrier(dst, D3D12_RESOURCE_STATE_COPY_DEST,
                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmd->list->ResourceBarrier(1, &barrier);
    cmd->list->Close();

    execute_command_list(queue, cmd);
}

void
setup_texture(ID3D12Device *dev, render_heap *upload, render_heap *texture,
              render_command *cmd, render_cmd_queue *queue,
              render_texture *tex, size_t width, size_t height)
{
    size_t size = width * height * sizeof (uint32_t);
    
    for (size_t i = 0; i < NR_TEXTURE; ++i) {
        std::vector<uint32_t>
            image = create_texture_image(width, height, i == 1);
        tex->image.push_back(image);
        create_upload_buffer(dev, upload, size, &tex->upload_buffer[i]);
        copy_to_upload_buffer(tex->upload_buffer[i].Get(),
                              (void *)tex->image[i].data(), size);

        create_texture_buffer(dev, texture, width, height,
                              &tex->texture_buffer[i]);
#ifndef COPY_ON_RENDER_CORE
        upload_texture(dev, cmd, queue, tex->texture_buffer[i].Get(),
                       tex->upload_buffer[i].Get());
#endif
    }
}

/***
 *** READBACK BUFFER
 ***/
void
create_readback_heap(ID3D12Device *dev, render_heap *heap, size_t size)
{
    HRESULT hr;

    D3D12_HEAP_DESC desc = {
        size,
        {
            D3D12_HEAP_TYPE_READBACK,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            1,
            1
        },
        KiB(64),
        D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS
    };
    hr = dev->CreateHeap(&desc, IID_PPV_ARGS(&heap->heap));
    RCK(hr, "CreateHeap(readback):%x");
    heap->offset = 0;
}

void
create_readback_buffer(ID3D12Device *dev, render_heap *heap, size_t size,
                       ID3D12Resource **res)
{
    HRESULT hr;

    D3D12_RESOURCE_DESC desc = {
        D3D12_RESOURCE_DIMENSION_BUFFER,
        0,
        size,
        1,
        1,
        1,
        DXGI_FORMAT_UNKNOWN,
        {1, 0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_NONE
    };
    hr = dev->CreatePlacedResource(heap->heap.Get(), heap->offset, &desc,
                                   D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                   IID_PPV_ARGS(res));
    RCK(hr, "CreatePlacedResource:%x");
    D3D12_RESOURCE_ALLOCATION_INFO
        alloc_info = dev->GetResourceAllocationInfo(0, 1, &desc);
    heap->offset += alloc_info.SizeInBytes;
    heap->count += 1;
}

/***
 *** CREATE VIEWS
 ***     view : share resource between CPU and GPU
 ***/
void
create_vbv(ID3D12Device *dev, render_heap *heap, render_input *in)
{
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
    create_upload_buffer(dev, heap, sizeof vertexes, &in->vertex);

    copy_to_upload_buffer(in->vertex.Get(), vertexes,  sizeof vertexes);

    /* set vertex bufffer view */
    in->vbv.BufferLocation = in->vertex->GetGPUVirtualAddress();
    in->vbv.SizeInBytes = sizeof vertexes;
    in->vbv.StrideInBytes = sizeof vertexes[0];
}

void
create_ibv(ID3D12Device *dev, render_heap *heap, render_input *in)
{
    uint16_t indexes[] = {
        0, 1, 2,
        2, 1, 3
    };
    create_upload_buffer(dev, heap, sizeof indexes, &in->index);

    copy_to_upload_buffer(in->index.Get(), indexes, sizeof indexes);

    in->ibv.BufferLocation = in->index->GetGPUVirtualAddress();
    in->ibv.SizeInBytes = sizeof indexes;
    in->ibv.Format = DXGI_FORMAT_R16_UINT;
}

void
create_srv(ID3D12Device *dev, render_texture *tex)
{
    HRESULT hr;

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        NR_TEXTURE,             /* # of descriptor */
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        0                       /* node mask */
    };
    hr = dev->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&tex->desc_heap));
    RCK(hr, "CreateDescriptorHeap(CBV_SRV_UAV):%x");

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
        DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D12_SRV_DIMENSION_TEXTURE2D,
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
    };
    D3D12_TEX2D_SRV tex2d = {
        0,                      /* most detailed mip */
        1,                      /* mip level */
        0,                      /* plane slice */
        0.0F                    /* resource min LOD calmp */
    };
    srv_desc.Texture2D =  tex2d;

    D3D12_CPU_DESCRIPTOR_HANDLE
        handle = tex->desc_heap->GetCPUDescriptorHandleForHeapStart();
    uint32_t incr = (dev->GetDescriptorHandleIncrementSize(
                         D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    for (size_t i = 0; i < NR_TEXTURE; ++i) {
        dev->CreateShaderResourceView(tex->texture_buffer[i].Get(),
                                      &srv_desc, handle);
        RCK(hr, "CreateShaderResourceView:%x");
        handle.ptr += incr;
    }
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

/**
 ** HLSL compiler
 **/
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

/**
 ** ROOT SIGNATURE AND PIPELINE STATE
 **/
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

/**
 ** VIEWPORT AND SCISSOR RECT
 **/
void
setup_viewport(D3D12_VIEWPORT *vp, uint32_t width, uint32_t height)
{
    vp->Width = (float)width;
    vp->Height = (float)height;
    vp->TopLeftX = 0;
    vp->TopLeftY = 0;
    vp->MaxDepth = 1.0f;
    vp->MinDepth = 0.0f;
}

void
setup_scissor(D3D12_RECT *sr, size_t width, size_t height)
{
    sr->top = 0;
    sr->left = 0;
    sr->right = (uint32_t)width;
    sr->bottom = (uint32_t)height;
}

/**
 ** RENDERING MAIN PROCESS
 **/
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

    D3D12_RESOURCE_BARRIER barrier =
        transition_barrier(render->out.back_buff[index].Get(),
                           D3D12_RESOURCE_STATE_PRESENT,
                           D3D12_RESOURCE_STATE_RENDER_TARGET);

    render->cmd.list->ResourceBarrier(1, &barrier);
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
    render->cmd.list->IASetVertexBuffers(0, 1, &render->in.vbv);
    render->cmd.list->IASetIndexBuffer(&render->in.ibv);

    render->cmd.list->DrawIndexedInstanced(6, 1, 0, 0, 0);

    transition_barrier_state(&barrier,
                             D3D12_RESOURCE_STATE_RENDER_TARGET,
                             D3D12_RESOURCE_STATE_PRESENT);

    render->cmd.list->ResourceBarrier(1, &barrier);

    render->cmd.list->Close();

    execute_command_list(&render->queue, &render->cmd);

#ifdef RESET_LAST
    render->cmd.alloc->Reset();
    render->cmd.list->Reset(render->cmd.alloc.Get(), nullptr);
#endif

    render->out.schain->Present(1, 0);
}
