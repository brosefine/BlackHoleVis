
#define M_PI 3.14159265

#ifdef DISK

layout (std140) uniform accDisk
{
    float minRad;
    float maxRad;
    float rot;
};

uniform sampler2D accretionTex;

bool diskIntersect (vec3 pos, vec3 vel, inout vec4 col) {
    if(abs(pos.y) > abs(vel.y) || sign(pos.y) == sign(vel.y)) return false;
    if(vel.y == 0.0) return false;

    vec3 diskHit = pos - pos.y * vel / vel.y;
    if(length(diskHit) > minRad && length(diskHit) < maxRad) {
    #ifdef CHECKEREDDISK
        float checker = max(0,(atan(diskHit.x, diskHit.z) + M_PI) / (2*M_PI)) + rot;
        if(checker > 1.0) checker -= 1.0;
        float weight = mod(int(36*checker),2);
        col = weight * vec4(1) + (1-weight) * vec4(1, checker, 0, 1);
    #else
        float heat = 1.0 - (length(diskHit) - minRad)/(maxRad-minRad);
        col = vec4(2 * vec3(1, heat, pow(heat,2)), 1);
        #ifdef DISKTEX
        float angle = max(0,(atan(diskHit.x, diskHit.z) + M_PI) / (2*M_PI)) + rot;
        if(angle > 1.0) angle -= 1.0;
        col.a = texture(accretionTex, vec2(angle, 1.0 - heat)).r;  
        #endif
    #endif // CHECKERBOARD
        return true;
    }

    return false;
}
#endif //CHECKEREDDISK

#ifdef CHECKEREDHOR
bool horizonIntersect (vec3 pos, vec3 vel, float r, inout vec4 col) {
    float x0, x1;
    float a = dot(vel, vel);
    float b = 2.0 * dot(pos, vel);
    float c = dot(pos, pos) - r*r;
    float discr = b * b - 4.0 * a * c; 
    if (discr < 0) {
        col = vec4(1,1,1,1);
        return false; 
    } else if (discr == 0) {
        x0 = x1 = - 0.5 * b / a; 
    } else { 
        float q = (b > 0) ? 
            -0.5 * (b + sqrt(discr)) : 
            -0.5 * (b - sqrt(discr)); 
        x0 = q / a; 
        x1 = c / q; 
    } 

    vec3 horHit = pos + max(x0, x1) * vel;

    float phi = max(0,(atan(horHit.x, horHit.z) + M_PI) / (2*M_PI));
    float theta = max(0,(atan(length(horHit.xz), (horHit.y)) + M_PI) / (2*M_PI));
    float phiWeight = mod(int(18*phi),2);
    float thetaWeight = mod(int(18*theta),2);
    col = vec4(phi, vec2(1) * mod(thetaWeight + phiWeight,2), 1);
    return true;
}

#endif