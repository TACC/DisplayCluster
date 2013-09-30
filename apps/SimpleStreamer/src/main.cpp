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

std::string dcStreamName = "SimpleStreamer";
bool dcParallelStreaming = false;
bool dcCompressStreaming = true;
int dcSegmentSize = 512;
bool dcInteraction = false;
char * dcHostname = NULL;
DcSocket * dcSocket = NULL;

void syntax(char * app);
void display();
void reshape(int width, int height);

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
                        dcStreamName = argv[i+1];
                        i++;
                    }
                    break;
                case 'p':
                    dcParallelStreaming = true;
                    break;
                case 's':
                    if(i+1 < argc)
                    {
                        dcSegmentSize = atoi(argv[i+1]);
                        i++;
                    }
                    break;
                case 'i':
                    dcInteraction = true;
                    break;
                case 'u':
                    dcCompressStreaming = false;
                    break;
                default:
                    syntax(argv[0]);
            }
        }
        else if(i == argc-1)
        {
            dcHostname = argv[i];
        }
    }

    if(dcHostname == NULL)
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

    // the reshape function will be called on window resize
    glutReshapeFunc(reshape);

    glClearColor(0.5, 0.5, 0.5, 1.);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // connect to DisplayCluster
    dcSocket = dcStreamConnect(dcHostname);

    if(dcSocket == NULL)
    {
        std::cerr << "could not connect to DisplayCluster host: " << dcHostname << std::endl;
        return 1;
    }

    if(dcInteraction == true)
    {
        bool success = dcStreamBindInteraction(dcSocket, dcStreamName);

        if(success != true)
        {
            std::cerr << "could not bind interaction events" << std::endl;
            return 1;
        }
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
    std::cerr << " -i                   enable interaction events (default disabled)" << std::endl;
    std::cerr << " -u                   enable uncompressed streaming (default disabled)" << std::endl;

    exit(1);
}

void display()
{
    // angles of camera rotation
    static float angleX = 0.;
    static float angleY = 0.;

    // zoom factor
    static float zoom = 1.;

    // clear color / depth buffers and setup view
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float size = 2.;
    glOrtho(-size, size, -size, size, -size, size);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(angleX, 0.0, 1.0, 0.0);
    glRotatef(angleY, -1.0, 0.0, 0.0);

    glScalef(zoom, zoom, zoom);

    // render the teapot
    glutSolidTeapot(1.);

    // send to DisplayCluster

    // get current window dimensions
    int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
    int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

    // grab the image data from OpenGL
    const int imageSize = windowWidth * windowHeight * 4;
    unsigned char * imageData = (unsigned char *)malloc(imageSize);
    glReadPixels(0,0,windowWidth,windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, (void *)imageData);

    bool success;

    if(dcParallelStreaming == true)
    {
        // use a streaming segment size of roughly <dcSegmentSize> x <dcSegmentSize> pixels
        std::vector<DcStreamParameters> parameters = dcStreamGenerateParameters(dcStreamName, 0, dcSegmentSize,dcSegmentSize, 0,0,windowWidth,windowHeight, windowWidth,windowHeight, dcCompressStreaming);

        // finally, send it to DisplayCluster
        success = dcStreamSend(dcSocket, imageData, 0,0,windowWidth,0,windowHeight, RGBA, parameters);
    }
    else
    {
        // use a single streaming segment for the full window
        DcStreamParameters parameters = dcStreamGenerateParameters(dcStreamName, 0,0,windowWidth,windowHeight, windowWidth,windowHeight, dcCompressStreaming);

        // finally, send it to DisplayCluster
        success = dcStreamSend(dcSocket, imageData, imageSize, 0,0,windowWidth,0,windowHeight, RGBA, parameters);
    }

    if(success == false)
    {
        std::cerr << "failure in dcStreamSend()" << std::endl;
        exit(1);
    }

    dcStreamIncrementFrameIndex();

    // and free the allocated image data
    free(imageData);

    glutSwapBuffers();

    // increment rotation angle according to interaction, or by a constant rate if interaction is not enabled
    // note that mouse position is in normalized window coordinates: (0,0) to (1,1)
    if(dcInteraction == true)
    {
        static float mouseX = 0.;
        static float mouseY = 0.;

        InteractionState interactionState = dcStreamGetInteractionState(dcSocket);

        float newMouseX = interactionState.mouseX;
        float newMouseY = interactionState.mouseY;

        if(interactionState.mouseLeft == true)
        {
            angleX += (newMouseX - mouseX) * 360.;
            angleY += (newMouseY - mouseY) * 360.;
        }
        else if(interactionState.mouseRight == true)
        {
            zoom += (newMouseY - mouseY);
        }

        mouseX = newMouseX;
        mouseY = newMouseY;
    }
    else
    {
        angleX += 1.;
        angleY += 1.;
    }
}

void reshape(int width, int height)
{
    // reset the viewport
    glViewport(0, 0, width, height);

    // reset the stream since we may have a different number of segments now
    dcStreamReset(dcSocket);
}
