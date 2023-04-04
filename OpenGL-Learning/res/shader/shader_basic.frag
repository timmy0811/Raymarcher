#shader fragment
#version 330 core

layout(location = 0) out vec4 o_Color;

uniform vec3 u_CameraOrigin;
uniform vec2 u_Resolution;
uniform float u_Time;

uniform vec3 u_SpotLightPos;
uniform float u_SoftShadow;
uniform float u_MinAlbedo;

uniform float u_BodyInterpolation;
uniform float u_BodyInterpolationEnabled;

uniform int u_MarchSteps;
uniform int u_ReflectionIteration;
uniform float u_Reflection;
uniform float u_Roughness;

uniform mat4 u_InverseView;
uniform mat4 u_InverseProjection;

uniform float u_SurfaceDistance;
uniform float u_RenderDistance;
uniform float u_SpecularPower;
uniform float u_ShadowStrength;

uniform vec3 u_SkyLowerGradient;
uniform vec3 u_SkyUpperGradient;

uniform float u_RepeatSpaceX;
uniform float u_RepeatSpaceZ;

uniform float u_NormalCheckOffset;

// Objects
#define MAX_OBJ 4
// uniform int u_ObjectCount;
uniform int u_ObjectIDs[MAX_OBJ];
uniform vec3 u_ObjectTransforms[MAX_OBJ];
uniform vec3 u_ObjectColor[MAX_OBJ];
uniform float u_ObjectPattern[MAX_OBJ];

// Math --------------------------------------------------------------------------------------

float smin(float a, float b, float k) {
    float h = clamp(0.5 + 0.5*(b-a)/k, 0.0, 1.0);
    return mix(b, a, h) - k*h*(1.0-h);
}

float sminMulti(float objs[MAX_OBJ], int numObjs, float k) {
    float res = objs[0];
    for (int i = 1; i < numObjs; i++) {
        res = smin(res, objs[i], k);
    }
    return res;
}

vec4 sminMultiColor(float objs[MAX_OBJ], vec3 colors[MAX_OBJ], int numObjs, float k) {
    float res = objs[0];
    vec3 col = colors[0];
    for (int i = 1; i < numObjs; i++) {
        float d = objs[i];
        vec3 c = colors[i];
        res = smin(res, d, k);
        col = mix(col, c, smoothstep(res - k, res + k, d));
    }
    return vec4(res, col);
}

float sdBox(vec3 pt, vec3 b) {
    vec3 box = abs(pt) - b;
    return length(max(box, 0.0)) + min(max(box.x, max(box.y, box.z)), 0.0);
}

mat4 rotationX(float angle) {
	return mat4(	1.0,		0,			0,			0,
			 		0, 	cos(angle),	-sin(angle),		0,
					0, 	sin(angle),	 cos(angle),		0,
					0, 			0,			  0, 		1);
}

mat4 rotationY(float angle) {
	return mat4(	1.0,		0,			0,			0,
			 		0, 	cos(angle),	-sin(angle),		0,
					0, 	sin(angle),	 cos(angle),		0,
					0, 			0,			  0, 		1);
}

float dot2( in vec2 v ) { return dot(v,v); }
float dot2( in vec3 v ) { return dot(v,v); }
float ndot( in vec2 a, in vec2 b ) { return a.x*b.x - a.y*b.y; }

vec3 floatToColor(float x){
    return (0.2 + 0.2 * sin(x * 2.0 + vec3(0.0,1.0,2.0))) * 2.0;
}

float rand(vec2 seed)
{
    float noise = fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
    return noise;
}

vec3 mapIntToRGB(int id)
{
    int r = (id & 0xFF0000) >> 16;
    int g = (id & 0x00FF00) >> 8;
    int b = (id & 0x0000FF);

    return vec3(r / 255.0, g / 255.0, b / 255.0);
}

int mapRGBToInt(vec3 color)
{
    int r = int(color.r * 255) << 16;
    int g = int(color.g * 255) << 8;
    int b = int(color.b * 255);

    return r | g | b;
}

// Distance Estimators -----------------------------------------------------------------------

float DE_RoundCone(vec3 p, vec3 a, vec3 b, float r1, float r2)
{
    vec3  ba = b - a;
    float l2 = dot(ba,ba);
    float rr = r1 - r2;
    float a2 = l2 - rr*rr;
    float il2 = 1.0/l2;
    
    vec3 pa = p - a;
    float y = dot(pa,ba);
    float z = y - l2;
    float x2 = dot2( pa*l2 - ba*y );
    float y2 = y*y*l2;
    float z2 = z*z*l2;

    float k = sign(rr)*rr*rr*x2;
    if( sign(z)*a2*z2 > k ) return  sqrt(x2 + z2)        *il2 - r2;
    if( sign(y)*a2*y2 < k ) return  sqrt(x2 + y2)        *il2 - r1;
                            return (sqrt(x2*a2*il2)+y*rr)*il2 - r1;
}

