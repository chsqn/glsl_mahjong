// 判断和了

#include "GL/gl3w.h"
#include "GL/glfw3.h"
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

const GLuint NO_TILE = 0x00; // 没有牌

const GLuint TILE_1M = 0x11; // 一萬 🀇
const GLuint TILE_2M = 0x12; // 二萬 🀈
const GLuint TILE_3M = 0x13; // 三萬 🀉
const GLuint TILE_4M = 0x14; // 四萬 🀊
const GLuint TILE_5M = 0x15; // 五萬 🀋
const GLuint TILE_6M = 0x16; // 六萬 🀌
const GLuint TILE_7M = 0x17; // 七萬 🀍
const GLuint TILE_8M = 0x18; // 八萬 🀎
const GLuint TILE_9M = 0x19; // 九萬 🀏

const GLuint TILE_1P = 0x21; // 一筒 🀙
const GLuint TILE_2P = 0x22; // 二筒 🀚
const GLuint TILE_3P = 0x23; // 三筒 🀛
const GLuint TILE_4P = 0x24; // 四筒 🀜
const GLuint TILE_5P = 0x25; // 五筒 🀝
const GLuint TILE_6P = 0x26; // 六筒 🀞
const GLuint TILE_7P = 0x27; // 七筒 🀟
const GLuint TILE_8P = 0x28; // 八筒 🀠
const GLuint TILE_9P = 0x29; // 九筒 🀡

const GLuint TILE_1S = 0x31; // 一索 🀐
const GLuint TILE_2S = 0x32; // 二索 🀑
const GLuint TILE_3S = 0x33; // 三索 🀒
const GLuint TILE_4S = 0x34; // 四索 🀓
const GLuint TILE_5S = 0x35; // 五索 🀔
const GLuint TILE_6S = 0x36; // 六索 🀕
const GLuint TILE_7S = 0x37; // 七索 🀖
const GLuint TILE_8S = 0x38; // 八索 🀗
const GLuint TILE_9S = 0x39; // 九索 🀘

const GLuint TILE_1Z = 0x41; // 東 🀀
const GLuint TILE_2Z = 0x49; // 南 🀁
const GLuint TILE_3Z = 0x51; // 西 🀂
const GLuint TILE_4Z = 0x59; // 北 🀃
const GLuint TILE_5Z = 0x61; // 白 🀆
const GLuint TILE_6Z = 0x69; // 發 🀅
const GLuint TILE_7Z = 0x71; // 中 🀄

const GLuint INDEX_IN_BUFFER  = 0;
const GLuint INDEX_OUT_BUFFER = 1;

static GLuint s_prog_hule = 0;
static GLuint s_buffer[2] = { 0, 0, };

void setup_rc();
void clean_rc();
int read_shoupai(GLuint out_shoupai[14], const char *buf);
int calc_hule(const GLuint shoupai[14]);

int main(int argc, char *argv[])
{
    if (glfwInit () != GLFW_TRUE)
    {
        printf ("glfwInit fail\n");
        return -1;
    }

    glfwWindowHint (GLFW_VISIBLE, GLFW_FALSE);
    auto *window = glfwCreateWindow (1, 1, "glsl_majhong", nullptr, nullptr);
    if (window == nullptr)
    {
        printf ("glfwCreateWindow fail\n");
        return -1;
    }

    glfwMakeContextCurrent (window);

    if (gl3wInit () != GL3W_OK)
    {
        fprintf (stderr, "gl3wInit fail\n");
        return -1;
    }

    setup_rc ();
    printf ("input tiles, e.g. \"19m19p19s1234567z 7z\", or \"q\" to quit\n");

    while (1)
    {
        enum { BUF_SIZE = 0x40 };
        char buf[BUF_SIZE] = "";
        fgets (buf, BUF_SIZE, stdin);
        if (buf[0] == 'q')
        {
            break;
        }

        GLuint shoupai[14] = {};
        int tile_n = read_shoupai (shoupai, buf);
        if (tile_n <= 0)
        {
            printf ("bad tiles\n");
            continue;
        }

        int hule = calc_hule (shoupai);
        printf ("hule = %d\n", hule);
    }

    clean_rc ();
    glfwTerminate ();

    return 0;
}

GLuint load_shader(const char *filename)
{
    FILE *cs_stream = fopen (filename, "rb");
    if (cs_stream == nullptr)
    {
        printf ("fopen \"%s\" fail\n", filename);
        exit (-1);
    }

    fseek (cs_stream, 0, SEEK_END);
    GLint length = ftell (cs_stream);

    GLchar *sourse = (GLchar *) malloc (length + 1);
    if (sourse == nullptr)
    {
        printf ("malloc fail\n");
        exit (-1);
    }

    fseek (cs_stream, 0, SEEK_SET);
    size_t read_ans = fread (sourse, length, 1, cs_stream);
    if (read_ans != 1)
    {
        printf ("fread fail\n");
        exit (-1);
    }

    fclose (cs_stream);
    cs_stream = nullptr;
    sourse[length] = '\0';

    GLuint cs = glCreateShader (GL_COMPUTE_SHADER);
    if (cs == 0)
    {
        fprintf (stderr, "glCreateShader GL_COMPUTE_SHADER fail\n");
        exit (-1);
    }

    glShaderSource (cs, 1, &sourse, &length);
    glCompileShader (cs);

    GLint compile_status = 0;
    GLint info_log_length = 0;
    glGetShaderiv (cs, GL_COMPILE_STATUS, &compile_status);
    glGetShaderiv (cs, GL_INFO_LOG_LENGTH, &info_log_length);

    if (info_log_length > 1)
    {
        GLchar *info_log = (GLchar *) alloca (info_log_length);
        glGetShaderInfoLog (cs, info_log_length, nullptr, info_log);
        fprintf (stderr, "glCompileShader \"%s\": %s\n", filename, info_log);
    }
    if (compile_status == GL_FALSE)
    {
        exit (-1);
    }

    free (sourse);
    sourse = nullptr;

    return cs;
}

