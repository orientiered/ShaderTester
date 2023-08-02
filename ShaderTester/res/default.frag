#ifdef GL_ES
precision highp float;
#endif
uniform vec2 iResolution;
uniform vec2 windowRes;
uniform float iTime;
uniform vec3 iMouse; 
uniform vec2 offset;
uniform float globalScale;

vec2 coordTransform(vec2 a, vec2 res) {
	return (a * 2. - res.xy) / res.y * 2.;
}

void main()
{
    vec2 uv = coordTransform(gl_FragCoord.xy+offset, iResolution) / globalScale;
    vec3 col = vec3(0.5, 0.5, 0.5)*abs(sin(iTime))*log(length(uv));
    // Output to screen
    gl_FragColor = vec4(col,1.0);
}