float DE_fractSierpinskiPyramid(vec3 marchPoint){
    vec3 ori = vec3(0.0,2.5,0.0);
    vec3 a1 = vec3(1,1,1)+ori;
	vec3 a2 = vec3(-1,-1,1)+ori;
	vec3 a3 = vec3(1,-1,-1)+ori;
	vec3 a4 = vec3(-1,1,-1)+ori;
    
    vec3 c;
    int n = 0;
    float dist, d;
    float scale = 2.;
    while(n < 16) {
        c = a1;
        dist = length(marchPoint - a1);
        d = length(marchPoint - a2);
        if(d < dist) { 
            c = a2;
            dist = d;
        }
        d = length(marchPoint - a3);
        if(d < dist) { 
            c = a3;
            dist = d;
        }
        d = length(marchPoint - a4);
        if(d < dist) { 
            c = a4;
            dist = d;
        }
        marchPoint = scale * marchPoint - c * (scale - 1.0);
        n++;
    }
    
    return length(marchPoint) * pow(scale, float(-n));
}

float DE_fractMandelbulb(vec3 pos) {
	vec3 z = pos;
	float dr = 1.0;
	float r = 0.0;
    float Power = 8.0;
    float Bailout = 4.0;
	for (int i = 0; i < 16 ; i++) {
		r = length(z);
		if (r>Bailout) break;
		
		// convert to polar coordinates
		float theta = acos(z.z/r);
		float phi = atan(z.y,z.x);
		dr =  pow( r, Power-1.0)*Power*dr + 1.0;
		
		// scale and rotate the point
		float zr = pow( r,Power);
		theta = theta*Power;
		phi = phi*Power;
		
		// convert back to cartesian coordinates
		z = zr*vec3(sin(theta)*cos(phi), sin(phi)*sin(theta), cos(theta));
		z+=pos;
	}
	return 0.5*log(r)*r/dr;
}

float DE_Sphere(vec3 position, float radius, vec3 marchPoint){
    return distance(marchPoint, position) - radius;
}

float DE_Plane(float heigth, vec3 marchPoint){
    return marchPoint.y - heigth;
}

// Raymarcher --------------------------------------------------------------------------------

vec2 minVecX(vec2 d1, vec2 d2)
{
	return (d1.x < d2.x) ? d1 : d2;
}

vec2 map(vec3 marchPoint){
    marchPoint.x = (fract(marchPoint.x / 4.0) * 4.0 - 2.0) * u_RepeatSpaceX + marchPoint.x * (1.0 - u_RepeatSpaceX);
    marchPoint.z = (fract(marchPoint.z / 4.0) * 4.0 - 2.0) * u_RepeatSpaceZ + marchPoint.z * (1.0 - u_RepeatSpaceZ);

    vec2 res = vec2(9999.0, 0.0);

    float distances[MAX_OBJ];
    vec3 colors[MAX_OBJ];

    distances[0] = 9999.0;
    int ind = 0;

    for(int i = 0; i < MAX_OBJ; i++){
        switch(u_ObjectIDs[i]){
            case 0: // Sphere
                distances[ind] = DE_Sphere(u_ObjectTransforms[i], 0.5, marchPoint);
                break;
            case 1: // Plane
                distances[ind] = DE_Plane(u_ObjectTransforms[i].y, marchPoint);
                break;
            case 2: // Mandelbulb
                distances[ind] = DE_fractMandelbulb(marchPoint);
                break;
            case -1:
                continue;
        }

        res = minVecX(res, vec2(distances[ind++], mapRGBToInt(u_ObjectColor[i]) * (1.0 - u_ObjectPattern[i]) + (-1 * u_ObjectPattern[i])));
        // colors[ind++] = u_ObjectColor[i];
    }

    if(u_BodyInterpolationEnabled > 0){
        return vec2(sminMulti(distances, ind, u_BodyInterpolation), mapRGBToInt(u_ObjectColor[0]));
    }
    else{
        return res;
    }

    
}

vec3 normal(vec3 p) {
    float d = map(p).x;
    vec2 e = vec2(u_NormalCheckOffset, 0);
    
    vec3 n = d - vec3(
        map(p-e.xyy).x,
        map(p-e.yxy).x,
        map(p-e.yyx).x);
    
    return normalize(n);
}

