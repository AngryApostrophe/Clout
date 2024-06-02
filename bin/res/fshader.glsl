#version 330 core
in vec3 Normal;
in vec3 FragPos; 

out vec4 FragColor;
  
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 cameraPos;

uniform float Lighting_Ambient;
uniform float Lighting_Specular;

void main()
{
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos); 

	
	vec3 ambient = Lighting_Ambient * lightColor;


	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor;
	
	vec3 viewDir = normalize(cameraPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm); 
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = Lighting_Specular * spec * lightColor;  
	

	vec3 result = (ambient + diffuse + specular) * objectColor;
	FragColor = vec4(result, 1.0);
}
