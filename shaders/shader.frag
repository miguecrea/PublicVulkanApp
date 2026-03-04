#version 450

//The triangle that is formed by the positions from the vertex shader fills an area on the screen with fragments. The fragment shader is invoked on these fragments to produce a color and depth 
//for the framebuffer (or framebuffers).
// main function is called ro every fragment


//linked to the framebuffer at index 0 

layout(location = 0) out vec4 outColor;

//  means we get it from vertex 
layout(location = 0) in vec3 fragColor; 

void main() 
{
    //outColor = vec4(1.0, 0.0, 0.0, 1.0);
    outColor = vec4(fragColor, 1.0);
}