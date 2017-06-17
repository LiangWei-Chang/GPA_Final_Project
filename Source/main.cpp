#include "../Externals/Include/Include.h"

#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

using namespace glm;
using namespace std;

char** loadShaderSource(const char* file)
{
    FILE* fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *src = new char[sz + 1];
    fread(src, sizeof(char), sz, fp);
    src[sz] = '\0';
    char **srcp = new char*[1];
    srcp[0] = src;
    return srcp;
}

void freeShaderSource(char** srcp)
{
    delete[] srcp[0];
    delete[] srcp;
}

// define a simple data structure for storing texture image raw data
typedef struct _TextureData
{
    _TextureData(void) :
    width(0),
    height(0),
    data(0)
    {
    }
    
    int width;
    int height;
    unsigned char* data;
} TextureData;

struct Shape{
    GLuint vao;
    GLuint vbo_position;
    GLuint vbo_normal;
    GLuint vbo_texcoord;
    GLuint vbo_tangent;
    GLuint ibo;
    int drawCount;
    int materialID;
};

struct Material
{
    GLuint diffuse_tex;
    int discard;
};

class Scene{
public:
    vector<Material> materials;
    vector<Shape> shapes;
    Scene(){
        materials.clear();
        shapes.clear();
    }
    ~Scene(){
        materials.clear();
        shapes.clear();
    }
};

mat4 mv_matrix;
mat4 proj_matrix;
mat4 rotation;
mat4 translation;
GLuint program;
GLuint skybox_program;
GLuint cubemapTexture;
GLuint skyboxVAO;
GLint um4mv_location;       // id of mvp
GLint um4p_location;        // id of the texture
GLint tex_color_location;
GLint skybox_location;
GLint view_matrix_location;
GLint discard_location;

Scene *sceneNow;

const float cameraSpeed = 0.1f;
float up_angle = 0.0f, left_right = 0.0f;
vec3 cameraPos;
vec3 cameraFront;
vec3 cameraUp;
int lastX, lastY;
bool pressed = false;
map<pair<float, float>, float> zPosiotion;
map<pair<float, float>, int> numCount;

// load a png image and return a TextureData structure with raw data
// not limited to png format. works with any image format that is RGBA-32bit
TextureData loadPNG(const char* const pngFilepath)
{
    TextureData texture;
    int components;
    
    // load the texture with stb image, force RGBA (4 components required)
    stbi_uc *data = stbi_load(pngFilepath, &texture.width, &texture.height, &components, 4);
    
    // is the image successfully loaded?
    if (data != NULL)
    {
        // copy the raw data
        size_t dataSize = texture.width * texture.height * 4 * sizeof(unsigned char);
        texture.data = new unsigned char[dataSize];
        memcpy(texture.data, data, dataSize);
        
        // mirror the image vertically to comply with OpenGL convention
        for (size_t i = 0; i < texture.width; ++i)
        {
            for (size_t j = 0; j < texture.height / 2; ++j)
            {
                for (size_t k = 0; k < 4; ++k)
                {
                    size_t coord1 = (j * texture.width + i) * 4 + k;
                    size_t coord2 = ((texture.height - j - 1) * texture.width + i) * 4 + k;
                    std::swap(texture.data[coord1], texture.data[coord2]);
                }
            }
        }
        
        // release the loaded image
        stbi_image_free(data);
    }
    
    return texture;
}

GLuint loadCubemap(vector<const char *> faces){
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    glGenVertexArrays(1, &skyboxVAO);
    glBindVertexArray(skyboxVAO);
    TextureData face;
    for(GLuint i = 0; i < faces.size(); i++){
        face = loadPNG(faces[i]);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, face.width, face.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, face.data);
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    
    return textureID;
}

