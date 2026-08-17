#version 460 core

uniform float fadeAmount;

out vec4 screenColor;

void main() {
    screenColor = vec4(0.0, 0.0, 0.0, fadeAmount);
}