static const float4x4 KMatrixIdentity = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
static const int KBoneMatrixMaxCount = 60;
static const float4 KUpDirection = float4(0, 1, 0, 0);
static const float KPIDIV2 = 1.57079f;
static const float KPI = 3.14159f;
static const float K2PI = 6.28318f;
static const float K4PI = 12.56637f;

struct VS_INPUT
{
	float4 Position		: POSITION;
	float4 Color		: COLOR;
	float3 UV			: TEXCOORD;
	float4 Normal		: NORMAL;
	float4 Tangent		: TANGENT;

	// Instance
	float4 InstanceWorld0	: INSTANCEWORLD0;
	float4 InstanceWorld1	: INSTANCEWORLD1;
	float4 InstanceWorld2	: INSTANCEWORLD2;
	float4 InstanceWorld3	: INSTANCEWORLD3;
};

struct VS_INPUT_ANIMATION
{
	float4	Position	: POSITION;
	float4	Color		: COLOR;
	float3	UV			: TEXCOORD;
	float4	Normal		: NORMAL;
	float4	Tangent		: TANGENT;

	uint4	BoneIndex	: BLENDINDICES;
	float4	BoneWeight	: BLENDWEIGHT;
};

struct VS_LINE_INPUT
{
	float4	Position	: POSITION;
	float4	Color		: COLOR;
};

struct VS_PARTICLE_INPUT
{
	float4	Position	: POSITION;
	float4	TexColor	: TEXCOLOR;
	float	Rotation	: ROTATION;
	float2	Scaling		: SCALING;
};

struct VS_OUTPUT
{
	float4	Position		: SV_POSITION;
	float4	WorldPosition	: POSITION;
	float4	Color			: COLOR;
	float3	UV				: TEXCOORD;
	float4	WorldNormal		: NORMAL;
	float4	WorldTangent	: TANGENT;
	float4	WorldBitangent	: BITANGENT;
	int		bUseVertexColor : BOOL;
};

struct VS_LINE_OUTPUT
{
	float4 Position			: SV_POSITION;
	float4 Color			: COLOR;
};

struct VS_PARTICLE_OUTPUT
{
	float4	Position		: SV_POSITION;
	float4	WorldPosition	: POSITION;
	float4	TexColor		: TEXCOLOR;
	float	Rotation		: ROTATION;
	float2	Scaling			: SCALING;
};

#define HS_OUTPUT VS_OUTPUT
#define DS_OUTPUT HS_OUTPUT

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]	: SV_TessFactor;
	float InsideTessFactor : SV_InsideTessFactor;
};

float4 CalculateBitangent(float4 Normal, float4 Tangent)
{
	return float4(normalize(cross(Normal.xyz, Tangent.xyz)), 0);
}

float4 CalculateAmbient(float4 AmbientColor, float3 AmbientLightColor, float AmbientLightIntensity)
{
	return float4(AmbientColor.xyz * AmbientLightColor * AmbientLightIntensity, AmbientColor.a);
}

float4 CalculateDirectional(float4 DiffuseColor, float4 SpecularColor, float SpecularExponent, float SpecularIntensity,
	float4 LightColor, float4 LightDirection, float4 ToEye, float4 Normal)
{
	float NDotL = saturate(dot(LightDirection, Normal));
	float4 PhongDiffuse = DiffuseColor * LightColor * NDotL;

	float4 H = normalize(ToEye + LightDirection);
	float NDotH = saturate(dot(H, Normal));
	float SpecularPower = pow(NDotH, SpecularExponent);
	float4 BlinnSpecular = SpecularColor * LightColor * SpecularPower * SpecularIntensity;

	float4 Result = PhongDiffuse + BlinnSpecular;
	return Result;
}

float4 GetBezier(float4 P1, float4 P2, float4 P3, float4 N1, float4 N2, float4 N3, float3 uvw)
{
	float4 w12 = dot((P2 - P1), N1);
	float4 w21 = dot((P1 - P2), N2);

	float4 w23 = dot((P3 - P2), N2);
	float4 w32 = dot((P2 - P3), N3);

	float4 w31 = dot((P1 - P3), N3);
	float4 w13 = dot((P3 - P1), N1);

	float4 b300 = P1;
	float4 b030 = P2;
	float4 b003 = P3;

	float4 b210 = (2 * P1 + P2 - w12 * N1) / 3;
	float4 b120 = (2 * P2 + P1 - w21 * N2) / 3;

	float4 b021 = (2 * P2 + P3 - w23 * N2) / 3;
	float4 b012 = (2 * P3 + P2 - w32 * N3) / 3;

	float4 b102 = (2 * P3 + P1 - w31 * N3) / 3;
	float4 b201 = (2 * P1 + P3 - w13 * N1) / 3;

	float4 E = (b210 + b120 + b021 + b012 + b102 + b201) / 6;
	float4 V = (b300 + b030 + b003) / 3;

	float4 b111 = E + (E - V) / 2;

	float u = uvw.x;
	float v = uvw.y;
	float w = uvw.z;
	float4 Bezier = pow(u, 3) * b300 +
		3 * pow(u, 2) * v * b210 +
		3 * pow(u, 2) * w * b201 +
		3 * u * pow(v, 2) * b120 +
		3 * u * pow(w, 2) * b102 +
		6 * u * v * w * b111 +
		pow(v, 3) * b030 +
		3 * pow(v, 2) * w * b021 +
		3 * v * pow(w, 2) * b012 +
		pow(w, 3) * b003;

	return Bezier;
}