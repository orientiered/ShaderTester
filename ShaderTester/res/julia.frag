#ifdef GL_ES
precision highp float;
#endif
uniform vec2 iResolution;
uniform vec2 windowRes;
uniform float iTime;
uniform vec3 iMouse; 
uniform vec2 offset;
uniform float globalScale;
vec3 palette( float t ) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263,0.416,0.557);

    return a + b*cos( 6.28318*(c*t+d) );
}

vec2 mult(vec2 a, vec2 b) {
    return vec2(a.x*b.x-a.y*b.y, a.x*b.y+a.y*b.x);
}
vec2 complexexp(vec2 a) {
    return vec2(cos(a.y), sin(a.y))*exp(a.x); 
}
vec2 complexpow(vec2 a, float t) {
    float phi = atan(a.y, a.x);
    return vec2(cos(phi*t), sin(phi*t))*pow(length(a), t);
}
vec2 coordTransform(vec2 a, vec2 res) {
	return (a * 2. - res.xy) / res.y * 2.;
}

vec3 fractal(vec2 z, float power) {
    float i = 0.;
    float maxi = 100.*max(1., pow(log(globalScale), 1.32));
    vec2 c = (iMouse.z > 0.) ? coordTransform(iMouse.xy, windowRes) 
                      : vec2(sin(iTime*.5), cos(iTime*.5))*(.5+0.05*sin(iTime*2.));
    while (z.x*z.x+z.y*z.y < 4. && i < maxi) {
        i++;
        //z = mult(z, mult(z,z))+c;
        z = complexpow(z, power)+c;
        //z = complexpow(z, 3.)+c;
    }
    return i > maxi-0.1 ? vec3(0., 0., 0.) : palette((i+1.-log(log(length(z)))/log(2.))/maxi);
    
}

void main()
{
    vec2 uv = coordTransform(gl_FragCoord.xy+offset, iResolution) / globalScale;
    vec3 col = fractal(uv, 2.0);
    // Output to screen
    gl_FragColor = vec4(col,1.0);
}