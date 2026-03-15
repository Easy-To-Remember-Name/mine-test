#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in float aHeight;
layout(location = 2) in vec2 aTexCoord;

out float vHeight;      // normalized height
out vec2 TexCoord;
out float fogDistance;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float maxHeight;

void main()
{
    // Normalize to [0,1]
    vHeight = aHeight / maxHeight;  
    TexCoord = aTexCoord;
    
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position = projection * view * worldPos;
    
    fogDistance = length((view * worldPos).xyz);
}
