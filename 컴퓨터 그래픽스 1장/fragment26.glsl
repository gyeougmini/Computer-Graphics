#version 330 core
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform vec3 lightPos;      // 조명 위치 (월드 좌표)
uniform vec3 lightColor;    // 조명 색
uniform vec3 objectColor;   // 객체 색
uniform vec3 viewPos;       // 카메라 위치
uniform bool lightOn;       // 조명 on/off (M키)

void main ()
{
    // 주변 조명: 조명이 꺼져 있어도 항상 적용
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0);

    vec3 result = ambient * objectColor;

    if (lightOn) {
        // 산란 반사
        vec3 normalVector = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diffuseLight = max(dot(normalVector, lightDir), 0.0);
        vec3 diffuse = diffuseLight * lightColor;

        // 거울 반사
        float shininess = 128.0;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, normalVector);
        float specularLight = max(dot(viewDir, reflectDir), 0.0);
        specularLight = pow(specularLight, shininess);
        vec3 specular = specularLight * lightColor;

        result = (ambient + diffuse + specular) * objectColor;
    }

    FragColor = vec4(result, 1.0);
}