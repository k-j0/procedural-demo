///Tonemapping post-processing effect
///This is quite a huge effect and takes a massive toll on gpu so the usage should be onto a small LUT (256x16) and then colour-grade the full render using the LUT

///Using: Zhang D., Zheng B. (2017) GPU-Based Post-Processing Color Grading Algorithms in Real-Time Rendering for Mobile Commerce Service User. In: Pan Z., Cheok A., Müller W., Zhang M. (eds) Transactions on Edutainment XIII. Lecture Notes in Computer Science, vol 10092. Springer, Berlin, Heidelberg
///Using: https://github.com/ampas/aces-dev

///accessors for hue-saturation-value colours:
#define HUE(f) f.x
#define SAT(f) f.y
#define VAL(f) f.z

#pragma region Inputs

// Texture and sampler registers
Texture2D texture0 : register(t0);
SamplerState Sampler0 : register(s0);

#define TONEMAP_NONE 0
#define TONEMAP_ACES 1
#define TONEMAP_NEUTRAL 2

cbuffer ColourGrading : register(b0) {
	float brightness;//-1..1
	float contrast;//-1..1
	float hue;//0..1
	float saturation;//0..1
	float value;//0..2
	float tonemapping;//0, 1, 2
	float exposure;//0..2
	float padding;
}

struct FS_IN {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

#pragma endregion Inputs

#pragma region Constants

///Constants
#define EPSILON 1e-10
#define PI 3.141592f
#define HALF_MAX 65504.0f//according to https://en.wikipedia.org/wiki/float-precision_floating-point_format
#define ROOT_OF_3 1.73205f //sqrt(3.0f)
#define CONTRAST_FACTOR 1.01568f
#define YC_RADIUS_WEIGHT 1.75f
#define RRT_GLOW_GAIN 0.05f
#define RRT_GLOW_MID 0.08f
#define RRT_RED_HUE 0.0f
#define RRT_RED_WIDTH 135.0f
#define RRT_RED_PIVOT 0.03f
#define RRT_RED_SCALE 0.82f
#define RRT_SAT_FACTOR 0.96f
#define CINEMA_WHITE 48.0f
#define CINEMA_BLACK CINEMA_WHITE/2400.0f
#define DIM_SURROUND_GAMMA 0.9811f
#define ODT_SAT_FACTOR 0.93f
#define AP1_RGB2Y float3(0.272229, 0.674082, 0.0536895)
#define LOGC_A 5.555556f
#define LOGC_B 0.047996f
#define LOGC_C 0.244161f
#define LOGC_D 0.386036f

///Pretransposed matrices (https://github.com/ampas/aces-dev/blob/master/transforms/ctl/README-MATRIX.md)
static const float3x3 sRGB_2_AP0 = {//rgb to AP0 Aces
	0.4397010, 0.3829780, 0.1773350,
	0.0897923, 0.8134230, 0.0967616,
	0.0175440, 0.1115440, 0.8707040
};

static const float3x3 AP0_2_sRGB = {//AP0 Aces to rgb
	2.52169, -1.13413, -0.38756,
	-0.27648, 1.37272, -0.09624,
	-0.01538, -0.15298, 1.16835,
};

static const float3x3 AP1_2_AP0_MAT = {//AP1 Aces to AP0
	0.6954522414, 0.1406786965, 0.1638690622,
	0.0447945634, 0.8596711185, 0.0955343182,
	-0.0055258826, 0.0040252103, 1.0015006723
};

static const float3x3 AP0_2_AP1_MAT = {//AP0 Aces to AP1
	1.4514393161, -0.2365107469, -0.2149285693,
	-0.0765537734,  1.1762296998, -0.0996759264,
	0.0083161484, -0.0060324498,  0.9977163014
};

static const float3x3 AP1_2_XYZ_MAT = {//AP1 Aces to XYZ
	0.6624541811, 0.1340042065, 0.1561876870,
	0.2722287168, 0.6740817658, 0.0536895174,
	-0.0055746495, 0.0040607335, 1.0103391003
};

static const float3x3 XYZ_2_AP1_MAT = {//xYZ to AP1 Aces
	1.6410233797, -0.3248032942, -0.2364246952,
	-0.6636628587,  1.6153315917,  0.0167563477,
	0.0117218943, -0.0082844420,  0.9883948585
};

static const float3x3 D60_2_D65_CAT = {
	0.98722400, -0.00611327, 0.0159533,
	-0.00759836,  1.00186000, 0.0053302,
	0.00307257, -0.00509595, 1.0816800
};

static const float3x3 XYZ_2_REC709_MAT = {
	3.2409699419, -1.5373831776, -0.4986107603,
	-0.9692436363,  1.8759675015,  0.0415550574,
	0.0556300797, -0.2039769589,  1.0569715142
};

static const float3x3 M = {//Matrix for segmented spline functions
	0.5, -1.0, 0.5,
	-1.0,  1.0, 0.0,
	0.5,  0.5, 0.0
};

#pragma endregion Constants

#pragma region Conversion

///Converts colour from RGB to HSV (0..1, 0..1, 0..1)
float3 rgb_to_hsv(float3 input) {
	//from http://chilliant.blogspot.com/2014/04/rgbhsv-in-hlsl-5.html
	float4 k = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	float4 p = lerp(float4(input.bg, k.wz), float4(input.gb, k.xy), step(input.b, input.g));
	float4 q = lerp(float4(p.xyw, input.r), float4(input.r, p.yzx), step(p.x, input.r));
	float d = q.x - min(q.w, q.y);
	float e = EPSILON;
	return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);

}

