#version 330 core

in VS_OUT {
    vec3 FragPos;
//	vec3 Normal;
    vec2 TexCoords;
    mat3 TBN;
} fs_in;

out vec4 FragColor;

struct DirLight{
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform vec3 viewPos;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform DirLight rightMainLight;
uniform DirLight leftMainLight;
uniform DirLight backLight;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

void main()
{    
	vec3 norm = texture(texture_normal1, fs_in.TexCoords).rgb;
	norm = normalize(norm * 2.0 - 1.0);
	norm = normalize(fs_in.TBN * norm);

	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec3 objectColor = texture(texture_diffuse1, fs_in.TexCoords).rgb;
	// dirLighting includes the diff and specular
	vec3 rightDirLighting = CalcDirLight(rightMainLight, norm, viewDir);
	vec3 leftDirLighting = CalcDirLight(leftMainLight, norm, viewDir);
	vec3 backDirLighting = CalcDirLight(backLight, norm, viewDir);

	vec3 lighting = (backDirLighting + rightDirLighting + leftDirLighting) * objectColor;
	FragColor = vec4(lighting, 1.0f);
}


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir){	
	vec3 lightDir = normalize(-light.direction);

	// diff
	float diff = max(dot(normal, lightDir),0.0);

	// specular

	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(viewDir, halfwayDir),0.0), 64.0);

	vec3 ambient = light.ambient;
	vec3 diffuse = light.diffuse * diff;
	vec3 specular = light.specular * spec;

	return (ambient + diffuse + specular);

}