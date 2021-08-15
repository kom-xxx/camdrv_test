struct vsout {
	float4 svpos :SV_POSITION;
	float2 uv :TEXCOORD;
};

vsout vs_main(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	vsout output;
	output.svpos = pos;
	output.uv = uv;
	return output;
}

float4 ps_main(vsout input) : SV_TARGET
{
	return float4(nput.uv, 1, 1);
}