///Converts colour from HSV to RGB (0..1, 0..1, 0..1)
float3 hsv_to_rgb(float3 input) {
	//from Unity's shaders source
	float4 k = float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
	float3 p = abs(frac(input.xxx + k.xyz) * 6.0 - k.www);
	return input.z * lerp(k.xxx, saturate(p - k.xxx), input.y);
}

///Converts colour from RGB to ACES2065-1
float3 rgb_to_aces(float3 input) {
	return mul(sRGB_2_AP0, input);
}

///Converts colour from ACES2065-1
float3 aces_to_rgb(float3 input) {
	return mul(AP0_2_sRGB, input);
}

///Gets saturation 0..1 from RGB colour
float rgb_to_sat(float3 input) {
	float mi = min(input.r, min(input.g, input.b));
	float ma = max(input.r, max(input.g, input.b));
	return (max(ma, EPSILON) - max(mi, EPSILON)) / max(ma, 1e-2);
}

///Gets hue 0..360 from RGB colour
float rgb_to_hue(float3 input) {
	if (input.r == input.g && input.g == input.b)
		return 0.0f;
	float hue = (180.0f / PI) * atan2(ROOT_OF_3 * (input.g - input.b), 2.0f * input.r - input.g - input.b);
	if (hue < 0.0f) hue += 360.0f;
	return hue;
}

///Converts colour from RGB to normalized luminance proxy
///YC is Y + K * Chroma
float rgb_to_yc(float3 input) {
	float chroma = sqrt(input.b * (input.b - input.g) + input.g * (input.g - input.r) + input.r * (input.r - input.b));
	return (input.b + input.g + input.r + YC_RADIUS_WEIGHT * chroma) / 3.0f;
}

///Converts luminance to linear code value
float3 y_to_linCV(float3 y, float yMax, float yMin) {
	return (y - yMin) / (yMax - yMin);
}

///Converts from XYZ to xyY (http://www.colorwiki.com/wiki/XyY)
float3 xyz_to_xyy(float3 xyz) {
	float divisor = max(dot(xyz, (1.0).xxx), 1e-4);
	return float3(xyz.xy / divisor, xyz.y);
	//return float3(xyz.x / (xyz.x + xyz.y + xyz.z), xyz.y / (xyz.x + xyz.y + xyz.z), xyz.y);
}

///Converts from xyY to XYZ
float3 xyy_to_xyz(float3 xyy) {
	float m = xyy.z / max(xyy.y, 1e-4);
	float3 xyz = float3(xyy.xz, (1.0f - xyy.x - xyy.y));
	xyz.xz *= m;
	return xyz;
}

