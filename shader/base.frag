#version 450

struct Particle {
	uint id;
	float lifetime;
};

layout(std430, binding = 1) buffer particlelayout
{
	Particle particles[];
};

uniform vec2 simResolution;

in vec4 gl_FragCoord;
out vec4 gl_FragColor;

float PHI = 1.61803398874989484820459;  // phi = Golden Ratio   
float PHIM1 = 0.61803398874989484820459;  // phi = Golden Ratio   

float TWO_PI = 6.28318530718;
float Directions = 16.0;
float Quality = 4.0;
float Size = 2.0;

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
	vec2 simCoord = floor(gl_FragCoord.xy);
	uint particleIndex = uint(simCoord.y) * uint(simResolution.x) + uint(simCoord.x);

	vec4 col;
	switch(particles[particleIndex].id) {
	case 1: // sand
		col = vec4(.7, .5, 0.26, 1.0) + render_noise(particleIndex, vec4(0.1, 0.2, 0.1, 0));
		break;
	case 2: // water
		col = vec4(0.2, 0.3, 0.8, 1.0) + render_noise(particleIndex, vec4(0.1, 0.1, 0.1, 0));
		break;
	case 3: // oil
		col = vec4(0.8, 0.6, 0.4, 1.0) + render_noise(particleIndex, vec4(0.1, 0.1, 0.1, 0));
		break;
	case 4: // wood
		col = vec4(0.5, 0.2, 0.1, 1.0) + render_noise(particleIndex, vec4(0.1, 0.1, 0.1, 0));
		break;
	case 5: // fire
		col = (1 - particles[particleIndex].lifetime) * (vec4(0.7, 0.1, 0.0, 1.0) + render_noise(particleIndex, vec4(0.2, 0.4, 0.1, 0))) + vec4(.1, .1, .1, 0);
		break;
	case 6: // smoke
		col = vec4(0.1, 0.1, 0.1, 1.0) + render_noise(particleIndex, vec4(0.3, 0.3, 0.3, 0));
		break;
	case 7: // gunpowder
		col = vec4(0.25, 0.25, 0.25, 1.0) + render_noise(particleIndex, vec4(0.1, 0.1, 0.1, 0));
		break;
	case 8: // acid
		col = vec4(0.25, .9, .5, 1.0) + render_noise(particleIndex, vec4(0.1, 0.1, 0.1, 0));
		break;
	case 9: // cotton
		col = vec4(.84, .84, .84, 1.0) + render_noise(particleIndex, vec4(0.1, 0.1, 0.1, 0));
		break;
	case 10: // fuse
		col = vec4(.30, .30, .30, 1.0) + render_noise(particleIndex, vec4(0.1, 0.1, 0.1, 0));
		break;
	default: // air 
		col = vec4(0, 0, 0, 0);
		break;
	}

	// apply blur
	uint blendFactor = 0;
	uint totalIters = 0;
	vec4 bloomCol = col;	
    for(float d=0.0; d<TWO_PI; d+=TWO_PI/Directions)
    {
		for(float i=1.0/Quality; i<=1.0; i+=1.0/Quality)
        {
			totalIters++;
			vec2 newSamplePoint = simCoord + vec2(cos(d), sin(d)) * Size * i;
			uint newSampleIndex = uint(newSamplePoint.y) * uint(simResolution.x) + uint(newSamplePoint.x);

			switch(particles[newSampleIndex].id) {
			case 5:	bloomCol += render_noise(newSampleIndex, (1 - particles[newSampleIndex].lifetime) * vec4(.1, 0, 0, 1)); blendFactor++; break;
			case 8: bloomCol += render_noise(newSampleIndex, vec4(.01, .7, 0, 1)); blendFactor++; break;
			default: break;
			}
        }
    }

	gl_FragColor = mix(col, bloomCol, 0.5);
	//float i;
	//float c = modf(float(particleIndex) / 255.0, i);
	//gl_FragColor = vec4(c, 0, 0, 1.0);
}