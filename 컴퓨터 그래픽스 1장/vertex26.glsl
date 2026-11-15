#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
out vec3 FragPos;
out vec3 Normal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main()
{
    gl_Position = projection * view * model * vec4(vPos, 1.0);
    FragPos = vec3(model * vec4(vPos, 1.0));
    Normal = mat3(model) * vNormal;   // 모델이 회전하면 법선도 같이 회전해야 함
}
