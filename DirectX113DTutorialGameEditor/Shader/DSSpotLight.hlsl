#include "DeferredLight.hlsli"
#include "iDSCBs.hlsli"

//                  £¯|¡¬                      ^
//                £¯  |  ¡¬                    |
//              £¯ ¥è  |    ¡¬                  |
//            £¯   2  |      ¡¬                |                  
//          £¯        |        ¡¬              | 
//        £¯          |          ¡¬            |  == Range
//      £¯            |            ¡¬          |                          ¥è
//    £¯              |              ¡¬        |     => ConeRadius = tan( - ) * Range
//  £¯   ConeRadius   |                ¡¬      |                          2
// _______________________________________     v
// \    ¥è             |                  /     ^                                     ¥è
//   -  4             |                -       |  == SquashedHemisphereHeight = tan( - ) * ConeRadius
//      -    _   _    _    _   _    -          v                                     4

[domain("quad")]
DS_SPOT_LIGHT_OUTPUT main(HS_CONSTANT_DATA_OUTPUT ConstantData, float2 Domain : SV_DomainLocation, const OutputPatch<HS_SPOT_LIGHT_OUTPUT, 1> ControlPoints)
{
	DS_SPOT_LIGHT_OUTPUT Output;

	float2 ClipSpaceDomain = Domain * 2.0 - 1.0; // ClipSpaceDomain = [-1.0, +1.0]
	float2 ClipSpaceDomainAbsolute = abs(ClipSpaceDomain); // ClipSpaceDomainAbsolute = [0, +1.0]
	float MaxLength = max(ClipSpaceDomainAbsolute.x, ClipSpaceDomainAbsolute.y);
	float4 WorldVertexPosition = float4(0, 0, 0, 1);
	float ThetaDiv2 = ControlPoints[0].Theta * 0.5;
	float ConeRadius = tan(ThetaDiv2) * ControlPoints[0].Range;
	float ThetaDiv4 = ThetaDiv2 * 0.5;

	if (ControlPoints[0].Direction.w == 2.0)
	{
		// Cone part

		// Direction(Orientation) changes the FrameY vector
		// NewFrameY = -Direction

		float3 NewFrameY = normalize(-ControlPoints[0].Direction.xyz);
		float3 NewFrameX = normalize(cross(NewFrameY, float3(0, 0, -1)));
		float3 NewFrameZ = cross(NewFrameX, NewFrameY);
		if (NewFrameY.y < 0) NewFrameZ = -NewFrameZ;

		float4x4 NewFrames = transpose(float4x4(float4(NewFrameX, 0), float4(NewFrameY, 0), float4(NewFrameZ, 0), float4(0, 0, 0, 1)));

		float2 NoralizedXY = normalize(ClipSpaceDomain);
		float Length = length(NoralizedXY);
		if (ClipSpaceDomainAbsolute.x < 1.0 && ClipSpaceDomainAbsolute.y < 1.0)
		{
			WorldVertexPosition = float4(0, 0, 0, 1);
		}
		else
		{
			if (NewFrameY.y < 0) ClipSpaceDomain.xy = ClipSpaceDomain.yx; // @important: change winding

			float2 FinalXY = NoralizedXY * ConeRadius;
			WorldVertexPosition = float4(FinalXY.x, -ControlPoints[0].Range, FinalXY.y, 1);
		}

		WorldVertexPosition = mul(WorldVertexPosition, NewFrames);
		WorldVertexPosition.xyz += ControlPoints[0].WorldPosition.xyz;
	}
	else
	{
		// Squashed hemisphere part

		// Direction(Orientation) changes the FrameY vector
		// NewFrameY = Direction
		
		float3 NewFrameY = normalize(ControlPoints[0].Direction.xyz);
		float3 NewFrameX = normalize(cross(NewFrameY, float3(0, 0, -1)));
		float3 NewFrameZ = cross(NewFrameX, NewFrameY);
		if (NewFrameY.y < 0) NewFrameZ = -NewFrameZ;

		float4x4 NewFrames = transpose(float4x4(float4(NewFrameX, 0), float4(NewFrameY, 0), float4(NewFrameZ, 0), float4(0, 0, 0, 1)));

		if (NewFrameY.y < 0) ClipSpaceDomain.xy = ClipSpaceDomain.yx; // @important: change winding

		WorldVertexPosition = float4(normalize(float3(ClipSpaceDomain.x, 1.0 - MaxLength, ClipSpaceDomain.y)) * ConeRadius, 1);
		WorldVertexPosition.y *= ThetaDiv4;
		WorldVertexPosition.y += ControlPoints[0].Range;
		
		WorldVertexPosition = mul(WorldVertexPosition, NewFrames);
		WorldVertexPosition.xyz += ControlPoints[0].WorldPosition.xyz;
	}
	
	Output.Position = mul(WorldVertexPosition, ViewProjection);
	Output.WorldPosition = ControlPoints[0].WorldPosition; // @important: light's center position in world space
	Output.Direction = ControlPoints[0].Direction;
	Output.Color = ControlPoints[0].Color;
	Output.InverseRange = 1.0 / (ControlPoints[0].Range / cos(ThetaDiv2)); // Range is now radius of outer sphere
	Output.CosThetaDiv2 = cos(ThetaDiv2);
	return Output;
}
