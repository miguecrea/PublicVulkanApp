#version 450

// Fullscreen triangle - no vertex buffer needed
// gl_VertexIndex: 0, 1, 2 generate a triangle that covers the screen
void main()
{
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
