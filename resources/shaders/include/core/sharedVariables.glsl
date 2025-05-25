#ifndef SHARED_VARIABLES_GLSL
#define SHARED_VARIABLES_GLSL

struct G_RenderInfo {
  vec3 camPosition;
  mat4 vMat;
  mat4 vMatInv;
  mat4 vMatPrev;
  mat4 vMatPrevInv;
  mat4 pMat;
  mat4 pMatInv;
  mat4 pMatPrev;
  mat4 pMatPrevInv;
  mat4 vpMat;
  mat4 vpMatInv;
  mat4 vpMatPrev;
  mat4 vpMatPrevInv;
  mat4 vpMatShadowMapCam;
  mat4 vpMatShadowMapCamInv;
  float vfov;
};

#endif // SHARED_VARIABLES_GLSL
