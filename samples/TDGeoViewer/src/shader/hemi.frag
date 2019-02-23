precision highp float;

varying vec3 pixWPos;
varying vec3 pixWNrm;
varying vec3 pixClr;

uniform vec3 prmHemiSky;
uniform vec3 prmHemiGround;
uniform vec3 prmHemiUp;
uniform vec3 prmInvGamma;

void main() {
	vec3 wpos = pixWPos;
	vec3 wnrm = normalize(pixWNrm);
	vec3 vclr = pixClr;
	vec3 clr = vclr;
	vec3 hemi = prmHemiGround == prmHemiSky ? prmHemiSky : mix(prmHemiGround, prmHemiSky, (dot(wnrm, prmHemiUp) + 1.0) * 0.5);
	clr *= hemi;
	clr = pow(clr, prmInvGamma);
	gl_FragColor = vec4(clr, 1);
}
