#version 430 core

out vec4 FragColor;

struct Portal {
    vec4 data;
};

layout(std140, binding = 0) buffer PortalBuffer {
    Portal portals[];
};

uniform int uPortalCount;
uniform vec3 uCameraPos;
uniform vec3 uCameraRot;
uniform vec2 uResolution;

uniform float uTime;
uniform int uDimension;

#define res uResolution
#define I gl_FragCoord.xy
#define O FragColor

float dot2(vec2 v) { return dot(v, v); }

float sdSegment( in vec2 p, in vec2 a, in vec2 b )
{
    vec2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h );
}

vec3 rainbow(float t) {
    float s = sin(t);
    return vec3(s, cos(t), -s) * .5 + .5;
}

vec2 boxIntersection( in vec3 ro, in vec3 rd, in vec3 rad, out vec3 oN ) 
{
    vec3 m = 1.0/rd;
    vec3 n = m*ro;
    vec3 k = abs(m)*rad;
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;

    float tN = max( max( t1.x, t1.y ), t1.z );
    float tF = min( min( t2.x, t2.y ), t2.z );
	
    if( tN>tF || tF<0.0) return vec2(-1);
    
    oN = -sign(rd)*step(t1.yzx,t1.xyz)*step(t1.zxy,t1.xyz);

    return vec2( tN, tF );
}

vec2 iPortal(vec3 ro, vec3 rd) {
    float t = -ro.z / rd.z, tq = t;
    vec3 point = ro + rd * t;
    
    float h = -0.2;
    float r = 0.7;
    bool dh = length(point.xy - vec2(0,r + h)) <= r && t > 0.0 && point.y > 0.0;

    return vec2(dh, t);
}

int Gid;
bool Gmf;
vec4 iScene(vec3 ro, vec3 rd, int d) {
    float t = 1e20;
    bool hit = false;
    vec3 normal = vec3(1e20);
    Gid = -1;

    vec2 tp = vec2(0, 1e20);
    for(int i = 0; i < uPortalCount; i++) {
        vec4 data = portals[i].data;
        vec2 ip = iPortal(ro - data.xyz, rd);
        if(ip.x > 0.5 && ip.y < tp.y) tp = ip;
    }
    if(tp.x > 0.5 && tp.y < t) t = tp.y, hit = true, Gid = 0, normal = vec3(0,0,sign(ro.z + rd.z * (tp.y - 0.01)));

    float tf = -ro.y / rd.y;
    if(Gmf) tf = abs(tf);
    if(tf > 0.0 && tf < t) t = tf, hit = true, Gid = 1, normal = vec3(0,1,0);

    if(d == 0) {
        vec3 N;

        vec2 bi = boxIntersection(ro - vec3(0.4, 0.2, 0.4), rd, vec3(0.2), N);
        bool bh = bi.y > 0.0;
        float bt = bi.x < 0.0 ? bi.y : bi.x;

        if(bh && bt < t) t = bt, hit = true, Gid = 2, normal = N;

        bi = boxIntersection(ro - vec3(0.6, 0.2, -0.6), rd, vec3(0.2), N);
        bh = bi.y > 0.0;
        bt = bi.x < 0.0 ? bi.y : bi.x;

        if(bh && bt < t) t = bt, hit = true, Gid = 3, normal = N;
    }
    if(d == 1) {
        vec3 N;

        vec2 bi = boxIntersection(ro - vec3(-0.4, 0.2, -0.6), rd, vec3(0.2), N);
        bool bh = bi.y > 0.0;
        float bt = bi.x < 0.0 ? bi.y : bi.x;

        if(bh && bt < t) t = bt, hit = true, Gid = 4, normal = N;
    }

    if(!hit) t = -1.0;

    return vec4(t, normal);
}

int Gdimension;
vec4 trace(vec3 ro, vec3 rd, int dimension) {
    bool hit = false;
    float td = 0.0;
    vec3 hitNormal;
    int hitId = -1;

    for(int i = 0; i < 15; i++) {
        vec3 pos = ro + rd * td;
        vec4 ts = iScene(pos, rd, dimension);
        vec3 normal = ts.yzw;
        bool scenehit = ts.x > 0.0;
        float d = ts.x;
        if(scenehit) {
            if(Gid == 0) {
                td += 0.0001 + d;
                dimension = 1 - dimension;
                continue;
            }
            td += d;
            hitId = Gid; hitNormal = normal; hit = true; break;
        }
        else break;
    }

    Gid = hitId;

    Gdimension = dimension;
    return vec4(td, hitNormal);
}

vec3 render(vec2 Iq)
{
    vec2 p = (Iq - 0.5 * res) / res.y;

    float time = 0.5 * uTime;
    vec3 camPos = uCameraPos;
    vec3 camDir = vec3(
        cos(uCameraRot.x) * sin(uCameraRot.y)  ,
        sin(uCameraRot.x)                      ,
        cos(uCameraRot.x) * cos(uCameraRot.y) );

    camDir = normalize(camDir);

    float fov = radians(45.0);
    vec3 worldUp = vec3(0.0, 1.0, 0.0);
    vec3 camRight = normalize(cross(worldUp, camDir));
    vec3 camUp    = cross(camDir, camRight);

    float ta = tan(fov * 0.5);

    vec3 ro = camPos;
    vec3 rd = normalize(camDir + p.x * camRight * ta + p.y * camUp * ta);

    Gmf = true;
    vec4 ri = trace(ro, rd, uDimension);
    int di = Gdimension;
    vec3 normal = ri.yzw;
    float td = ri.x;
    vec3 hitPoint = ro + rd * td;
    int id = Gid;

    vec3 lightDir = normalize(vec3(1, .5, 1));
    
    Gmf = false;
    vec4 st = trace(hitPoint + 0.0001 * normal, lightDir, di);
    float light = Gid > 0 ? 0.5 : 1.0;
    
    vec3 color;
    bool dh = (di == 1 && uDimension == 0) || (di == 1 && uDimension == 1);
    if(id == 1) { color = vec3(0.9, 0.45, 0.0); if(dh) color = 1.0 - color; }
    else          color = vec3(1);
    
    vec3 col = color * light;
    
    float ds = 1e20;
    float a = 1e20;

    for(int i = 0; i < uPortalCount; i++) {
        vec3 hi = hitPoint - portals[i].data.xyz;
        float h = -0.2;
        float r = 0.7;

        vec2 d = vec2(0.725 * sqrt(r * r - h * h), 0.0);
        vec2 dq = d;
        
        d = abs(hi.xz) - d;
        float da = atan(d.y, d.x) * 0.3;
        
        float sd = sdSegment(hi.xz, dq, -dq);
        
        if(sd < ds) { ds = sd; }
        if(i > 0) a = mix(da, a, tanh(sd)); else a = da;
    }

    if(!dh) a = mix(3.141 * 0.6 - a, a, tanh(2.0 * sqrt(ds)));
    else    a = 3.141 * 0.6 - a;

    if(id == 1) col = (0.2 + 0.8 * tanh(rainbow(a * 2.0))) * light;
    else col = color * light;

    return col;
}

const int U = 3;
void main() 
{
    vec3 col = vec3(0.0, 0.0, 0.0);
    
    for(int i = 0; i < U; i++) {
        for(int j = 0; j < U; j++) {
            float k = 1.0 / float(U);
            
            vec2 q = I + k * (vec2(i, j) - 0.5 * float(U));
            col += render(q);
        }
    }
    
    col /= float(U * U);
    
    O = vec4(col, 1.0);
}