#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 out_color;

uniform vec2 screenResolution;
uniform vec2 elementResolution;
uniform vec2 elementPosition;

uniform sampler2D textureSampler;

void main() {
    vec2 fragCoord = (gl_FragCoord.xy);
    fragCoord.y = screenResolution.y - fragCoord.y;
    vec2 uv = (fragCoord - elementPosition) / elementResolution;

    vec2 texSize = textureSize(textureSampler, 0);
    uv.y = uv.y * texSize.x / texSize.y;
    uv.x -= 0.5;
    uv.y *= 1.1;
    uv.x *= 1.1;
    uv.x += 0.5;
    uv.y -= 0.1;

    if (uv.y > 0.0) {
        out_color = texture(textureSampler, uv);
    } else {
        out_color = vec4(0);
    }

    // out_color = vec4(uv, 0, 1);
}