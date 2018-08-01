precision highp float;

attribute vec3 vtxPos;
attribute vec3 vtxNrm;
attribute vec3 vtxClr;

varying vec3 pixWPos;
varying vec3 pixWNrm;
varying vec3 pixClr;

uniform vec4 prmWMtx[3];

uniform mat4 prmViewProj;


vec3 calcWVec(vec3 v, vec3 sr0, vec3 sr1, vec3 sr2) {
	return vec3(dot(v, sr0), dot(v, sr1), dot(v, sr2));
}

vec3 calcWVec(vec3 v) {
	return calcWVec(v, prmWMtx[0].xyz, prmWMtx[1].xyz, prmWMtx[2].xyz);
}

vec3 calcWPos(vec3 v, vec4 w0, vec4 w1, vec4 w2) {
	vec4 p = vec4(v, 1);
	return vec3(dot(p, w0), dot(p, w1), dot(p, w2));
}

vec3 calcWPos(vec3 v) {
	return calcWPos(v, prmWMtx[0], prmWMtx[1], prmWMtx[2]);
}

void main() {
	vec3 wpos = calcWPos(vtxPos);
	vec3 wnrm = calcWVec(vtxNrm);
	pixWPos = wpos;
	pixWNrm = wnrm;
	pixClr = vtxClr;
	vec4 cpos = vec4(wpos, 1.0) * prmViewProj;
	gl_Position = cpos;
}
