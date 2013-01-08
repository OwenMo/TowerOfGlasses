uniform sampler2D texUnit;

varying vec4 Color;
varying vec2 texCoord;
varying vec4 labelColor;
uniform int nonShadowCodeUniform;

void main() {

if(nonShadowCodeUniform == 1){
  vec4 texel = texture2D(texUnit,texCoord);
  gl_FragColor = texel * Color;
 }else{
  gl_FragColor = Color;
 }
}
