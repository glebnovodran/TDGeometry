//#version 100
precision highp float;

varying vec3 pixWPos;
varying vec3 pixWNrm;
varying vec3 pixClr;

uniform vec3 prmHemiSky;
uniform vec3 prmHemiGround;
uniform vec3 prmHemiUp;
uniform vec3 prmInvGamma;

uniform vec3 prmViewPos;
uniform vec3 prmSpecDir;
uniform vec3 prmSpecClr;
uniform float prmSpecRough;

struct WK_CTX {
	vec3 P;
	vec3 N;
	vec3 V;
	vec3 L;
	vec3 H;
	vec3 vclr;
};

vec3 diff_hemi(WK_CTX ctx) {
	vec3 wnrm = normalize(pixWNrm);
	vec3 hemi = prmHemiGround == prmHemiSky ? prmHemiSky : mix(prmHemiGround, prmHemiSky, (dot(wnrm, prmHemiUp) + 1.0) * 0.5);
	return hemi;
}

vec3 diff_nolit(WK_CTX ctx) {
	return pixClr;
}

vec3 diff_none(WK_CTX ctx) {
	return vec3(0.0, 0.0, 0.0);
}

vec3 diff_test(WK_CTX ctx) {
	float d = dot(ctx.N, ctx.H);
	return vec3(d, d, d);
}

vec3 spec_blinn(WK_CTX ctx) {

	float nh = dot(ctx.N, ctx.H);
	nh = max(nh, 0.0);
	float rr = prmSpecRough * prmSpecRough;

	float pwr = max(0.001, 2.0/(rr*rr) - 2.0);
	float val = pow(nh, pwr);

	return val * prmSpecClr;
//	return nh * prmSpecClr;
}

vec3 spec_phong(WK_CTX ctx) {
	float vr = max(0.0, dot(ctx.V, reflect(-ctx.L, ctx.N)));
	float rr = prmSpecRough * prmSpecRough;
	float pwr = max(0.001, 2.0/rr - 2.0);
	float val = pow(vr, pwr) / 2.0;
	return val * prmSpecClr;
//	return vr * prmSpecClr;
}

vec3 spec_none(WK_CTX ctx) {
		return vec3(0, 0, 0);
}

void main() {
	WK_CTX wk;
	wk.P = pixWPos;
	wk.N = normalize(pixWNrm);
	wk.V = -normalize(wk.P - prmViewPos);
	wk.L = -prmSpecDir;
	wk.H = normalize(wk.L + wk.V);
	wk.vclr = pixClr;
	
	vec3 clr = wk.vclr;
	vec3 hemi = diff_hemi(wk);
	clr *= hemi;
//	clr = diff_none(wk);

//	vec3 specClr = spec_blinn(wk);
	vec3 specClr = spec_phong(wk);
	clr += specClr;

	clr = pow(clr, prmInvGamma);

	gl_FragColor = vec4(clr, 1);
}