Scene* LoadSceneByAssimp(const char *objPath, const char *texPath){
    Scene *scene_datas = new Scene();
    const aiScene *scene = aiImportFile(objPath, aiProcessPreset_TargetRealtime_MaxQuality);
    if(!scene) cout << "Unable to load scene\n";
    
    for(unsigned int i = 0; i < scene->mNumMaterials; i++){
        aiMaterial *material = scene->mMaterials[i];
        Material my_material;
        aiString texturePath;
        if(material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS){
            aiString path;
            path.Set(texPath);
            path.Append(texturePath.C_Str());
            // load width, height and data from texturePath.C_Str();

            TextureData texture = loadPNG(path.C_Str());
            
            glActiveTexture(GL_TEXTURE0);
            glGenTextures(1, &my_material.diffuse_tex);
            glBindTexture(GL_TEXTURE_2D, my_material.diffuse_tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture.data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else{
            my_material.diffuse_tex = 1;
        }
        scene_datas->materials.push_back(my_material);
    }
    
    for(unsigned int i = 0; i < scene->mNumMeshes; i++){
        aiMesh *mesh = scene->mMeshes[i];
        Shape shape;
        glGenVertexArrays(1, &shape.vao);
        glBindVertexArray(shape.vao);
        
        if(mesh->HasPositions()){
            float *positions = new float[mesh->mNumVertices * 3];
            
            for (unsigned int v = 0; v < mesh->mNumVertices; v++){
                // mesh->mVertices[v][0~2] => position
                positions[v * 3] = mesh->mVertices[v][0];
                positions[v * 3 + 1] = mesh->mVertices[v][1];
                positions[v * 3 + 2] = mesh->mVertices[v][2];
                float nextX = (int)(positions[v*3]*10)/10.0;
                float nextY = (int)(positions[v*3+2]*10)/10.0;

                if(positions[v * 3 + 1] > zPosiotion[make_pair(nextX, nextY)]){
                    zPosiotion[make_pair(nextX, nextY)] = positions[v*3+1];
                    numCount[make_pair(nextX, nextY)]++;
                }
            }
            
            glGenBuffers(1, &shape.vbo_position);
            glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_position);
            glBufferData(GL_ARRAY_BUFFER, 3 * mesh->mNumVertices * sizeof(GL_FLOAT), positions, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray(0);
            
            delete[] positions;
        }
        
        if(mesh->HasTextureCoords(0)){
            float *texcoords = new float[mesh->mNumVertices * 2];
            
            for (unsigned int v = 0; v < mesh->mNumVertices; v++){
                // mesh->mTextureCoords[0][v][0~1] => texcoord
                texcoords[v * 2] = mesh->mTextureCoords[0][v][0];
                texcoords[v * 2 + 1] = mesh->mTextureCoords[0][v][1];
            }
            
            glGenBuffers(1, &shape.vbo_texcoord);
            glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_texcoord);
            glBufferData(GL_ARRAY_BUFFER, 2 * mesh->mNumVertices * sizeof(GL_FLOAT), texcoords, GL_STATIC_DRAW);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray(1);
            
            delete[] texcoords;
        }
        
        if(mesh->HasNormals()){
            float *normals = new float[mesh->mNumVertices * 3];
            
            for (unsigned int v = 0; v < mesh->mNumVertices; v++){
                // mesh->mNormals[v][0~2] => normal
                normals[v * 3] = mesh->mNormals[v][0];
                normals[v * 3 + 1] = mesh->mNormals[v][1];
                normals[v * 3 + 2] = mesh->mNormals[v][2];
            }
            
            glGenBuffers(1, &shape.vbo_normal);
            glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_normal);
            glBufferData(GL_ARRAY_BUFFER, 3 * mesh->mNumVertices * sizeof(GL_FLOAT), normals, GL_STATIC_DRAW);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray(2);
            
            delete[] normals;
        }
        
        if(mesh->HasFaces()){
            unsigned int *indices = new unsigned int[mesh->mNumFaces * 3];
            
            for (unsigned int f = 0; f < mesh->mNumFaces; f++){
                // mesh->mFaces[f].mIndices[0~2] => index
                indices[f * 3] = mesh->mFaces[f].mIndices[0];
                indices[f * 3 + 1] = mesh->mFaces[f].mIndices[1];
                indices[f * 3 + 2] = mesh->mFaces[f].mIndices[2];
            }
            
            glGenBuffers(1, &shape.ibo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.ibo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * mesh->mNumFaces * sizeof(GLuint), indices, GL_STATIC_DRAW);
            
            delete[] indices;
        }
        shape.materialID = mesh->mMaterialIndex;
        shape.drawCount = mesh->mNumFaces * 3;
        scene_datas->shapes.push_back(shape);
    }
    
    aiReleaseImport(scene);
    return scene_datas;
}

