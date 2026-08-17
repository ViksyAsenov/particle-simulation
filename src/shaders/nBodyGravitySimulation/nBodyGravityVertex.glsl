#version 460 core

layout (location = 0) in vec2 vertexPosition;
layout (location = 1) in vec4 vertexColor;
layout (location = 2) in vec2 vertexMassRadius; // x = mass, y = radius

uniform mat4 projection;

out vec4 fragmentColor;

void main() {
    gl_Position = projection * vec4(vertexPosition, 0.0, 1.0);
    
    gl_PointSize = vertexMassRadius.y;
    fragmentColor = vertexColor;
}