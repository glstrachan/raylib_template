#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D myTexture;

void main() {
    vec2 size = 1.0 / textureSize(myTexture, 0);
    int thickness = 2;

    vec4 aC = texture(myTexture, fragTexCoord);
    if (aC.a > 0.1) {
        finalColor = mix(fragColor, aC, aC.a);
    } else {
        float alpha = 0.0;
        for (int x = -thickness; x <= thickness; x++)
        for (int y = -thickness; y <= thickness; y++)
        {
            alpha += texture(myTexture, fragTexCoord + vec2(x, y) * size).a;
        }

        if (alpha > 0.01) {
            float outlineStrength = clamp(alpha, 0.0, 1.0);
            finalColor = vec4(fragColor.rgb, outlineStrength);
        }
    }
}