void My_Init()
{
    glClearColor(0.0f, 0.6f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    skybox_program = glCreateProgram();
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    char** vertexShaderSource = loadShaderSource("skybox.vs.glsl");
    char** fragmentShaderSource = loadShaderSource("skybox.fs.glsl");
    glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
    glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
    freeShaderSource(vertexShaderSource);
    freeShaderSource(fragmentShaderSource);
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);
    glAttachShader(skybox_program, vertexShader);
    glAttachShader(skybox_program, fragmentShader);
    glLinkProgram(skybox_program);
    glUseProgram(skybox_program);
    
    view_matrix_location = glGetUniformLocation(skybox_program, "view_matrix");
    skybox_location = glGetUniformLocation(skybox_program, "tex_cubemap");
    
    program = glCreateProgram();
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    vertexShaderSource = loadShaderSource("vertex.vs.glsl");
    fragmentShaderSource = loadShaderSource("fragment.fs.glsl");
    glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
    glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
    freeShaderSource(vertexShaderSource);
    freeShaderSource(fragmentShaderSource);
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glUseProgram(program);
    
    um4mv_location = glGetUniformLocation(program, "um4mv");
    um4p_location = glGetUniformLocation(program, "um4p");
    tex_color_location = glGetUniformLocation(program, "tex_color");
    discard_location = glGetUniformLocation(program, "needDiscard");
    
    sceneNow = LoadSceneByAssimp("../Executable/Medieval/Medieval_City.obj", "../Executable/Medieval/");
    
    cameraPos = vec3(-10.0f, 41.0f, 0.0f);
    cameraFront = vec3(1.0f, 0.0f, 0.0f);
    cameraUp = vec3(0.0f, 1.0f, 0.0f);
    
    vector<const GLchar*> faces;
    faces.push_back("../Executable/mp_crimelem/criminal-element_rt.tga");
    faces.push_back("../Executable/mp_crimelem/criminal-element_lf.tga");
    faces.push_back("../Executable/mp_crimelem/criminal-element_dn.tga");
    faces.push_back("../Executable/mp_crimelem/criminal-element_up.tga");
    faces.push_back("../Executable/mp_crimelem/criminal-element_bk.tga");
    faces.push_back("../Executable/mp_crimelem/criminal-element_ft.tga");
    cubemapTexture = loadCubemap(faces);
    
}

void My_Display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    mat4 view_matrix = lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glUseProgram(skybox_program);
    glBindVertexArray(skyboxVAO);
    
    glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE, &view_matrix[0][0]);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);
    
    glUseProgram(program);
    glUniformMatrix4fv(um4mv_location, 1, GL_FALSE, &mv_matrix[0][0]);
    glUniformMatrix4fv(um4p_location, 1, GL_FALSE, &proj_matrix[0][0]);
    glUniform1i(tex_color_location, 0);
    
    mv_matrix = lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    
    for(int i = 0; i < sceneNow->shapes.size(); i++){
        glBindVertexArray(sceneNow->shapes[i].vao);
        int materialID = sceneNow->shapes[i].materialID;

        glBindTexture(GL_TEXTURE_2D, sceneNow->materials[materialID].diffuse_tex);
        glDrawElements(GL_TRIANGLES, sceneNow->shapes[i].drawCount, GL_UNSIGNED_INT, 0);
    }
    
    glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    
    float viewportAspect = (float)width / (float)height;
    
    proj_matrix = perspective(radians(60.0f), viewportAspect, 0.1f, 10000.0f);
    mv_matrix = lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

void My_Timer(int val)
{
    glutPostRedisplay();
    glutTimerFunc(timer_speed, My_Timer, val);
}

void My_Motion(int x, int y){
    if(pressed){
        int deltaX = x - lastX;
        int deltaY = lastY - y;
        lastX = x;
        lastY = y;
        GLfloat sensitivity = 0.25f;
        deltaX *= sensitivity;
        deltaY *= sensitivity;
        up_angle += (float)deltaY;
        left_right += (float)deltaX;
        if(up_angle > 89.0f)
            up_angle =  89.0f;
        if(up_angle < -89.0f)
            up_angle = -89.0f;
        vec3 front;
        front.x = cos(radians(up_angle)) * cos(radians(left_right));
        front.y = sin(radians(up_angle));
        front.z = cos(radians(up_angle)) * sin(radians(left_right));
        cameraFront = normalize(front);
    }
}

void My_Mouse(int button, int state, int x, int y)
{
    if(state == GLUT_DOWN)
    {
        lastX = x;
        lastY = y;
        pressed = true;
    }
    else if(state == GLUT_UP)
    {
        pressed = false;
    }
}