///Converts colour from dark surrounding to dim surrounding (colour grading is so cool haha)
float3 darkSurround_to_dimSurround(float3 linearCV) {
	float3 xyz = mul(AP1_2_XYZ_MAT, linearCV);

	float3 xyy = xyz_to_xyy(xyz);
	xyy.z = clamp(xyy.z, 0.0, HALF_MAX);
	xyy.z = pow(xyy.z, DIM_SURROUND_GAMMA);
	xyz = xyy_to_xyz(xyy);

	return mul(XYZ_2_AP1_MAT, xyz);
}

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

#pragma endregion Conversion

#pragma region MathFunctions

///Sigmoid function in 0..1 spanning -2..2
float sigmoidShaper(float input) {
	float t = max(1.0f - abs(input / 2.0f), 0.0f);
	return (1.0f + sign(input) * (1.0f - t * t)) / 2.0f;
}

///Segmented spline functions, from Unity's shaders source
float segmentedSplineC5Fwd(float x) {
	const float coefsLow[6] = { -4.0000000000, -4.0000000000, -3.1573765773, -0.4852499958, 1.8477324706, 1.8477324706 }; // coefs for B-spline between minPoint and midPoint (units of log luminance)
	const float coefsHigh[6] = { -0.7185482425, 2.0810307172, 3.6681241237, 4.0000000000, 4.0000000000, 4.0000000000 }; // coefs for B-spline between midPoint and maxPoint (units of log luminance)
	const float2 minPoint = float2(0.18 * exp2(-15.0), 0.0001); // {luminance, luminance} linear extension below this
	const float2 midPoint = float2(0.18, 0.48); // {luminance, luminance}
	const float2 maxPoint = float2(0.18 * exp2(18.0), 10000.0); // {luminance, luminance} linear extension above this
	const float slopeLow = 0.0; // log-log slope of low linear extension
	const float slopeHigh = 0.0; // log-log slope of high linear extension

	const int N_KNOTS_LOW = 4;
	const int N_KNOTS_HIGH = 4;

	// Check for negatives or zero before taking the log. If negative or zero,
	// set to ACESMIN.1
	float xCheck = x;
	if (xCheck <= 0.0) xCheck = 0.00006103515;

	float logx = log10(xCheck);
	float logy;

	if (logx <= log10(minPoint.x)) {
		logy = logx * slopeLow + (log10(minPoint.y) - slopeLow * log10(minPoint.x));
	}
	else if ((logx > log10(minPoint.x)) && (logx < log10(midPoint.x))) {
		float knot_coord = (N_KNOTS_LOW - 1) * (logx - log10(minPoint.x)) / (log10(midPoint.x) - log10(minPoint.x));
		int j = knot_coord;
		float t = knot_coord - j;

		float3 cf = float3(coefsLow[j], coefsLow[j + 1], coefsLow[j + 2]);
		float3 monomials = float3(t * t, t, 1.0);
		logy = dot(monomials, mul(M, cf));
	}
	else if ((logx >= log10(midPoint.x)) && (logx < log10(maxPoint.x))) {
		float knot_coord = (N_KNOTS_HIGH - 1) * (logx - log10(midPoint.x)) / (log10(maxPoint.x) - log10(midPoint.x));
		int j = knot_coord;
		float t = knot_coord - j;

		float3 cf = float3(coefsHigh[j], coefsHigh[j + 1], coefsHigh[j + 2]);
		float3 monomials = float3(t * t, t, 1.0);
		logy = dot(monomials, mul(M, cf));
	}
	else {
		logy = logx * slopeHigh + (log10(maxPoint.y) - slopeHigh * log10(maxPoint.x));
	}

	return pow(10.0, logy);
}
float segmentedSplineC9Fwd(float x) {
	const float coefsLow[10] = { -1.6989700043, -1.6989700043, -1.4779000000, -1.2291000000, -0.8648000000, -0.4480000000, 0.0051800000, 0.4511080334, 0.9113744414, 0.9113744414 }; // coefs for B-spline between minPoint and midPoint (units of log luminance)
	const float coefsHigh[10] = { 0.5154386965, 0.8470437783, 1.1358000000, 1.3802000000, 1.5197000000, 1.5985000000, 1.6467000000, 1.6746091357, 1.6878733390, 1.6878733390 }; // coefs for B-spline between midPoint and maxPoint (units of log luminance)
	const float2 minPoint = float2(segmentedSplineC5Fwd(0.18 * exp2(-6.5)), 0.02); // {luminance, luminance} linear extension below this
	const float2 midPoint = float2(segmentedSplineC5Fwd(0.18), 4.8); // {luminance, luminance}
	const float2 maxPoint = float2(segmentedSplineC5Fwd(0.18 * exp2(6.5)), 48.0); // {luminance, luminance} linear extension above this
	const float slopeLow = 0.0; // log-log slope of low linear extension
	const float slopeHigh = 0.04; // log-log slope of high linear extension

	const int N_KNOTS_LOW = 8;
	const int N_KNOTS_HIGH = 8;

	// Check for negatives or zero before taking the log. If negative or zero,
	// set to OCESMIN.
	float xCheck = x;
	if (xCheck <= 0.0) xCheck = 1e-4;

	float logx = log10(xCheck);
	float logy;

	if (logx <= log10(minPoint.x))
	{
		logy = logx * slopeLow + (log10(minPoint.y) - slopeLow * log10(minPoint.x));
	}
	else if ((logx > log10(minPoint.x)) && (logx < log10(midPoint.x)))
	{
		float knot_coord = (N_KNOTS_LOW - 1) * (logx - log10(minPoint.x)) / (log10(midPoint.x) - log10(minPoint.x));
		int j = knot_coord;
		float t = knot_coord - j;

		float3 cf = float3(coefsLow[j], coefsLow[j + 1], coefsLow[j + 2]);
		float3 monomials = float3(t * t, t, 1.0);
		logy = dot(monomials, mul(M, cf));
	}
	else if ((logx >= log10(midPoint.x)) && (logx < log10(maxPoint.x)))
	{
		float knot_coord = (N_KNOTS_HIGH - 1) * (logx - log10(midPoint.x)) / (log10(maxPoint.x) - log10(midPoint.x));
		int j = knot_coord;
		float t = knot_coord - j;

		float3 cf = float3(coefsHigh[j], coefsHigh[j + 1], coefsHigh[j + 2]);
		float3 monomials = float3(t * t, t, 1.0);
		logy = dot(monomials, mul(M, cf));
	}
	else
	{ //if (logIn >= log10(maxPoint.x)) {
		logy = logx * slopeHigh + (log10(maxPoint.y) - slopeHigh * log10(maxPoint.x));
	}

	return pow(10.0, logy);
}

