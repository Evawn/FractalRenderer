#version 330

uniform mat4 mPi;  // projection matrix inverse
uniform mat4 mVi;
uniform mat4 turnMat;
uniform int toggle;

// uniform float ang1;
// uniform float ang2;
// uniform float bias;

uniform float angles[2];
uniform float bias[2];
uniform float theta;

//in vec3 vNormal;
in vec3 vPosition;
in vec2 geom_texCoord;

out vec4 fragColor;

const float PI = 3.14159265358979323846264;

const float threshold = 0.005;
vec3 pdir1 = normalize(vec3(sin(angles[0] * PI / 180), cos(angles[0] * PI / 180), 0));
vec3 pdir2 = normalize(vec3(sin(angles[1] * PI / 180), cos(angles[1] * PI / 180), 0));
vec3 planeDir = (turnMat * vec4(pdir1, 1)).xyz;
vec4 planeDirBias = vec4(planeDir, sin(theta) / 5 - bias[0]);
vec3 planeDir1 = (turnMat * vec4(pdir2, 1)).xyz;
vec4 planeDirBias1 = vec4(planeDir1, sin(theta) / 5 - bias[1]);
vec4 planeDirBias2 = vec4(vec3(0, 0, 1), -sin(theta) / 5 - 0.5);
vec4 planeDirBias3 = vec4(-normalize(vec3(1, 0, 1)), 0.5);

vec4 worldPos = vec4(1);
vec4 worldDir = vec4(0);

vec4 reflect(vec4 abcd, vec4 worldPt) {
  float val = dot(worldPt, abcd);
  vec4 testingPos = worldPt;
  if(val < 0) {
    testingPos -= 2 * val * vec4(abcd.xyz, 0);
  }
  return testingPos;
}

float sdBox(vec3 pt, vec3 b) {
  vec3 box = abs(pt) - b;
  return length(max(box, 0.0)) + min(max(box.x, max(box.y, box.z)), 0.0);
}

mat4 rotationX(in float angle) {
  return mat4(1.0, 0, 0, 0, 0, cos(angle), -sin(angle), 0, 0, sin(angle), cos(angle), 0, 0, 0, 0, 1);
}

mat4 rotationY(in float angle) {
  return mat4(1.0, 0, 0, 0, 0, cos(angle), -sin(angle), 0, 0, sin(angle), cos(angle), 0, 0, 0, 0, 1);
}

// mandlebulb
float distanceEstimatorMandlebulb(inout vec4 testingPos, vec4 pos) {
  testingPos = turnMat * pos;
  testingPos = reflect(planeDirBias, testingPos);
  testingPos = reflect(planeDirBias1, testingPos);
  testingPos = reflect(planeDirBias2, testingPos);
  testingPos = reflect(planeDirBias3, testingPos);
  pos = testingPos;

  vec3 z = pos.xyz;
  float dr = 1.0;
  float r = 0.0;
  int Power = 6;

  for(int i = 0; i < 50; i++) {
    r = length(z);
    if(r > 50)
      break;

		// convert to polar coordinates
    float theta = acos(z.z / r);
    float phi = atan(z.y, z.x);
    dr = pow(r, Power - 1.0) * Power * dr + 1.0;

		// scale and rotate the point
    float zr = pow(r, Power);
    theta = theta * Power;
    phi = phi * Power;

		// convert back to cartesian coordinates
    z = zr * vec3(sin(theta) * cos(phi), sin(phi) * sin(theta), cos(theta));
    z += pos.xyz;
  }
  return 0.5 * log(r) * r / dr;
}

float mengerDE(inout vec4 testingPos, vec4 pos) {
  testingPos = turnMat * pos;
  testingPos = reflect(planeDirBias, testingPos);
  testingPos = reflect(planeDirBias1, testingPos);
  testingPos = reflect(planeDirBias2, testingPos);
  testingPos = reflect(planeDirBias3, testingPos);
  pos = testingPos;

  float scale = 1.0;
  float offset = 0.;
  float iterations = 3.;

  vec3 pt = pos.xyz;

  float dist = sdBox(vec3(pt.x, pt.y + offset, pt.z), vec3(scale));

  float s = 2;

  float da, db, dc;

  for(int i = 0; i < 4; i++) {
    vec3 a = mod(pt * s, 2.0) - 1.0;
    s *= iterations;
    vec3 r = abs(1.0 - 3.0 * abs(a));

    r = (vec4(r, 1.0) * rotationX(20.)).xyz;
    da = max(r.x + 1.5, r.y);
    r = (vec4(r, 1.0) * rotationY(80.)).xyz;

    da = max(da + r.x - 0.5, r.y);
    db = max(r.y, r.z);
    dc = max(r.z + 0.5, r.x);

    float c = (min(da, min(db, dc)) - 1.) / s;
    if(c > dist)
      dist = c;
  }

  return dist;
}

//sphere
float distanceEstimatorSphere(vec4 position) {
  return length(position.xyz) - 2;
}

//cube
float distanceEstimatorCube(vec4 position) {
  float size = 1;
  float x = position.x;
  float y = position.y;
  float z = position.z;
  float distance = 0;

  distance = sqrt(pow(max(0, abs(x) - size), 2) + pow(max(0, abs(y) - size), 2) + pow(max(0, abs(z) - size), 2));
  return distance;
}

float get_glow(float minDistance) {
  return 0.8 - 40 * minDistance;
}

void main() {
  vec4 NDC = vec4(geom_texCoord * 2 - 1, 0, 1);
  vec4 eyeSpace = mPi * NDC;
  eyeSpace = eyeSpace / eyeSpace.w;
  vec4 dir = normalize(vec4(eyeSpace.xyz, 0));

  vec4 pos = vec4(0, 0, 0, 1);
  worldPos = mVi * pos;
  worldDir = mVi * dir;

  float travelled = 0;
  float steps = 0;
  float distance = 100000;
  float minDistance = 100000;

  vec4 testingPos = worldPos;

  while(travelled < 50 && distance > threshold) {

    //distance = distanceEstimatorMandlebulb(testingPos, worldPos);

    distance = toggle == 0 ? distanceEstimatorMandlebulb(testingPos, worldPos) : mengerDE(testingPos, worldPos);
    travelled += distance;
    worldPos = worldPos + worldDir * distance;
    minDistance = min(minDistance, distance);
    steps += 1;
  }
  //testingPos = worldPos;

  if(travelled < 50) {
    vec3 color = vec3(1, 1, 1);
    if(toggle == 0) {
      color = vec3(length(testingPos.xyz) / 2, 0.2, .5);

    } else {
      color = vec3(.6, length(testingPos.xyz) / 2, .8);
    }
    fragColor = vec4(color - steps / 100, 1);

    //fragColor = vec4(color, 1);

  } else {
    float glow = get_glow(minDistance);
    vec3 glow_hue = vec3(0.9, 1, 0.7);

    fragColor = vec4(glow_hue * glow, 0);
    fragColor = vec4(0, 0, 0, 1);
  }
}