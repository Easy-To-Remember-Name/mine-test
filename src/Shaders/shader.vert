#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in float aHeight;
layout(location = 2) in vec2 aTexCoord;

out float HeightValue;
out vec2 TexCoord;
out float FogDistance;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main()
{
    // aHeight stores terrain texture selector from CPU (already normalized).
    HeightValue = aHeight;
    TexCoord = aTexCoord;

    vec4 worldPos = model * vec4(aPos, 1.0);
    vec4 viewPos = view * worldPos;
    FogDistance = length(viewPos.xyz);
    gl_Position = projection * viewPos;
}
