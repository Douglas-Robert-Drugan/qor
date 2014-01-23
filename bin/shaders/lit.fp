#version 120

uniform sampler2D Texture;
uniform sampler2D TextureNrm;
uniform sampler2D TextureDisp;
uniform sampler2D TextureOcc;
uniform sampler2D TextureSpec;

/*uniform vec3 CameraPosition;*/

/*varying vec3 Position;*/
varying vec3 Tangent;
varying vec3 Bitangent;
varying vec2 Wrap;

varying vec3 ViewDir;
varying vec3 Normal;

varying vec3 WorldLight;
varying vec3 LightDir;

void main(void)
{
	vec3 eye = normalize(ViewDir);
	vec3 light = normalize(LightDir);
	float dist = length(WorldLight);
	vec3 wlight = normalize(WorldLight);
	
	float height = texture2D(TextureDisp, Wrap).r;
	height = height * 0.04 - 0.02;
	vec2 uvp = Wrap + (eye.xy * height);
	
	vec4 texel = texture2D(Texture, uvp);
	vec3 bump = normalize(texture2D(TextureNrm, uvp).rgb * 2.0 - 1.0);
	
	float ambient = 0.1;
	float diffuse = max(dot(light, bump), 0.0) * 0.5;
	float shine = 1.0 / 2.0;
	float spec = pow(clamp(dot(reflect(-eye, bump), light), 0.0, 1.0), shine);
	
	gl_FragColor =
	   texel * ambient * texture2D(TextureOcc, uvp)
	   + diffuse * texel
	   /*+ diffuse*texel*/
	   + vec4(vec3(spec * texture2D(TextureSpec, uvp).r), 1.0);
}

