#version 330 core

in vec3 FragPos;
in float HeightValue;
in vec2 TexCoord;

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
    
    // Debug: Show height value as color
    // FragColor = vec4(HeightValue, HeightValue, HeightValue, 1.0);
    // return;
    
    // Simple texture selection based on HeightValue
    vec4 finalColor;
    
    if (HeightValue < 0.25) {
        // Underground - soil texture
        finalColor = texture(soilTexture, TexCoord);
    }
    else if (HeightValue < 0.5) {
        // Low areas - grass texture
        finalColor = texture(grassTexture, TexCoord);
    }
    else if (HeightValue < 0.75) {
        // Mid areas - rock texture
        finalColor = texture(rockTexture, TexCoord);
    }
    else {
        // High areas - snow texture
        finalColor = texture(snowTexture, TexCoord);
    }
    
    // If texture is black, show height as red debug color
    if (finalColor.r < 0.1 && finalColor.g < 0.1 && finalColor.b < 0.1) {
        finalColor = vec4(1.0, HeightValue, 0.0, 1.0); // Red-yellow debug
    }
    
    // Apply gamma correction
    finalColor.rgb = pow(finalColor.rgb, vec3(1.0/gamma));
    
    // Apply fog
    float distance = length(FragPos);
    float fogFactor = clamp((distance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    FragColor = mix(finalColor, vec4(fogColor, 1.0), fogFactor);
}