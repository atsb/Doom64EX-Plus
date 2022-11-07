#version 210 core

/*
Taken from: https://www.inf.ufrgs.br/~amaciel/teaching/SIS0384-09-2/exercise9.html
*/
varying vec4 diffuse,ambient;
varying vec3 normal,lightDir,halfVector;

void main()
{	
	/* primeiro transforma a normal para o sistema da camera e normaliza o resultado */
		normal = normalize(gl_NormalMatrix * gl_Normal);
			
				/* agora normaliza a direção da luz. Lembre-se de que de acordo com a
					especificacao da OpenGL, a luz eh armazenada no sistema da camera. Lembre-se também que 
						estamos falando de luz direcional. Assim, o campo position eh na verdade a direcao.*/
							lightDir = normalize(vec3(gl_LightSource[0].position));

								/* Normaliza o vetor-meio para passá-lo ao FS */
									halfVector = normalize(gl_LightSource[0].halfVector.xyz);

													
														/* Computa os termos difuso, ambiente e ambiente global */
															diffuse = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse;
																ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;
																	ambient += gl_LightModel.ambient * gl_FrontMaterial.ambient;

																		gl_Position = ftransform();
						
}
