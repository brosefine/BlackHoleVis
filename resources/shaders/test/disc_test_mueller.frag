/*
* Interactive Visualization of a thin disc around a Schwarzschild black hole
* by Thomas Müller
* https://www2.mpia-hd.mpg.de/homes/tmueller/projects/rela/thindisk/thindisk.html
* https://arxiv.org/abs/1206.4259v1
*/
  
#define PI     3.14159265
#define PI_2   1.5707963
#define invPI  0.318309886

#define DEG_TO_RAD   0.01745329
#define RAD_TO_DEG   57.2957795
#define MIN_TEMP     1000.0
#define MAX_TEMP    11000.0
#define FLUX_REL    9.167e-4
#define FLUX_MAX    0.1
  
#define edr  0.333333333  //  = 1/3
#define es   0.037037037  //  = 1/27
#define s3   1.732050808  //  sqrt(3)
#define s3h  0.866025404  //  sqrt(3/2)  
 
in vec2 texPos;

out vec4 FragColor;

uniform sampler2D  tempTex;

uniform float      ri;
uniform float      rsdri;
uniform float      iAngle;  // theta
uniform vec2       tfovXY;
uniform vec2       discRadius;
 
const float      rs = 1.0;
float a[15];
float b[15];
float c[15];
float phi[15];

// --------------------- clearArrays ------------------------
void clearArrays() {
   for(int i=0; i<15; i++) {
      a[i] = b[i] = c[i] = phi[1] = 0.0;
   }
}

// ------------------------- acosh ---------------------------
float acosh ( float x ) {
    return log(x+sqrt(x*x-1.0));
}

// ------------------------- sinh ----------------------------
float sinh ( float x ) {
    return (exp(x)-exp(-x))*0.5;
}

// ------------------------- cosh ----------------------------
float cosh ( float x ) {
    return (exp(x)+exp(-x))*0.5;
}

// ------------------------- tanh ----------------------------
float tanh ( float x ) {
    return sinh(x)/cosh(x);
}

// --------------------- transCartSph -------------------------
void transCartSph(in vec3 xOld, out vec3 xNew ) {
    float x = xOld.x;
    float y = xOld.y;
    float z = xOld.z;
    float r     = sqrt(x*x + y*y + z*z);
    float theta = atan(sqrt(x*x+y*y),z);
    float phi   = atan(y,x);
    xNew = vec3(r,theta,phi);
}

// --------------------- sncndn function -------------------------
void sncndn( float u, float m, float eps, 
             out float sn, out float cn, out float dn) {
    float mu,v,hn;
    if (m>1.0) { mu=1.0/m; v=u*sqrt(m); } else {mu=m; v=u;}
    a[0] = 1.0;
    b[0] = sqrt(1.0-mu);
    c[0] = sqrt(mu);
    
    for(int n=1; n<15; n++) {
        a[n] = 0.5*(a[n-1]+b[n-1]);
        b[n] = sqrt(a[n-1]*b[n-1]);
        c[n] = 0.5*(a[n-1]-b[n-1]);
    };    
    phi[14] = pow(2.0,float(14))*a[14]*v;
    for(int i=14;i>0;i--) {
        phi[i-1] = 0.5*(asin(c[i]/a[i]*sin(phi[i]))+phi[i]);
    }
    sn = sin(phi[0]);
    cn = cos(phi[0]);
    dn = cn/cos(phi[1]-phi[0]);
    if (m>1.0) {
        sn *= sqrt(mu);
        hn = dn; dn = cn; cn = hn;
    }
}

// --------------------- einfach function -------------------------
float einfach ( const float q, const float rho, float Phi1 ) {
    float xi   = rsdri;
    float psi  = acos(q/pow(rho,3.0));
    float psi3 = psi*edr;
    float sq3  = sqrt(3.0);
    float cpsi3 = cos(psi3);
    float spsi3 = sin(psi3);
    float x1,x2,x3;
    if (rho>0.0)  {
        x1 = rho*(cpsi3-sq3*spsi3)+edr;
        x2 = rho*(cpsi3+sq3*spsi3)+edr;
        x3 = -2.0*rho*cpsi3+edr;
    } else {
        x1 = rho*(cpsi3-sq3*spsi3)+edr;
        x3 = rho*(cpsi3+sq3*spsi3)+edr;
        x2 = -2.0*rho*cpsi3+edr;
    }
    float x2mx1 = x2-x1;
    float x1mx3 = x1-x3;
    float x2mx3 = x2-x3;
    float kqua = x1mx3/x2mx3;
    float ximx1 = xi-x1;
    float ximx2 = xi-x2;
    float ximx3 = xi-x3;
    float u = 0.5*sqrt(x2mx3)*Phi1;
    float sn,cn,dn;
    sncndn(u,kqua,1.0e-12,sn,cn,dn);
    float numerator   = sn*sqrt(x2mx1*x2mx1 * ximx3 / (ximx1*ximx1*x2mx3)) + sqrt(ximx2/ximx1)*cn*dn;
    float denominator = 1.0 - x1mx3/x2mx3 * ximx2/ximx1*sn*sn;
    float snuv = numerator/denominator;
    return (x2-x1*snuv*snuv)/(1.0-snuv*snuv);
}         

// --------------------- mulComplex function -------------------------
vec2 mulComplex ( vec2 a, vec2 b ) {
   return vec2( a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x );
}

// --------------------- divComplex function -------------------------
vec2 divComplex ( vec2 a, vec2 b ) {
   float w = 1.0/(b.x*b.x + b.y*b.y);
   return w*vec2( a.x*b.x + a.y*b.y, a.y*b.x - a.x*b.y );
}

