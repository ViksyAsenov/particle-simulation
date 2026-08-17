#version 460 core

#define SAND 1
#define WATER 2
#define STONE 3
#define WOOD 4
#define FIRE 5

in vec3 fragmentColor;
in vec2 fragmentUV;
flat in uint fragmentType;

uniform float time;

out vec4 screenColor;

void main() 
{
    vec3 finalColor = fragmentColor;
    float alpha = 1.0;

    // Calculate distance to the edges of the cell (0.0 at center, 0.5 at edge)
    vec2 centerDistance = abs(fragmentUV - 0.5);
    float edgeDistance = max(centerDistance.x, centerDistance.y);

    // Subtle cell borders for solids
    if (fragmentType == SAND || fragmentType == STONE || fragmentType == WOOD) {
        if (edgeDistance > 0.40) {
            finalColor *= 0.85;
        }
    }

    // Water shimmer and transparency
    if (fragmentType == WATER) {
        finalColor += 0.05 * sin(time * 5.0 + fragmentUV.x * 10.0);
        alpha = 0.85;
    }

    // Fire glowing core
    if (fragmentType == FIRE) {
        float radialDist = distance(fragmentUV, vec2(0.5));

        if (radialDist < 0.25) {
            finalColor += vec3(0.5, 0.5, 0.0);
        }

        // Fade out fire edges
        alpha = smoothstep(0.5, 0.2, radialDist);
    }

    screenColor = vec4(finalColor, alpha);
}