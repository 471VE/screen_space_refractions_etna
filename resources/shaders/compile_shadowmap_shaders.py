import os
import subprocess
import pathlib

if __name__ == '__main__':
    glslang_cmd = "glslangValidator"

    shader_list = ["render_scene.vert", "prepare_gbuffer.frag", "fullscreen_quad.vert",
                   "resolve_gbuffer.vert", "resolve_gbuffer.frag", "ssao.frag", "gaussian_blur.comp"]

    for shader in shader_list:
        subprocess.run([glslang_cmd, "-V", shader, "-o", "{}.spv".format(shader)])

