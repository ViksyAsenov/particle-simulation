#version 460 core

in vec2 uv;
out vec4 screenColor;

uniform sampler2DArray trailMap;
uniform int speciesCount;

struct SpeciesSettings {
    float moveSpeed;
    float turnSpeed;
    float sensorAngle;
    float sensorDistance;
    int sensorSize;
    float depositAmount;
    vec2 padding;
    vec4 color;
};

layout(std430, binding = 1) buffer SpeciesBuffer {
    SpeciesSettings species[];
};

void main() {
    vec4 finalColor = vec4(0.0);
    
    for(int i = 0; i < speciesCount; i++) {
        float intensity = texture(trailMap, vec3(uv, i)).r;
        
        finalColor += species[i].color * intensity;
    }
    
    screenColor = vec4(finalColor.rgb, 1.0);
}