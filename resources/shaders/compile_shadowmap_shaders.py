import os
import subprocess
import pathlib

if __name__ == '__main__':
    glslang_cmd = "glslangValidator"

    shader_list = ["render_scene.vert", "prepare_gbuffer.frag", "resolve_gbuffer.vert", "resolve_gbuffer.frag",
                   "fullscreen_quad.vert", "ssao.frag", "gaussian_blur.comp",
                   "transparency.vert", "transparency.frag", "resolve_transparency.vert", "resolve_transparency.frag"]

    for shader in shader_list:
        subprocess.run([glslang_cmd, "-V", shader, "-o", "{}.spv".format(shader)])

