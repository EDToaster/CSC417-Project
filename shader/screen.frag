#version 450

in vec4 gl_TexCoord[];
in vec2 gl_FragCoord;
out vec3 color;
uniform sampler2D renderedTexture;

void main() {
    color = texture(renderedTexture, gl_TexCoord[0].xy).xyz;
}