vec2 marchRay(vec3 origin, vec3 direction){
    float dist = 0.0;
    vec2 distToObj = vec2(0.0, 0.0);
    for(int i = 0; i < u_MarchSteps; i++){
        vec3 marchPoint = origin + direction * dist;
        distToObj = map(marchPoint);
        dist += distToObj.x;
        if(dist > u_RenderDistance || distToObj.x < u_SurfaceDistance ){
            break;
        }
    }
    return vec2(dist, distToObj.y);
}

float softShadow(vec3 origin, vec3 direction, float k){
    float res = 1.0;
    float dist = u_SurfaceDistance;
    for( int i=0; i<u_MarchSteps && dist < u_RenderDistance; i++ )
    {
        vec3 marchPoint = origin + direction * dist;
        float distToObj = map(marchPoint).x;

        if( distToObj < 0.001 ){
            return 0.0;
        }

        res = min(res, k * distToObj / dist);
        dist += distToObj;
    }
    return res;
}

vec3 ComputePixel(vec3 rayOrigin, vec3 direction, int iter){
    vec3 color = vec3(0.0);
    vec3 sourcePosition = rayOrigin;
    vec3 rayDirection = direction;
    
    for(int i = 0; i < iter; i++){
        vec3 iterationColor = vec3(0.0);

        vec2 dist = marchRay(sourcePosition, rayDirection);
        if(dist.x > u_RenderDistance){
            iterationColor += mix(u_SkyLowerGradient, u_SkyUpperGradient, (dot(vec3(0.0, 1.0, 0.0), rayDirection) + 1) / 2);
            color += iterationColor * pow(u_Reflection, i);
            break;
        }

        vec3 surfacePoint = sourcePosition + rayDirection * dist.x;

        vec3 lightRay = normalize(u_SpotLightPos - surfacePoint);
        vec3 norm = normal(surfacePoint);

        // Diffuse
        vec3 albedo;
        if(dist.y < 0.0){
            float tile = mod(floor(surfacePoint.x), 2.0) * (1.0 - mod(floor(surfacePoint.z), 2.0)) + mod(1.0 - floor(surfacePoint.x), 2.0) * (mod(floor(surfacePoint.z), 2.0));
            albedo = (vec3(tile) + 0.5) / 2.0;
        }
        else{
            albedo = mapIntToRGB(int(dist.y));
        }

        vec3 diffuse = albedo * vec3(clamp(dot(norm, lightRay), 0.0, 1.0 - u_MinAlbedo));
        vec3 reflectDir = normalize(rayDirection - 2.0 * dot(rayDirection, norm) * norm);

        // Specular
        float specular = pow(max(dot(reflectDir, lightRay), 0.0), u_SpecularPower);

        vec3 surfacePointElevated = surfacePoint + norm * u_SurfaceDistance * 2.0;
        float distToLight = marchRay(surfacePointElevated, lightRay).x;
        if(distToLight < length(u_SpotLightPos - surfacePoint)){
            diffuse *= u_ShadowStrength;
        } 
        else{
            float shadow = softShadow(surfacePointElevated, lightRay, u_SoftShadow);
            diffuse *= clamp(shadow, u_ShadowStrength, 1.0);
            diffuse += vec3(specular) * shadow;
        }

        iterationColor +=  diffuse  + u_MinAlbedo * albedo;
        
        // Tail recursive step
        sourcePosition = surfacePointElevated;
        rayDirection = reflectDir + vec3((rand(vec2(surfacePoint.x)) * 2.0 - 1.0) / u_Roughness, (rand(vec2(surfacePoint.y)) * 2.0 - 1.0) / u_Roughness, (rand(vec2(surfacePoint.z)) * 2.0 - 1.0) / u_Roughness);
        
        color += iterationColor * pow(u_Reflection, i);   
    }

    return color;
}

void main(){
    vec2 frag_Coord = vec2(gl_FragCoord.x / u_Resolution.x, gl_FragCoord.y / u_Resolution.y);
    frag_Coord =  frag_Coord * 2.0 - 1.0;

    vec4 target = u_InverseProjection * vec4(frag_Coord, 1.0, 1.0);
    vec3 rayDirection = normalize(vec3(u_InverseView * vec4(normalize(target.xyz / target.w), 0.0)));

    vec3 rayOrigin = u_CameraOrigin;

    vec3 color = vec3(ComputePixel(rayOrigin, rayDirection, u_ReflectionIteration));

    color = pow(color, vec3(0.4545));
    o_Color = vec4(color, 1.0);
}