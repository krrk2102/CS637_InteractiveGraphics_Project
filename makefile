C_OPTIONS=-Wall -pedantic -I include -g
GL_OPTIONS=-lGLEW -lGL -lglut
OPTIONS=$(GCC_OPTIONS) $(GL_OPTIONS)


project:
        g++ $@.cpp Common/InitShader.cpp $(OPTIONS) -o $@


clean:
        rm project
