//terrain fragment shader

#define NUM_LIGHTS 8 //maximum number of lines in the scene - doesn't mean they're all necessarily active though

//light types
#define INACTIVE_LIGHT 0
#define POINT_LIGHT 1
#define DIRECTIONAL_LIGHT 2
#define SPOTLIGHT 3

//modes
#define COLOUR 0
#define DIFFUSE_TEXTURE 1
#define DIFFUSE_AND_NORMAL_MAP 2
#define NORMAL_MAP 3

//terrain colour multipliers
#define ROCK float3(0.6f, 0.6f, 0.9f)

//#define SHOW_DEBUG_RUINS_MAP //uncomment to see the debug ruins map - note that TerrainMesh.h needs a define as well for this to work.

//caustics effect
#define CAUSTICSANIMSPEED 0.8f
#define CAUSTICS_UVMARGIN 0.01f

#pragma region Parameters

Texture2D texSand : register(t0);//albedo sand texture
Texture2D texNormalMap : register(t1);//normal map
Texture2D shadowMap[NUM_LIGHTS] : register(t2);//one shadowmap per light
SamplerState sampler0 : register(s0);
SamplerState pointSampler : register(s1);//filter set to point

Texture2D texEffect : register(t13);//caustics texture
Texture2D texRocks : register(t14);//albedo rock texture
Texture2D texNormalmapRocks : register(t15);//normal map rock texture

Texture2D texRuins : register(t16);

cbuffer LightBuffer : register(b0) {
	float4 ambient;//unused alpha
	float4 diffuse[NUM_LIGHTS];//unused alpha
	float4 lightPosition[NUM_LIGHTS];//w is type (INACTIVE_LIGHT, POINT_LIGHT, DIRECTIONAL_LIGHT or SPOTLIGHT)
	float4 lightDirection[NUM_LIGHTS];//unused w
	float4 attenuation[NUM_LIGHTS];//constant - linear - quadratic - shadowmap mode (0 for no shadows, 1 for shadows)
	float oneOverFarPlane;
	float shadowmapBias;
	float showShadowmapErrors;//if 1, shows red where shadowmaps dont extend far enough
	float oneOverShadowmapSize;
};

cbuffer MaterialBuffer : register(b1) {
	float mode;//see modes defined at top
	float3 colour;
	float3 specularColour;
	float specularPower;
	float3 emissive;
	float pad;
}

cbuffer EffectBuffer : register(b5) {
	float time;//how much time has passed since start of application
	float3 unused;
}


struct FS_IN {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 binormal : BINORMAL;
	float3 tangent : TANGENT;
	float3 worldPosition : TEXCOORD1;
	float3 viewVector : TEXCOORD2;
	float4 lightViewPos[NUM_LIGHTS] : TEXCOORD3;
};

#pragma endregion Parameters

#pragma region Lighting

//Blinn-Phong specular lighting
float3 blinnPhong(float3 lightDirection, float3 normal, float3 binormal, float3 tangent, float3 tangentSpaceNormal, float3 viewVector, float3 specularColour, float specularPower) {
	float3 halfway = /*normalize*/(lightDirection + viewVector);//normalizing is done in tangent space anyways

	//transform light direction and half vector to put into tangent space
	float3 v;
	v.x = dot(lightDirection, tangent);
	v.y = dot(lightDirection, binormal);
	v.z = dot(lightDirection, normal);
	lightDirection = normalize(v);//now in tangent space
	v.x = dot(halfway, tangent);
	v.y = dot(halfway, binormal);
	v.z = dot(halfway, normal);
	halfway = normalize(v);//now in tangent space

	float intensity = pow(max(dot(tangentSpaceNormal, halfway), 0.0f), specularPower);
	return saturate(specularColour * intensity);
}

