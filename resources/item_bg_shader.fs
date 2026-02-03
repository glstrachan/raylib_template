#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D myTexture;

void main() {
    float visible = texture(myTexture, fragTexCoord).a;
    finalColor = vec4(1.0, 0.0, 1.0, visible * (100.0 / 255.0));
}