// --------------------- sqrtComplex function -------------------------
vec2 sqrtComplex ( vec2 a ) {
   float r = sqrt(a.x*a.x + a.y*a.y);
   float f1 = 0.5*(r+a.x);
   float f2 = 0.5*(r-a.x);
   return vec2(sqrt(f1),sign(a.y)*sqrt(f2));
}

// --------------------- aufwendig function -------------------------
float aufwendig ( const float q, const float rho, float Phi1 ) {
   float xi   = rsdri;
   float psi  = acosh(sign(q)*abs(q)*27.0);
   float psi3 = psi*edr;
   float sq3  = sqrt(3.0);
   float esdr = 1.0/sq3;
   float cpsi3 = cosh(psi3);
   float spsi3 = sinh(psi3);

   vec2 x1 = vec2(edr*(cpsi3+1.0), esdr*spsi3);
   vec2 x2 = vec2(-2.0*edr*cpsi3+edr, 0);
   vec2 x3 = vec2(edr*(cpsi3+1.0), -esdr*spsi3);

   vec2 kq = divComplex(x1-x3,x2-x3);
   vec2 ks = sqrtComplex(vec2(1.0,0.0)-kq);
   vec2 k1 = divComplex(vec2(1.0,0.0)-ks,vec2(1.0,0.0)+ks);

   float mchi = tanh(psi3)*esdr;
   float k2   = mchi/(1.0+sqrt(1.0+mchi*mchi));
   float zeta = sqrt(1.0+k2*k2);

   vec2 x_i = vec2(xi,0.0);
   vec2 cnvm = sqrtComplex(divComplex(x2-x3,x_i-x1));
   vec2 dnvm = sqrtComplex(divComplex(x_i-x3,x_i-x1));

   vec2 f1 = cnvm + dnvm + vec2(1.0,0.0) + mulComplex(ks,cnvm);
   vec2 f2 = cnvm + dnvm - vec2(1.0,0.0) - mulComplex(ks,cnvm);
   vec2 cnz0c = mulComplex(k1,divComplex(f1,f2));

   float ezeta2 = 1.0/(zeta*zeta);

   float cnz0 = cnz0c.x;
   float snz0 = sqrt(1.0-cnz0*cnz0);
   float dnz0 = sqrt(1.0-snz0*snz0*ezeta2);

   float z = 0.5*Phi1*sqrt(cpsi3)*pow(1.0+mchi*mchi,0.25);

   float sn,cn,dn;
   sncndn(z,ezeta2,1.0e-12,sn,cn,dn);

   float SD = (sn*cnz0*dnz0 + snz0*cn*dn)/(dn*dnz0 - ezeta2*sn*snz0*cn*cnz0);
   float SD2 = pow(SD,2.0);
   float SD4 = pow(SD,4.0);
   float numerator   = (1.0-2.0*cpsi3)*(1.0-pow(k2*ezeta2,2.0)*SD4) - ((cpsi3+1.0)*(k2*k2-1.0)-2.0*sq3*k2*spsi3)*ezeta2*SD2;
   float denominator = 3.0*(1.0-pow(k2*ezeta2,2.0)*SD4 + (1.0-k2*k2)*ezeta2*SD2);

   return numerator/denominator;
}

// --------------------- main shader function -------------------------
void main()  {
    clearArrays();

    float sx = texPos[0];
    float sy = texPos[1];
    float dx,dy;        // direction of geodesic
    float omega;        // geodesic plane is rotated by omega out of the equatorial plane
    float normxd,normdr,aqua,q,psi,rho;
    float ti  = tan(iAngle);
    float ti2 = ti*ti;

    float ci  = cos(iAngle);
    float si  = sin(iAngle);
    float vs  = 4.0*es;

    float ksi;         // initial angle
    float angleQ;      // intersection angle
    float chi=0.0;     // intersection (distance to black hole, azimuth)
    int pos;

    dy = tfovXY.y*(sy);
    dx = tfovXY.x*(sx);

    // determine rotation angle 'omega'
    omega = atan(dy,dx);

    // initial direction of the geodesic within the geodesic plane
    normdr = sqrt(1.0 + dx*dx + dy*dy);
    ksi    = acos(1.0/normdr);

    // parameter for analytic solution
    aqua = rsdri*rsdri*(1.0-rsdri)/(sin(ksi)*sin(ksi));
    q    = aqua*0.5-es;
    rho  = sign(q)*edr;

    // determine intersection angle between geodesic and disk
    normxd = sqrt(dx*dx + dy*dy*(1.0 + ti2));
    angleQ = acos(-dy*ti/normxd);

    int side = 0;  // noside = 0, front = 1, back = 2

    // background color
    vec4 color = vec4(0,0,0,1);

    if (aqua<vs) {
        chi = rs/einfach(q,rho,angleQ);
        side = 1;
        if ((chi>discRadius.y) || (chi<discRadius.x)) {
            angleQ += PI;
            chi = rs/einfach(q,rho,angleQ);
            if ((chi>discRadius.y) || (chi<rs)) { chi = 0.0; side = 0;}
            else { side = 2; }
        }
    }
    else {
        chi = rs/aufwendig(q,rho,angleQ);
        side = 1;
        if (chi<discRadius.x || chi>discRadius.y) { side=3; }
    }

    if ((chi<discRadius.y) && (chi>discRadius.x)) {
        if (side==1) {color.rgb = vec3(0.5, 0, 0);}
        else if (side==2) {color.rgb = vec3(0, 0.5, 0);}
    }
   
#if 0
   if (side==0) { color = vec4(0,0,0,1); }
   else if (side==1) { color = vec4(1,0,0,1); }
   else if (side==2) { color = vec4(0,0,1,1); }
   else if (side==3) { color = vec4(0,1,0,1); }
#endif   
   FragColor = color;
}
