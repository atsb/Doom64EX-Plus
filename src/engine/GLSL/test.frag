#version 220 core

varying vec4 diffuse,ambient;
varying vec3 normal,lightDir,halfVector;

void main()
{
    vec3 n,halfV;
    float NdotL,NdotHV;
    
    /* O termo ambiente sempre estará presente */
    vec4 color = ambient;
    
    /* como o FS não pode escrever em uma varying, usamos uma variavel adicional
       para guardar a normal 'normalizada' */
    n = normalize(normal);
    
    /* faz o produto escalar entre a normal e a direcao */
    NdotL = max(dot(n,lightDir),0.0);

    /* calcula a cor final do fragmento */
    if (NdotL > 0.0) {
        color += diffuse * NdotL;
        halfV = normalize(halfVector);
        NdotHV = max(dot(n,halfV),0.0);
        color += gl_FrontMaterial.specular *
                gl_LightSource[0].specular *
                pow(NdotHV, gl_FrontMaterial.shininess);
    }

    gl_FragColor = color;
}