void My_Keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        case 'w':
        {
            vec3 temp = cameraPos + vec3((cameraSpeed * cameraFront).x, 0.0f, (cameraSpeed * cameraFront).z);
            temp = vec3((int)(temp.x*10)/10.0, (int)(temp.y*10)/10.0, (int)(temp.z*10)/10.0);
            cout << "Next Height = " << zPosiotion[make_pair(temp.x, temp.z)] << endl;
            cout << "Next Pos = (" << temp.x << ", " << temp.z << ")\n";
            if(zPosiotion[make_pair(temp.x, temp.z)] - temp.y <= 1.0f && numCount[make_pair(temp.x, temp.z)] <= 10)
                cameraPos += vec3((cameraSpeed * cameraFront).x, 0.0f, (cameraSpeed * cameraFront).z);
            break;
        }
        case 's':
            cameraPos -= vec3((cameraSpeed * cameraFront).x, 0.0f, (cameraSpeed * cameraFront).z);
            break;
        case 'a':
        {
            vec3 temp = normalize(cross(cameraFront, cameraUp)) * cameraSpeed;
            cameraPos -= vec3(temp.x, 0.0f, temp.z);
            break;
        }
        case 'd':
        {
            vec3 temp = normalize(cross(cameraFront, cameraUp)) * cameraSpeed;
            cameraPos += vec3(temp.x, 0.0f, temp.z);
            break;
        }

        case 'z':
        {
            up_angle += 1;
            if(up_angle > 89.0f)
                up_angle =  89.0f;
            if(up_angle < -89.0f)
                up_angle = -89.0f;
            vec3 front;
            front.x = cos(radians(up_angle)) * cos(radians(left_right));
            front.y = sin(radians(up_angle));
            front.z = cos(radians(up_angle)) * sin(radians(left_right));
            cameraFront = normalize(front);
        }
            break;
        case 'x':
        {
            up_angle -= 1;
            if(up_angle > 89.0f)
                up_angle =  89.0f;
            if(up_angle < -89.0f)
                up_angle = -89.0f;
            vec3 front;
            front.x = cos(radians(up_angle)) * cos(radians(left_right));
            front.y = sin(radians(up_angle));
            front.z = cos(radians(up_angle)) * sin(radians(left_right));
            cameraFront = normalize(front);
        }
            break;
        default:
            break;
    }
}

void My_SpecialKeys(int key, int x, int y)
{
    switch(key){
        case GLUT_KEY_UP:
            break;
        case GLUT_KEY_DOWN:
            break;
        case GLUT_KEY_LEFT:
            break;
        case GLUT_KEY_RIGHT:
            break;
        default:
            break;
    }
}

void My_Menu(int id)
{
    switch(id)
    {
        case MENU_TIMER_START:
            if(!timer_enabled)
            {
                timer_enabled = true;
                glutTimerFunc(timer_speed, My_Timer, 0);
            }
            break;
        case MENU_TIMER_STOP:
            timer_enabled = false;
            break;
        case MENU_EXIT:
            exit(0);
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
    // Change working directory to source code path
    chdir(__FILEPATH__("/../Assets/"));
#endif
    // Initialize GLUT and GLEW, then create a window.
    ////////////////////
    glutInit(&argc, argv);
#ifdef _MSC_VER
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(600, 600);
    glutCreateWindow("Final Project Team 12"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
    
    glewInit();
    glPrintContextInfo();
    My_Init();
    
    // Create a menu and bind it to mouse right button.
    int menu_main = glutCreateMenu(My_Menu);
    int menu_timer = glutCreateMenu(My_Menu);
    
    glutSetMenu(menu_main);
    glutAddSubMenu("Timer", menu_timer);
    glutAddMenuEntry("Exit", MENU_EXIT);
    
    glutSetMenu(menu_timer);
    glutAddMenuEntry("Start", MENU_TIMER_START);
    glutAddMenuEntry("Stop", MENU_TIMER_STOP);
    
    glutSetMenu(menu_main);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    // Register GLUT callback functions.
    glutDisplayFunc(My_Display);
    glutReshapeFunc(My_Reshape);
    glutMouseFunc(My_Mouse);
    glutMotionFunc(My_Motion);
    glutKeyboardFunc(My_Keyboard);
    glutSpecialFunc(My_SpecialKeys);
    glutTimerFunc(timer_speed, My_Timer, 0);
    
    // Enter main event loop.
    glutMainLoop();
    
    return 0;
}
