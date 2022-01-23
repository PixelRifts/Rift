from dataclasses import dataclass

try:
    f = open('glad.h', 'r')
    out = open('gl.rf', 'w')
    raw = f.read()
    f.close()
except:
    print('Error: File not found.')
    exit(1)

out.write("""cinclude "glad/glad.h";
import "int.rf";

typedef uint GLenum;
typedef uchar GLboolean;
typedef uint GLbitfield;
typedef void GLvoid;
typedef int8_t GLbyte;
typedef uint8_t GLubyte;
typedef int16_t GLshort;
typedef uint16_t GLushort;
typedef int GLint;
typedef uint GLuint;
typedef int32_t GLclampx;
typedef int GLsizei;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void *GLeglClientBufferEXT;
typedef void *GLeglImageOES;
typedef cstring GLchar;
typedef cstring GLcharARB;
typedef uint16_t GLhalf;
typedef uint16_t GLhalfARB;
typedef int32_t GLfixed;
typedef int64_t GLintptr;
typedef int64_t GLintptrARB;
typedef uint64_t GLsizeiptr;
typedef uint64_t GLsizeiptrARB;
typedef int64_t GLint64;
typedef int64_t GLint64EXT;
typedef uint64_t GLuint64;
typedef uint64_t GLuint64EXT;
native typedef GLsync;
native struct _cl_context;
native struct _cl_event;
native typedef GLDEBUGPROC;
native typedef GLDEBUGPROCARB;
native typedef GLDEBUGPROCKHR;
native typedef CLDEBUGPROCAMD;
typedef ushort GLhalfNV;
typedef GLintptr GLvdpauSurfaceNV;
typedef void GLVULKANPROCNV;

""")

typedefs = []
typedefFuncs = []

@dataclass
class Function:
    returnType: str
    name: str
    paramList: list

print("\nGenerating gl.rf, OpenGL Version 4.6")

def isDigit(c):
    for i in range(len(c)):
        if(len(c) > 2 and c[i] == '0' and c[i+1] == 'x'):
            pass
        elif c[i] not in '0123456789':
            return False
    return True

lines = raw.split("\n")
for i in range(len(lines)):
    line = lines[i]
    linesp = line.split(" ")
    if((linesp[0] == "#define") and (len(linesp[2].split("0x")) > 1)):
        out.write(f"native int {linesp[1]};\n")
    elif((linesp[0] == "#define") and (isDigit(linesp[2]))):
        if(linesp[1] == "GL_VERSION_1_0" or linesp[1] == "GL_VERSION_1_1" or linesp[1] == "GL_VERSION_1_2" or linesp[1] == "GL_VERSION_1_3" or linesp[1] == "GL_VERSION_1_4" or linesp[1] == "GL_VERSION_1_5" or linesp[1] == "GL_VERSION_2_0" or linesp[1] == "GL_VERSION_2_1" or linesp[1] == "GL_VERSION_3_0" or linesp[1] == "GL_VERSION_3_1" or linesp[1] == "GL_VERSION_3_2" or linesp[1] == "GL_VERSION_3_3" or linesp[1] == "GL_VERSION_4_0" or linesp[1] == "GL_VERSION_4_1" or linesp[1] == "GL_VERSION_4_2" or linesp[1] == "GL_VERSION_4_3" or linesp[1] == "GL_VERSION_4_4" or linesp[1] == "GL_VERSION_4_5" or linesp[1] == "GL_VERSION_4_6"):
            continue
        out.write(f"native int {linesp[1]};\n")
    elif((linesp[0] == "#ifndef") or (linesp[0] == "#ifdef") or (linesp[0] == "#if") or (linesp[0] == "#elif") or (linesp[0] == "#else") or (linesp[0] == "#endif")):
        continue
    elif(linesp[0] == "GLAPI"):
        current_line_str = line.replace("GLAPI", "")
        current_line_str = current_line_str.replace(";", "")
        splt = current_line_str.split(" ")
        if (splt[2] == "GLAD_GL_VERSION_1_0" or splt[2] == "GLAD_GL_VERSION_1_1" or splt[2] == "GLAD_GL_VERSION_1_2" or splt[2] == "GLAD_GL_VERSION_1_3" or splt[2] == "GLAD_GL_VERSION_1_4" or splt[2] == "GLAD_GL_VERSION_1_5" or splt[2] == "GLAD_GL_VERSION_2_0" or splt[2] == "GLAD_GL_VERSION_2_1" or splt[2] == "GLAD_GL_VERSION_3_0" or splt[2] == "GLAD_GL_VERSION_3_1" or splt[2] == "GLAD_GL_VERSION_3_2" or splt[2] == "GLAD_GL_VERSION_3_3" or splt[2] == "GLAD_GL_VERSION_4_0" or splt[2] == "GLAD_GL_VERSION_4_1" or splt[2] == "GLAD_GL_VERSION_4_2" or splt[2] == "GLAD_GL_VERSION_4_3" or splt[2] == "GLAD_GL_VERSION_4_4" or splt[2] == "GLAD_GL_VERSION_4_5" or splt[2] == "GLAD_GL_VERSION_4_6"):
            continue
        func_type = splt[1]
        func_name = splt[2]
        func_name = func_name.replace("glad_", "")
        index = typedefFuncs.index(func_type)
        toWriteBuffer = ""
        toWriteBuffer += f"native{typedefs[index].returnType} {func_name}("
        for j in range(len(typedefs[index].paramList)):
            if(j == len(typedefs[index].paramList) - 1):
                toWriteBuffer += f"{typedefs[index].paramList[j]}"
            else:
                toWriteBuffer += f"{typedefs[index].paramList[j]}, "
        toWriteBuffer += ");"
        out.write(f"{toWriteBuffer}\n")
    elif(linesp[0] == "typedef"):
        current_line_str = line[7:]
        current_line_str = current_line_str.replace(";", "")
        current_line_str = current_line_str.replace("const ", "")
        current_line_str = current_line_str.replace("const*", "")
        current_line_str = current_line_str.replace("GLchar *", "cstring ")
        func_return_type = current_line_str.partition(" (")[0]
        func_name = current_line_str.partition(" (")[2].partition(")")[0].split(' ')[1]
        func_param_list = current_line_str.partition(")")[2]
        func_param_list = func_param_list.replace("(", "")
        func_param_list = func_param_list.replace(")", "")
        func_param_list = func_param_list.split(",")
        if(len(func_param_list) == 1 and func_param_list[0] == "void"):
            func_param_list = []
        typedefs.append(Function(func_return_type, func_name, func_param_list))
        typedefFuncs.append(func_name)
    elif(linesp[0] == "#define"):
        continue
    else:
        print(f"ERROR at line {i}: {line}")
        exit()