float glowFwd(float yc, float glowGain, float glowMid) {
	if (yc <= 2.0f / 3.0f * glowMid)
		return glowGain;
	else if (yc >= 2.0f * glowMid)
		return 0.0f;
	else
		return glowGain * (glowMid / yc - 1.0f / 2.0f);
}

///Centers hue -180..180
float centerHue(float hue, float centerH) {
	float hueCentered = hue - centerH;
	if (hueCentered < -180.0f) hueCentered += 360.0f;
	else if (hueCentered > 180.0f) hueCentered -= 360.0f;
	return hueCentered;
}

#pragma endregion MathFunctions


///Converts OCES colour for use in desktop monitors
///I'm not entirely sure how this works, please refer to ACES documentation
///But basically it outputs a colour that can be displayed onto monitors with specs ~IEC 61966-2-1:1999 (but not exactly)
///Apparently the viewing environment also comes into play?? i had no idea colour grading was so complex but yeah the viewing environment is assumed as a dim surround
float3 ODT_RGBmonitor_100nits_dim(float3 oces) {

	// OCES to RGB rendering space
	float3 rgbPre = mul(AP0_2_AP1_MAT, oces);

	// Apply the tonescale independently in rendering-space RGB
	float3 rgbPost;
	rgbPost.x = segmentedSplineC9Fwd(rgbPre.x);
	rgbPost.y = segmentedSplineC9Fwd(rgbPre.y);
	rgbPost.z = segmentedSplineC9Fwd(rgbPre.z);

	// Scale luminance to linear code value
	float3 linearCV = y_to_linCV(rgbPost, CINEMA_WHITE, CINEMA_BLACK);

	// Apply gamma adjustment to compensate for dim surround
	linearCV = darkSurround_to_dimSurround(linearCV);

	// Apply desaturation to compensate for luminance difference
	linearCV = lerp(dot(linearCV, AP1_RGB2Y).xxx, linearCV, ODT_SAT_FACTOR.xxx);

	// Convert to display primary encoding
	// Rendering space RGB to XYZ
	float3 XYZ = mul(AP1_2_XYZ_MAT, linearCV);

	// Apply CAT from ACES white point to assumed observer adapted white point
	XYZ = mul(D60_2_D65_CAT, XYZ);

	// CIE XYZ to display primaries
	linearCV = mul(XYZ_2_REC709_MAT, XYZ);

	// Handle out-of-gamut values
	return saturate(linearCV);
}


