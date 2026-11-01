#version 330 core
layout (location = 0) in vec3 vPosition; // 정점 위치 (Attribute Index 0)
layout (location = 1) in vec3 vColor;    // 정점 색상 (Attribute Index 1)

out vec3 out_Color;

uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(vPosition, 1.0);
    out_Color = vColor;
}