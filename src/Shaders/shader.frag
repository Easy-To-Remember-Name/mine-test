#version 330 core

in float HeightValue;
in vec2 TexCoord;
in float FogDistance;

out vec4 FragColor;

uniform sampler2D grassTexture;
uniform sampler2D rockTexture;
uniform sampler2D soilTexture;
uniform sampler2D snowTexture;

uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform float gamma;
uniform int isOutline;

void main() {
    if(isOutline == 1) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec4 finalColor;

    if (HeightValue < 0.25) {
        finalColor = texture(soilTexture, TexCoord);
    }
    else if (HeightValue < 0.5) {
        finalColor = texture(grassTexture, TexCoord);
    }
    else if (HeightValue < 0.75) {
        finalColor = texture(rockTexture, TexCoord);
    }
    else {
        finalColor = texture(snowTexture, TexCoord);
    }

    finalColor.rgb = pow(finalColor.rgb, vec3(1.0/gamma));

    float fogFactor = clamp((FogDistance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    FragColor = mix(finalColor, vec4(fogColor, 1.0), fogFactor);
}
