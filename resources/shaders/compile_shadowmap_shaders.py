import os
import subprocess
import pathlib

if __name__ == '__main__':
    glslang_cmd = "glslangValidator"

    shader_list = ["simple.vert", "quad3_vert.vert", "quad.frag", "simple_shadow.frag",
                   "prepare_gbuffer.frag", "resolve_gbuffer.frag", "ssao.frag", "gaussian_blur.comp"]

    for shader in shader_list:
        subprocess.run([glslang_cmd, "-V", shader, "-o", "{}.spv".format(shader)])

