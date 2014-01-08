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
#include "dcstream/Stream.h"

bool dcInteraction = false;
bool dcCompressImage = true;
unsigned int dcCompressionQuality = 75;
char * dcHostname = NULL;
std::string dcStreamName = "SimpleStreamer";
dc::Stream* dcStream = NULL;

void syntax(char * app);
void readCommandLineArguments(int argc, char **argv);
void initGLWindow(int argc, char **argv);
void initDCStream();
void display();
void reshape(int width, int height);


void cleanup()
{
    delete dcStream;
}

int main(int argc, char **argv)
{
    readCommandLineArguments(argc, argv);

    if(dcHostname == NULL)
    {
        syntax(argv[0]);
    }

    initGLWindow(argc, argv);

    initDCStream();

    atexit( cleanup );

    // enter the main loop
    glutMainLoop();

    return 0;
}

void readCommandLineArguments(int argc, char **argv)
{
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
                case 'i':
                    dcInteraction = true;
                    break;
                case 'u':
                    dcCompressImage = false;
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
}


void initGLWindow(int argc, char **argv)
{
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
}


void initDCStream()
{
    // connect to DisplayCluster
    dcStream = new dc::Stream(dcStreamName, dcHostname);
    if (!dcStream->isConnected())
    {
        std::cerr << "Could not connect to host!" << std::endl;
        delete dcStream;
        exit(1);
    }
}


void syntax(char * app)
{
    std::cerr << "syntax: " << app << " [options] <hostname>" << std::endl;
    std::cerr << "options:" << std::endl;
    std::cerr << " -n <stream name>     set stream name (default SimpleStreamer)" << std::endl;
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
    const size_t imageSize = windowWidth * windowHeight * 4;
    unsigned char* imageData = new unsigned char[imageSize];
    glReadPixels(0,0,windowWidth,windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)imageData);

    dc::ImageWrapper dcImage((const void*)imageData, windowWidth, windowHeight, dc::RGBA);
    dcImage.compressionPolicy = dcCompressImage ? dc::COMPRESSION_ON : dc::COMPRESSION_OFF;
    dcImage.compressionQuality = dcCompressionQuality;
    dc::ImageWrapper::swapYAxis((void*)imageData, windowWidth, windowHeight, 4);
    bool success = dcStream->send(dcImage);
    dcStream->finishFrame();

    // and free the allocated image data
    delete [] imageData;

    glutSwapBuffers();

    // increment rotation angle according to interaction, or by a constant rate if interaction is not enabled
    // note that mouse position is in normalized window coordinates: (0,0) to (1,1)
    if(dcInteraction)
    {
        if (dcStream->isRegisteredForEvents() || dcStream->registerForEvents())
        {
            static float mouseX = 0.;
            static float mouseY = 0.;

            // Note: there is a risk of missing events since we only process the latest state available.
            // For more advanced applications, event processing should be done in a separate thread.
            while (dcStream->hasEvent())
            {
                const dc::Event& event = dcStream->getEvent();

                if (event.type == dc::Event::EVT_CLOSE)
                {
                    std::cout << "Received close..." << std::endl;
                    exit(0);
                }

                const float newMouseX = event.mouseX;
                const float newMouseY = event.mouseY;

                if(event.mouseLeft)
                {
                    angleX += (newMouseX - mouseX) * 360.;
                    angleY += (newMouseY - mouseY) * 360.;
                }
                else if(event.mouseRight)
                {
                    zoom += (newMouseY - mouseY);
                }

                mouseX = newMouseX;
                mouseY = newMouseY;
            }
        }
    }
    else
    {
        angleX += 1.;
        angleY += 1.;
    }
    if(!success)
    {
        if (!dcStream->isConnected())
        {
            std::cout << "Stream closed, exiting." << std::endl;
            exit(0);
        }
        else
        {
            std::cerr << "failure in dcStreamSend()" << std::endl;
            exit(1);
        }
    }
}

void reshape(int width, int height)
{
    // reset the viewport
    glViewport(0, 0, width, height);
}
