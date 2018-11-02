#version 330 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN;
	vec4 FragPosLightSpace;
} fs_in;

struct PointLight{
	vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct DirLight{
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform int objectNum;
// 1 -> ship  2 -> water
uniform vec3 viewPos;
uniform sampler2D shadowMap;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;

uniform PointLight sunLight;

uniform DirLight rightLight;
uniform DirLight leftLight;
uniform DirLight backLight;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 norm);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

void main()
{    

	vec3 norm = texture(texture_normal1, fs_in.TexCoords).rgb;
	norm = normalize(norm * 2.0 - 1.0);
	norm = normalize(fs_in.TBN * norm);

	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec3 objectColor = texture(texture_diffuse1, fs_in.TexCoords).rgb;

	vec3 mainLight = CalcPointLight(sunLight, norm, fs_in.FragPos, viewDir);

	vec3 rightDirLighting = CalcDirLight(rightLight, norm, viewDir);
	vec3 leftDirLighting = CalcDirLight(leftLight, norm, viewDir);
	vec3 backDirLighting = CalcDirLight(backLight, norm, viewDir);

	vec3 baseLight = rightDirLighting + leftDirLighting + backDirLighting;

	float shadow = ShadowCalculation(fs_in.FragPosLightSpace, norm);       

	if (objectNum == 1){
		vec3 lighting =  objectColor * ((1 - shadow) * mainLight + baseLight);// * mainLight;// + baseLight);
		FragColor = vec4(lighting, 1.0f);
		float brightness = dot(lighting, vec3(0.2126, 0.7152, 0.0722));
		if(brightness > 1.0) BrightColor = vec4(lighting, 1.0);
		else BrightColor = vec4(0.0, 0.0, 0.0, 1.0f);
	}
	else if (objectNum == 2){
		vec3 lighting = objectColor * ((1 - shadow)) + shadow * 0.2 * objectColor;// * mainLight;//((0.9 - shadow) * mainLight) * 
		FragColor = vec4(lighting, 0.5f);
		float brightness = dot(lighting, vec3(0.2126, 0.7152, 0.0722));
		if(brightness > 1.0) BrightColor = vec4(lighting, 1.0);
		else BrightColor = vec4(0.0, 0.0, 0.0, 0.5f);
	}
	
}


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir){	
	vec3 lightDir = normalize(-light.direction);
	// diff
	float diff = max(dot(normal, lightDir), 0.0);
	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(viewDir, halfwayDir),0.0), 64.0);

	vec3 ambient = light.ambient;
	vec3 diffuse = light.diffuse * diff;
	vec3 specular = light.specular * spec;

	return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir){
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(viewDir, halfwayDir),0.0), 64.0);
    // attenuation
    //float distance = length(light.position - fragPos);
	// constant = 1.0 linear = 0.09  quadratic = 0.032
    //float attenuation = 1.5 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));    
    // combine results
    vec3 ambient = light.ambient;
	vec3 diffuse = light.diffuse * diff;
	vec3 specular = light.specular * spec;
	// * vec3(texture(texture_diffuse1, TexCoords));
	//* vec3(texture(texture_diffuse1, TexCoords));
	//* vec3(texture(material.specular, TexCoords));
    //ambient *= attenuation;
	//diffuse *= attenuation;
    //specular *= attenuation;
    return (ambient + diffuse + specular);
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 norm){

    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = norm;
    vec3 lightDir = normalize(sunLight.position - fs_in.FragPos);
    float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.001);

	//float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // check whether current frag pos is in shadow
	float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
       {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
		}    
    }
    shadow /= 9.0;

    if(projCoords.z > 1.0)shadow = 0.0;
        
    return shadow;
}