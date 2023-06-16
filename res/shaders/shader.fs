#version 330 core

out vec4 color;

in vec2 texCoord;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_diffuse2;
uniform sampler2D texture_diffuse3;
uniform sampler2D texture_specular1;
uniform sampler2D texture_specular2;

void main()
{
    color = mix(texture(texture_diffuse1, texCoord), texture(texture_specular1, texCoord), 0.5);    
};