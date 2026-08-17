#version 460 core

in vec4 fragmentColor;

out vec4 screenColor;

void main() {
    vec2 coordinates = gl_PointCoord - vec2(0.5);
    float distance = length(coordinates);
    
    if (distance > 0.5) {
        discard;
    }
    
    // gradually fade out the color towards the edges of the point sprite
    float intensity = 1.0 - (distance * 2.0);
    
    screenColor = vec4(fragmentColor.rgb * intensity, intensity);
}