//Heavily copied from LearnOpenGL.com

#version 330 core
in vec3 Normal;
in vec3 FragPos; 
in vec4 FragPosLightSpace;

out vec4 FragColor;
  
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 cameraPos;

uniform float Lighting_Ambient;
uniform float Lighting_Specular;

uniform sampler2D shadowMap;


float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    	float bias = 0.003;
	float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
	
	
 // PCF
    shadow = 0.0;
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
    
    if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
} 


void main()
{
   vec3 normal = normalize(Normal);
	
   // ambient
      vec3 ambient = Lighting_Ambient * lightColor;
    		
   // diffuse
      vec3 lightDir = normalize(lightPos - FragPos);
       float diff = max(dot(lightDir, normal), 0.0);
       vec3 diffuse = diff * lightColor;
   	    
   // specular
      vec3 viewDir = normalize(cameraPos - FragPos);
      vec3 halfwayDir = normalize(lightDir + viewDir);  
      float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
      vec3 specular = spec * lightColor;    
    
   // calculate shadow
      float shadow = ShadowCalculation(FragPosLightSpace);       
      vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * objectColor;    
        
    FragColor = vec4(lighting, 1.0);
}
