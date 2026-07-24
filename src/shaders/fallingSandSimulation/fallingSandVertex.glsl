#version 400 core

layout (location = 0) in vec2 vertexQuadPosition;

// Per instance attributes
layout (location = 1) in vec2 vertexOffset; 
layout (location = 2) in vec3 vertexColor;   
layout (location = 3) in uint vertexType;   

uniform mat4 projection;
uniform float cellSize;

out vec3 fragmentColor;
out vec2 fragmentUV;
flat out uint fragmentType;

void main() 
{
    vec2 worldPos = vertexOffset + (vertexQuadPosition * cellSize);
    
    gl_Position = projection * vec4(worldPos, 0.0, 1.0);

    fragmentColor = vertexColor;
    fragmentUV = vertexQuadPosition;
    fragmentType = vertexType;
}