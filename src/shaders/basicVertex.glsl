#version 400 core

layout (location = 0) in vec2 vertexPosition;
layout (location = 1) in vec3 vertexColor;

out vec3 fragmentColor;

uniform mat4 projection;

void main() {
  gl_Position = projection * vec4(vertexPosition, 0.0, 1.0);
  
  fragmentColor = vertexColor;
}