#version 330 core
// FXAA
// source: http://blog.simonrodriguez.fr/articles/2016/07/implementing_fxaa.html
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec2 inverseScreenSize;
uniform float multiplier = 1.0;

float EDGE_THRESHOLD_MIN = 0.0312;
float EDGE_THRESHOLD_MAX = 0.125;
float SUBPIXEL_QUALITY = 0.875;
int ITERATIONS = 12;

const float quality[12] = float[12](1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0);

float rgb2luma(vec3 rgb) {
    return sqrt(dot(rgb, vec3(0.299, 0.587, 0.114)));
}

void main() {
    vec3 colorCenter = texture(screenTexture, TexCoords).rgb;

    float lumaCenter = rgb2luma(colorCenter);

    float lumaDown = rgb2luma(textureOffset(screenTexture,TexCoords,ivec2(0,-1)).rgb);
    float lumaUp = rgb2luma(textureOffset(screenTexture,TexCoords,ivec2(0,1)).rgb);
    float lumaLeft = rgb2luma(textureOffset(screenTexture,TexCoords,ivec2(-1,0)).rgb);
    float lumaRight = rgb2luma(textureOffset(screenTexture,TexCoords,ivec2(1,0)).rgb);

    float lumaMin = min(lumaCenter,min(min(lumaDown,lumaUp),min(lumaLeft,lumaRight)));
    float lumaMax = max(lumaCenter,max(max(lumaDown,lumaUp),max(lumaLeft,lumaRight)));

    float lumaRange = lumaMax - lumaMin;
    // Check if pixel is part of an edge
    if (lumaRange < max(EDGE_THRESHOLD_MIN, lumaMax * EDGE_THRESHOLD_MAX)) {
        FragColor = vec4(colorCenter, 1.0);
        return;
    }

    float lumaDownLeft = rgb2luma(textureOffset(screenTexture,TexCoords,ivec2(-1,-1)).rgb);
    float lumaUpRight = rgb2luma(textureOffset(screenTexture,TexCoords,ivec2(1,1)).rgb);
    float lumaUpLeft = rgb2luma(textureOffset(screenTexture,TexCoords,ivec2(-1,1)).rgb);
    float lumaDownRight = rgb2luma(textureOffset(screenTexture,TexCoords,ivec2(1,-1)).rgb);

    // Combine the four edges lumas (using intermediary variables for future computations with the same values).
    float lumaDownUp = lumaDown + lumaUp;
    float lumaLeftRight = lumaLeft + lumaRight;

    // Same for corners
    float lumaLeftCorners = lumaDownLeft + lumaUpLeft;
    float lumaDownCorners = lumaDownLeft + lumaDownRight;
    float lumaRightCorners = lumaDownRight + lumaUpRight;
    float lumaUpCorners = lumaUpRight + lumaUpLeft;

    // Compute an estimation of the gradient along the horizontal and vertical axis.
    float edgeHorizontal =  abs(-2.0 * lumaLeft + lumaLeftCorners)  + abs(-2.0 * lumaCenter + lumaDownUp ) * 2.0    + abs(-2.0 * lumaRight + lumaRightCorners);
    float edgeVertical =    abs(-2.0 * lumaUp + lumaUpCorners)      + abs(-2.0 * lumaCenter + lumaLeftRight) * 2.0  + abs(-2.0 * lumaDown + lumaDownCorners);

    // Is the local edge horizontal or vertical ?
    bool isHorizontal = (edgeHorizontal >= edgeVertical);

    float luma1 = isHorizontal ? lumaDown : lumaLeft;
    float luma2 = isHorizontal ? lumaUp : lumaRight;

    float gradient1 = luma1 - lumaCenter;
    float gradient2 = luma2 - lumaCenter;

    bool is1Steepest = abs(gradient1) >= abs(gradient2);

    float gradientScaled = 0.25 * max(abs(gradient1), abs(gradient2));

    float stepLength = isHorizontal ? inverseScreenSize.y : inverseScreenSize.x;

    float lumaLocalAverage = 0.0;

    if (is1Steepest) {
        stepLength = -stepLength;
        lumaLocalAverage = 0.5 * (luma1 + lumaCenter);
    } else {
        lumaLocalAverage = 0.5 * (luma2 + lumaCenter);
    }

    vec2 currentUV = TexCoords;
    if (isHorizontal) currentUV.y += stepLength * 0.5;
    else currentUV.x += stepLength * 0.5;
    // First exploration to find both ends of the same edge
    vec2 offset = isHorizontal ? vec2(inverseScreenSize.x, 0.0) : vec2(0.0, inverseScreenSize.y);

    vec2 uv1 = currentUV - offset;
    vec2 uv2 = currentUV + offset;

    float lumaEnd1 = rgb2luma(texture(screenTexture, uv1).rgb);
    float lumaEnd2 = rgb2luma(texture(screenTexture, uv2).rgb);
    lumaEnd1 -= lumaLocalAverage;
    lumaEnd2 -= lumaLocalAverage;

    bool reached1 = abs(lumaEnd1) >= gradientScaled;
    bool reached2 = abs(lumaEnd2) >= gradientScaled;
    bool reachedBoth = reached1 && reached2;

    if (!reached1) uv1 -= offset;
    if (!reached2) uv2 += offset;

    //Iterate on both sides until you reach an edge on both
    if (!reachedBoth) {
        for (int i = 2; i < ITERATIONS; i++) {
            if (!reached1) {
                lumaEnd1 = rgb2luma(texture(screenTexture, uv1).rgb);
                lumaEnd1 = lumaEnd1 - lumaLocalAverage;
            }

            if (!reached2) {
                lumaEnd2 = rgb2luma(texture(screenTexture, uv2).rgb);
                lumaEnd2 = lumaEnd2 - lumaLocalAverage;
            }

            reached1 = abs(lumaEnd1) >= gradientScaled;
            reached2 = abs(lumaEnd2) >= gradientScaled;
            reachedBoth = reached1 && reached2;

            if (!reached1) uv1 -= offset * quality[i];
            if (!reached2) uv2 += offset * quality[i];

            if (reachedBoth) break;
        }
    }
    // Find the closest extremity to the point
    float distance1 = isHorizontal ? (TexCoords.x - uv1.x) : (TexCoords.y - uv1.y);
    float distance2 = isHorizontal ? (uv2.x - TexCoords.x) : (uv2.y - TexCoords.y);

    bool isDirection1 = distance1 < distance2;
    float distanceFinal = min(distance1, distance2);

    float edgeThickness = (distance1 + distance2);

    float pixelOffset = -distanceFinal / edgeThickness + 0.5;

    bool isLumaCenterSmaller = lumaCenter < lumaLocalAverage;
    bool correntVariation = (isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0 != isLumaCenterSmaller;
    float finalOffset = correntVariation ? pixelOffset : 0.0;

    // Sub-pixel shifting
    // Full weighted average of luma over 3x3 neighborhood
    float lumaAverage = (1.0/12.0) * (2.0 * (lumaDownUp + lumaLeftRight) + lumaLeftCorners + lumaRightCorners);
    // Ratio of the delta between the global average and the center luma, over the luma range in the 3x3 neighborhood.
    float subPixelOffset1 = clamp(abs(lumaAverage - lumaCenter)/lumaRange,0.0,1.0);
    float subPixelOffset2 = (-2.0 * subPixelOffset1 + 3.0) * subPixelOffset1 * subPixelOffset1;
    // Compute a sub-pixel offset based on this delta.
    float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;

    // Pick the biggest of the two offsets.
    finalOffset = max(finalOffset,subPixelOffsetFinal);

    vec2 finalUv = TexCoords;
    if (isHorizontal) {
        finalUv.y += finalOffset * stepLength * multiplier;
    } else {
        finalUv.x += finalOffset * stepLength * multiplier;
    }

    vec3 finalColor = texture(screenTexture, finalUv).rgb;
    FragColor = vec4(finalColor, 1.0);
}
