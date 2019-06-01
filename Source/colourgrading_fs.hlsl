///Colour-grading post-processing effect

#pragma region Inputs

// Texture and sampler registers
Texture2D texture0 : register(t0);
Texture2D logLut : register(t1);
Texture2D depthTexture : register(t2);
SamplerState Sampler0 : register(s0);
SamplerState SamplerLut : register(s1);

cbuffer ColourGrading : register(b0) {
	float3 fogColour;
	float strength;//0..1
	float3 logLutParams;//x: 1/lut_width, y: 1/lut_height, z: lut_height-1
	float vignette;//0: no vignette to 1: full vignette
	float chromaticAberrationStrength;//0..1
	float chromaticAberrationDistance;//0..1
	float2 pad;
}

struct FS_IN {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

#pragma endregion Inputs


#define LOGC_A 5.555556f
#define LOGC_B 0.047996f
#define LOGC_C 0.244161f
#define LOGC_D 0.386036f

///Converts from linear space to log space (http://www.vocas.nl/webfm_send/964)
float3 linear_to_logc(float3 input) {
	return LOGC_C * log10(LOGC_A * input + LOGC_B) + LOGC_D;
}

///Converts a colour from linear to gamma space (http://chilliant.blogspot.com.au/2012/08/srgb-approximations-for-hlsl.html?m=1)
float3 linear_to_gamma(float3 input) {
	input = max(input, (0.0f).xxx);
	return max(1.055f * pow(input, 0.416666667f) - 0.055f, (0.0f).xxx);
}

///Converts a colour from gamma space to linear (http://chilliant.blogspot.com.au/2012/08/srgb-approximations-for-hlsl.html?m=1)
float3 gamma_to_linear(float3 input) {
	return input * (input * (input * 0.305306011f + 0.682171111f) + 0.012522878f);
}

///LUT grading (from Unity's Post Processing Stack)
float3 lut2d(Texture2D tex, SamplerState samp, float3 uvw, float3 scaleOffset) {
	uvw.z *= scaleOffset.z;
	uvw.y *= -1;// <-- okay no screw this, i spent a day trying to figure out why this whole thing wasnt working and that fixed it
	float shift = floor(uvw.z);
	uvw.xy = uvw.xy * scaleOffset.z * scaleOffset.xy + scaleOffset.xy * 0.5;
	uvw.x += shift * scaleOffset.y;
	uvw.xyz = lerp(tex.Sample(samp, uvw.xy).rgb, tex.Sample(samp, uvw.xy + float2(scaleOffset.y, 0)).rgb, uvw.z - shift);
	return uvw;
}


float4 main(FS_IN input) : SV_TARGET{
	// Sample the pixel color from the texture using the sampler at this texture coordinate location.
	float3 color = texture0.Sample(Sampler0, input.tex).rgb;

	float distanceFromCenter = length(abs(input.tex - (0.5f).xx));//0 at the center, 1 towards edge of screen
	distanceFromCenter *= distanceFromCenter * distanceFromCenter;//cube it!

	// Chromatic aberration
	float ca = distanceFromCenter*chromaticAberrationDistance;
	float red = texture0.Sample(Sampler0, input.tex + ca).r;
	float blue = texture0.Sample(Sampler0, input.tex - ca).b;
	color = lerp(color, float3(red, color.g, blue), chromaticAberrationStrength);

	// Quadratic fog
	float depth = 1.0f-depthTexture.Sample(Sampler0, input.tex).r;//0 at NEAR, 1 at FAR
	color = lerp(color, fogColour, depth * depth);

	// Vignette
	color -= distanceFromCenter * vignette;//remove some brightness from pixels far from center

	// LUT (tonemapping)
	//To gamma space
	color = max(saturate(color), (0.005f).xxx);//removes artifacts in very dark colours in lut
	color = linear_to_gamma(color);
	float3 colorGraded = lut2d(logLut, SamplerLut, color, logLutParams);
	//Back to linear
	colorGraded = gamma_to_linear(colorGraded);
	color = lerp(color, colorGraded, strength);//Vary how strong we want the effect

	return float4(color.rgb, 1);
}