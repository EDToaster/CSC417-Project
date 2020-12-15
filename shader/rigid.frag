#version 450

in vec4 gl_TexCoord[];
in vec4 gl_FragCoord;
out vec4 gl_FragColor;

float PHI = 1.61803398874989484820459;  // phi = Golden Ratio   
float PHIM1 = 0.61803398874989484820459;  // phi = Golden Ratio   

float gold_noise(in vec2 xy, in float seed){
	return fract(tan(distance(xy*PHI, xy)*seed)*xy.x);
}

float gold_noise(in float x, in float seed) {
	return fract(tan(x * PHIM1 *seed) * x);
}

vec4 render_noise(in uint ind, in vec4 col) {
	return clamp(gold_noise(ind, 1), 0, 1) * col;
}

void main() {
	gl_FragColor = vec4(0.7, 0.5, 0.4, 1) + render_noise(uint(10 * gl_TexCoord[0].x) + 10 * uint(10 * gl_TexCoord[0].y), vec4(0.2, 0.2, 0.2, 0.0));
	
}