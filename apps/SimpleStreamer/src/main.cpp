#include <string>
#include <iostream>

#ifdef __APPLE__
    #include <OpenGL/gl.h>
    #include <GLUT/glut.h>
#else
    #include <GL/gl.h>
    #include <GL/glut.h>
#endif

// DisplayCluster streaming
#include <dcStream.h>

std::string streamName = "SimpleStreamer";
bool parallelStreaming = false;
int segmentSize = 512;
char * hostname = NULL;
DcSocket * socket = NULL;

void syntax(char * app);
void display();

int main(int argc, char **argv)
{
    // read command-line arguments
    for(int i=1; i<argc; i++)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 'n':
                    if(i+1 < argc)
                    {
                        streamName = argv[i+1];
                        i++;
                    }
                    break;
                case 'p':
                    parallelStreaming = true;
                    break;
                case 's':
                    if(i+1 < argc)
                    {
                        segmentSize = atoi(argv[i+1]);
                        i++;
                    }
                    break;
                default:
                    syntax(argv[0]);
            }
        }
        else if(i == argc-1)
        {
            hostname = argv[i];
        }
    }

    if(hostname == NULL)
    {
        syntax(argv[0]);
    }

    // setup GLUT / OpenGL
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(1024, 768);

    glutCreateWindow("SimpleStreamer");

    // the display function will be called continuously
    glutDisplayFunc(display);
    glutIdleFunc(display);

    glClearColor(0.5, 0.5, 0.5, 1.);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // connect to DisplayCluster
    socket = dcStreamConnect(hostname);

    if(socket == NULL)
    {
        std::cerr << "could not connect to DisplayCluster host: " << hostname << std::endl;
        return 1;
    }

    // enter the main loop
    glutMainLoop();

    return 0;
}


void syntax(char * app)
{
    std::cerr << "syntax: " << app << " [options] <hostname>" << std::endl;
    std::cerr << "options:" << std::endl;
    std::cerr << " -n <stream name>     set stream name (default SimpleStreamer)" << std::endl;
    std::cerr << " -p                   enable parallel streaming (default disabled)" << std::endl;
    std::cerr << " -s <segment size>    set parallel streaming segment size (default 512)" << std::endl;

    exit(1);
}

void display()
{
    // angle of camera rotation
    static float angle = 0;

    // clear color / depth buffers and setup view
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float size = 2.;
    glOrtho(-size, size, -size, size, -size, size);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(angle, 0.0, 1.0, 1.0);

    // render the teapot
    glutSolidTeapot(1.);

    // send to DisplayCluster

    // get current window dimensions
    int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
    int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

    // grab the image data from OpenGL
    unsigned char * imageData = (unsigned char *)malloc(windowWidth * windowHeight * 4);
    glReadPixels(0,0,windowWidth,windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, (void *)imageData);

    bool success;

    if(parallelStreaming == true)
    {
        // use a streaming segment size of roughly <segmentSize> x <segmentSize> pixels
        std::vector<DcStreamParameters> parameters = dcStreamGenerateParameters(streamName, 0, segmentSize,segmentSize, 0,0,windowWidth,windowHeight, windowWidth,windowHeight);

        // finally, send it to DisplayCluster
        success = dcStreamSend(socket, imageData, 0,0,windowWidth,0,windowHeight, RGBA, parameters);
    }
    else
    {
        // use a single streaming segment for the full window
        DcStreamParameters parameters = dcStreamGenerateParameters(streamName, 0, 0,0,windowWidth,windowHeight, windowWidth,windowHeight);

        // finally, send it to DisplayCluster
        success = dcStreamSend(socket, imageData, 0,0,windowWidth,0,windowHeight, RGBA, parameters);
    }

    if(success == false)
    {
        std::cerr << "failure in dcStreamSend()" << std::endl;
        exit(1);
    }

    // and free the allocated image data
    free(imageData);

    glutSwapBuffers();

    // increment rotation angle
    angle += 1.;
}
