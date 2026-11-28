#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;            // 보간된 텍스처 좌표 (실습 30 추가)
out vec4 FragColor;

uniform vec3 lightPos;      // 조명 위치 (월드 좌표)
uniform vec3 lightColor;    // 조명 색
uniform vec3 objectColor;   // 객체 색 (텍스처를 안 쓸 때)
uniform vec3 viewPos;       // 카메라 위치
uniform bool lightOn;       // 조명 on/off (M키)

uniform sampler2D outTexture;   // 텍스처 샘플러 (텍스처 유닛 0)
uniform bool useTexture;        // true: 텍스처 색 사용, false: objectColor 사용 (기본값 false → 실습 28도 그대로 동작)

uniform bool  noLighting;       // true: 조명 계산 없이 원래 색 그대로 (배경 이미지용, 실습 31)
uniform float alpha;            // 투명도 0.3 ~ 1.0 (실습 31). 설정 안 하면 0 → 불투명(1.0)으로 취급

void main ()
{
    // 물체의 기본 색: 텍스처를 쓰면 texture()로 샘플링한 텍셀 색, 아니면 objectColor
    vec3 baseColor = useTexture ? texture(outTexture, TexCoord).rgb : objectColor;
    float a = (alpha > 0.0) ? alpha : 1.0;

    // 배경: 조명 영향 없이 이미지 색 그대로
    if (noLighting) {
        FragColor = vec4(baseColor, a);
        return;
    }

    // 주변 조명: 조명이 꺼져 있어도 항상 적용
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0);

    vec3 result = ambient * baseColor;

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

        // 조명 결과 * 텍스처 색 (강의자료: FragColor = texture(...) * 조명결과)
        result = (ambient + diffuse + specular) * baseColor;
    }

    FragColor = vec4(result, a);   // 알파값을 실어 보내면 glBlendFunc가 프레임 버퍼 색과 섞어줌
}