//Diffuse lighting
float4 calculateLighting(float3 lightDirection, float3 normal, float3 binormal, float3 tangent, float3 tangentSpaceNormal, float3 ldiffuse) {

	//transform light direction to put into tangent space
	float3 v;
	v.x = dot(lightDirection, tangent);
	v.y = dot(lightDirection, binormal);
	v.z = dot(lightDirection, normal);
	lightDirection = normalize(v);//now in tangent space

	float intensity = saturate(dot(tangentSpaceNormal, lightDirection));
	float3 colour = saturate(ldiffuse * intensity);
	return float4(colour.rgb, 1);
}

#pragma endregion Lighting


float4 main(FS_IN input) : SV_TARGET{

	float ruins = texRuins.Sample(pointSampler, input.tex).r;//grab whether there's ruins on the fragment or not (1 = yes, 0 = no)

	//use slope to determine how to texture terrain
	float slope = 1 - input.normal.y;
	//bring in range -1..3 from 0..1
	slope = slope*4 - 1;
	float2 sandUvs = input.tex * 5;
	float2 rockUvs = input.tex * 10;
	float3 rock = texRocks.Sample(sampler0, rockUvs).rgb * ROCK;
	float3 sand = texSand.Sample(sampler0, sandUvs).rgb;
	float3 baseColour = lerp(sand, rock, saturate(slope));
	//increase brightness based on height
	float heightFactor = saturate((input.worldPosition.y + 10)*0.05f);
	baseColour *= heightFactor;

	//Add animated caustics (sprite sheet)
	int animFrame = fmod(time, CAUSTICSANIMSPEED) / CAUSTICSANIMSPEED * 16;//16 sprites
	int2 frameDecal = int2(animFrame % 4, animFrame * 0.25f);
	float2 causticsUvs = fmod(input.tex * 6 + float2(sin(time*0.004f) * 10, cos(time*0.009f) * 10), 1);//scale the uvs up to get smaller caustics
	causticsUvs = causticsUvs * (1.0f - CAUSTICS_UVMARGIN) + CAUSTICS_UVMARGIN*0.5f; //make a subset of the uvs to map from 0..1 -> small->1-small (because otherwise the sprite animation borders are mangled)
	causticsUvs = causticsUvs * 0.25f + frameDecal * 0.25f;//finally we end up with the right caustics for one frame of the animation
	
	float4 caustics = texEffect.Sample(sampler0, causticsUvs) * (heightFactor*0.8f+0.2f) * ((1-saturate(slope))*0.7f + 0.3f);//attenuate based on height and slope
	baseColour += caustics.rgb;



	//default tangent space normal
	float3 tangentSpaceNormal = float3(0.5f, 0.5f, 1);//the per-fragment normal in tangent space rather than per-vertex in world space

	//get tangent space normal from normal map (different normal maps blend together for sand and rock)
	if (mode == NORMAL_MAP || mode == DIFFUSE_AND_NORMAL_MAP) {
		float3 normalmapSand = texNormalMap.Sample(sampler0, sandUvs).rgb *0.8f + tangentSpaceNormal * 0.2f;
		float3 normalmapRock = texNormalmapRocks.Sample(sampler0, rockUvs).rgb *0.8f + tangentSpaceNormal * 0.2f;
		tangentSpaceNormal = lerp(normalmapSand, normalmapRock, saturate(slope));
	}

	tangentSpaceNormal *= 2.0f;
	tangentSpaceNormal -= 1.0f;
	tangentSpaceNormal = normalize(tangentSpaceNormal);

	float4 lightColour = ambient;
	float4 specular = float4(0, 0, 0, 0);
	for (int light = 0; light < NUM_LIGHTS; ++light) {
		if (lightPosition[light].w == INACTIVE_LIGHT) {//w component of light's position corresponds to light type
													   //inactive light, so nothing to add here
		}
		else {//compute lighting for this light:

			//get distance from light and light vector (point lights, spotlights and shadowmapping)
			float3 lightVector = lightPosition[light] - input.worldPosition;
			float dist = length(lightVector);//distance to point light for attenuation


						     //check relevant shadowmap to see if we should light this fragment from this light
			float shadow = 1;//shadow multiplier is set to 1 in case shadowmap read is impossible or out of range

			if (attenuation[light].w == 1) {//read shadowmap
											//Compute projected uvs
				float2 pTexCoord = input.lightViewPos[light].xy / input.lightViewPos[light].w;
				pTexCoord *= float2(0.5f, -0.5f);
				pTexCoord += float2(0.5f, 0.5f);

				//if uvs are in 0..1 range, keep going
				if (pTexCoord.x >= 0 && pTexCoord.x <= 1 && pTexCoord.y >= 0 && pTexCoord.y <= 1) {

					//Compare to distance to light
					float lightDepthValue = 1 - dist * oneOverFarPlane;
					//add bias
					lightDepthValue += shadowmapBias;

					//Very simple soft shadow computation (16 samples instead of 1 to minimize jagged edges)
					shadow = 0;
					for (float y = -1.5f; y <= 1.5f; y += 1.0f) {
						for (float x = -1.5f; x <= 1.5f; x += 1.0f) {
							//Sample shadowmap (16 samples per light)
							float depthValue = shadowMap[light].Sample(sampler0, pTexCoord + float2(x, y) * oneOverShadowmapSize).r;
							if (lightDepthValue > depthValue) {
								//add a bit of light (if all 16 samples result in passes on this condition, pixel will be totally lit)
								shadow += 0.0625f;// 1/16
							}
						}
					}

				}
				else if (showShadowmapErrors == 1) {//uvs outside 0..1
					return float2(1, 0).rggr;// <-- see red where shadowmap is out of range
				}
				else {//uvs outside 0..1 but no error showing, just show shadow.
					shadow = 1;
				}
			}
			

			if (/*shouldLight*/shadow > 0) {
				if (lightPosition[light].w == POINT_LIGHT) {
					//point light
					lightVector /= dist;//normalize
					float attFactor = 1.0f / (attenuation[light].x + attenuation[light].y * dist + attenuation[light].z * dist * dist);
					//diffuse lighting
					lightColour += shadow * calculateLighting(lightVector, input.normal, input.binormal, input.tangent, tangentSpaceNormal, diffuse[light].rgb * attFactor);
					//specular component
					specular += shadow * blinnPhong(lightVector, input.normal, input.binormal, input.tangent, tangentSpaceNormal, input.viewVector, specularColour, specularPower).rgbr * diffuse[light] * attFactor;
				}
				else if (lightPosition[light].w == DIRECTIONAL_LIGHT) {
					//directional light
					float3 dir = -normalize(lightDirection[light].xyz);
					//diffuse lighting
					lightColour += shadow * calculateLighting(dir, input.normal, input.binormal, input.tangent, tangentSpaceNormal, diffuse[light].rgb);
					//specular component
					specular += shadow * blinnPhong(dir, input.normal, input.binormal, input.tangent, tangentSpaceNormal, input.viewVector, specularColour, specularPower).rgbr * diffuse[light];
				}
				else if (lightPosition[light].w == SPOTLIGHT) {
					//spotlight
					lightVector /= dist;//normalize
					float attFactor = 1.0f / (attenuation[light].x + attenuation[light].y * dist + attenuation[light].z * dist * dist);
					//compute cosine of the angle between [towards the light] and [spot light direction], base intensity off that
					float3 dir = -normalize(lightDirection[light].xyz);
					float dirDotlv = saturate(dot(dir, lightVector));
					//diffuse lighting
					lightColour += shadow * calculateLighting(lightVector, input.normal, input.binormal, input.tangent, tangentSpaceNormal, diffuse[light].rgb * attFactor * dirDotlv);
					//specular
					specular += shadow * blinnPhong(lightVector, input.normal, input.binormal, input.tangent, tangentSpaceNormal, input.viewVector, specularColour, specularPower).rgbr * diffuse[light] * dirDotlv * attFactor;
				}
			}//shouldLight == false (in shadow)
		}
	}//foreach light

#ifdef SHOW_DEBUG_RUINS_MAP
	return (lightColour * float4(baseColour, 1) + specular) - ruins * float4(0.3f, -0.3f, 0.3f, 0);//make ruin map greenish
#else
	return (lightColour * float4(baseColour, 1) + specular);
#endif
}



