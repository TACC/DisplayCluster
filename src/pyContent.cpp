/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include <iostream>
#include <string>
#include <dlfcn.h>

#include "pyContent.h"
#include "Content.h"


pyContent::pyContent(const char *uri)
{
#if 0
    if (! getContent)
    {
        void *handle = dlopen(NULL, RTLD_LAZY);
        if (! handle)
        {
            std::cerr << "unable to dlopen app " << dlerror() << "\n";
            exit(1);
        }

        getContent = (boost::shared_ptr<Content>(*)(const std::string&))dlsym(handle, "getContent");
        if (! getContent) 
        {
            std::cerr << "unable to get Content getter " << dlerror() << "\n";
            exit(1);
        }
    }
#endif
    boost::shared_ptr<Content> c = Content::getContent(std::string(uri));
    ptr_ = c;
}

pyContent::pyContent(boost::shared_ptr<Content> ptr)
{
    ptr_ = ptr;
}

boost::shared_ptr<Content> 
pyContent::get()
{
  return ptr_;
}

char *pyContent::getURI()
{
    std::string s = ptr_->getURI();
    const char *a = s.c_str();
    char *b = new char[strlen(a) + 1];
    strcpy(b, a);
    return b;
}

PyObject *
pyContent::getDimensions()
{
    int w, h;
    ptr_->getDimensions(w, h);
    return Py_BuildValue("(ii)", w, h);
}

int pyContent::getWidth() 
{
    int w, h;
    ptr_->getDimensions(w, h);
    return w;
}
   
int pyContent::getHeight()
{
    int w, h;
    ptr_->getDimensions(w, h);
    return h;
}


