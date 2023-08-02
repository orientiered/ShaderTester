#ifdef GL_ES
precision highp float;
#endif
uniform vec2 iResolution;
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
vec2 cexp(vec2 a) {
    return vec2(cos(a.y), sin(a.y))*exp(a.x); 
}
vec2 cpow(vec2 a, float t) {
    float phi = atan(a.y, a.x);
    return vec2(cos(phi*t), sin(phi*t))*pow(length(a), t);
}
vec2 coordTransform(vec2 a) {
	return (a * 2. - iResolution.xy) / iResolution.y * 2.;
}

vec3 fractal(vec2 c, float power) {
    float i = 0.;
    float maxi = 100.*max(1., pow(log(globalScale), 1.2));
    vec2 z = vec2(0., 0.);
    while (z.x*z.x+z.y*z.y < 20. && i < maxi) {
        i++;
        z = cpow(z, power)+c;
    }
    return i > maxi-0.1 ? vec3(0., 0., 0.) : palette( (i+2. - log(log(z.x*z.x+z.y*z.y))/log(2.) ) /maxi);
    
}

void main()
{
    vec2 uv = coordTransform(gl_FragCoord.xy+offset) / globalScale;
    vec3 col = fractal(uv, 2.0);
    // Output to screen
    gl_FragColor = vec4(col,1.0);
}