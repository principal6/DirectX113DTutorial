struct GBufferOutput
{
	float4 BaseColor_Rough	: SV_Target0;
	float4 Normal 			: SV_Target1;
	float4 MetalAO			: SV_Target2;
};