///Reference Rendering Transform (ACES)
float3 rrt(float3 aces) {

	///Glow
	float sat = rgb_to_sat(aces);
	float yc = rgb_to_yc(aces);
	float s = sigmoidShaper((sat - 0.4f) / 0.2f);
	aces *= 1.0f + glowFwd(yc, RRT_GLOW_GAIN * s, RRT_GLOW_MID);//add Glow

																///Red modifier
	float hue = rgb_to_hue(aces);
	float centeredHue = centerHue(hue, RRT_RED_HUE);
	float hueWeight = smoothstep(0.0f, 1.0f, 1.0f - abs(2.0f * centeredHue / RRT_RED_WIDTH));
	hueWeight *= hueWeight;//square

	aces.r += hueWeight * sat * (RRT_RED_PIVOT - aces.r) * (1.0f - RRT_RED_SCALE);

	///ACES to RGB space
	aces = clamp(aces, 0.0f, HALF_MAX);
	float3 rgb = mul(AP0_2_AP1_MAT, aces);
	rgb = clamp(rgb, 0, HALF_MAX);

	///Global desaturation
	rgb = lerp(dot(rgb, AP1_RGB2Y).xxx, rgb, RRT_SAT_FACTOR.xxx);

	///Tonescale
	rgb.r = segmentedSplineC5Fwd(rgb.r);
	rgb.g = segmentedSplineC5Fwd(rgb.g);
	rgb.b = segmentedSplineC5Fwd(rgb.b);

	///Convert to OCES
	return mul(AP1_2_AP0_MAT, rgb);
}






float4 main(FS_IN input) : SV_TARGET{
	// Sample the pixel color from the texture using the sampler at this texture coordinate location.
	float3 color = texture0.Sample(Sampler0, input.tex).rgb;

	//Exposure
	color *= exposure;

	//Filmic colours (ACES)
	if (tonemapping == TONEMAP_ACES) {
		float3 aces = rgb_to_aces(color.rgb);
		float3 oces = rrt(aces);
		color = ODT_RGBmonitor_100nits_dim(oces);
	}

	//Brightness
	color += brightness.xxx;

	//Convert to HSV
	float3 hsv = rgb_to_hsv(color.rgb);

	//Hue
	HUE(hsv) += hue;//assuming hue is in 0..1
	if (HUE(hsv) >= 1.0f) HUE(hsv) -= 1.0f;
	//Saturation
	SAT(hsv) *= saturation;
	//Value
	VAL(hsv) *= value;

	//Convert back to RGB
	color = hsv_to_rgb(hsv).rgb;

	//Contrast (from https://www.dfstudios.co.uk/articles/programming/image-programming-algorithms/image-processing-algorithms-part-5-contrast-adjustment/)
	float factor = (CONTRAST_FACTOR * (contrast + 1.0f)) / (1.0f * (CONTRAST_FACTOR - contrast));
	color.rgb = saturate(float3(factor*(color.r - 0.5f) + 0.5f, factor*(color.g - 0.5f) + 0.5f, factor*(color.b - 0.5f) + 0.5f));

	return float4(color.rgb, 1.0f);
}