GLuint link_program(GLuint cs, const char *name)
{
    GLuint prog = glCreateProgram ();
    if (prog == 0)
    {
        fprintf (stderr, "glCreateProgram fail\n");
        exit (-1);
    }

    glAttachShader(prog, cs);
    glLinkProgram(prog);

    GLint link_status = 0;
    GLint info_log_length = 0;
    glGetProgramiv (prog, GL_LINK_STATUS, &link_status);
    glGetProgramiv (prog, GL_INFO_LOG_LENGTH, &info_log_length);

    if (info_log_length > 1)
    {
        GLchar *info_log = (GLchar *) alloca (info_log_length);
        glGetProgramInfoLog (prog, info_log_length, nullptr, info_log);
        fprintf (stderr, "glLinkProgram \"%s\": %s\n", name, info_log);
    }
    if (link_status == GL_FALSE)
    {
        exit (-1);
    }

    glValidateProgram (prog);

    GLint validate_status = 0;
    glGetProgramiv (prog, GL_VALIDATE_STATUS, &validate_status);
    glGetProgramiv (prog, GL_INFO_LOG_LENGTH, &info_log_length);
    if (info_log_length > 1)
    {
        GLchar *info_log = (GLchar *) alloca (info_log_length);
        glGetProgramInfoLog (prog, info_log_length, nullptr, info_log);
        fprintf (stderr, "glValidateProgram \"%s\": %s\n", name, info_log);
    }
    if (validate_status == GL_FALSE)
    {
        exit (-1);
    }

    return prog;
}

void setup_rc()
{
    GLuint cs_hule = load_shader ("shaders/hule.cs");
    s_prog_hule = link_program (cs_hule, "hule");
    glDeleteShader (cs_hule);
    glGenBuffers (2, s_buffer);

    glBindBufferBase (GL_SHADER_STORAGE_BUFFER,
        INDEX_IN_BUFFER, s_buffer[INDEX_IN_BUFFER]);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER,
        INDEX_OUT_BUFFER, s_buffer[INDEX_OUT_BUFFER]);

    return;
}

void clean_rc()
{
    glUseProgram (0);
    glDeleteProgram (s_prog_hule);
    glDeleteBuffers (2, s_buffer);

    return;
}

int read_shoupai(GLuint out_shoupai[14], const char *buf)
{
    int tile_n = 0;
    unsigned digits[14] = {};
    int num = 0;
    while (1)
    {
        char ch = *buf;
        if (ch >= '1' && ch <= '9')
        {
            if (tile_n + num >= 14)
            {
                return -1;
            }

            digits[num] = ch - '1';
            ++num;
            ++buf;
            continue;
        }

        unsigned bias = 0;
        unsigned scale = 1;

        if (ch == 'm')
        {
            bias = TILE_1M;
        }
        else if (ch == 'p')
        {
            bias = TILE_1P;
        }
        else if (ch == 's')
        {
            bias = TILE_1S;
        }
        else if (ch == 'z')
        {
            bias = TILE_1Z;
            scale = 8;
        }
        else if (ch == ' ' || ch == '\t' || ch == '\n')
        {
            if (num > 0)
            {
                return -1;
            }

            ++buf;
            continue;
        }
        else if (ch != '\0')
        {
            return -1;
        }
        else
        {
            break;
        }

        for (int i = 0; i < num; i++)
        {
            if (ch == 'z' && digits[i] >= 7)
            {
                return -1;
            }

            out_shoupai[tile_n + i] = bias + scale * digits[i];
        }

        tile_n += num;
        num = 0;
        ++buf;
        continue;
    }

    for (int i = tile_n; i < 14; i++)
    {
        out_shoupai[i] = NO_TILE;
    }

    return tile_n;
}

int calc_hule(const GLuint shoupai[14])
{
    glUseProgram (s_prog_hule);

    glBindBuffer (GL_SHADER_STORAGE_BUFFER,
        s_buffer[INDEX_IN_BUFFER]);
    glBufferData (GL_SHADER_STORAGE_BUFFER,
        sizeof (GLuint[14]), shoupai, GL_STATIC_DRAW);

    GLuint out_data[1] = { 0 };
    glBindBuffer (GL_SHADER_STORAGE_BUFFER,
        s_buffer[INDEX_OUT_BUFFER]);
    glBufferData (GL_SHADER_STORAGE_BUFFER,
        sizeof (GLuint[1]), out_data, GL_STATIC_READ);

    glDispatchCompute (1, 1, 1);
    glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT);
    glFinish ();

    glGetBufferSubData (GL_SHADER_STORAGE_BUFFER,
        0, sizeof (GLuint[1]), out_data);

    glBindBuffer (GL_SHADER_STORAGE_BUFFER, 0);

    return (int) out_data[0];
}

// end of file
