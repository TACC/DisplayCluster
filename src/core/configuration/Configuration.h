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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QString>
#include <QColor>

#include "types.h"

/**
 * @brief The Configuration class manages all the settings needed by a
 * DisplayCluster application.
 *
 * Both WallConfiguration and MasterConfiguation extend this class to provide
 * additional parameters specific to a Wall or Master process.
 *
 * In a typical DisplayWall setup, the "screens" are configured to map 1 to 1
 * with the XServer screens (which can extend to one or more physical displays).
 *
 * For testing purposes, the "screens" can also be represented as windows on a
 * single display.
 *
 * @note For historic reasons, both the words "screen" and "tile" are used in
 * xml configuration files and have the same meaning.
 */

class Configuration
{
public:
    /**
     * @brief Configuration constructor
     * @param filename path to the xml configuration file
     * @param options a pointer to options that change during runtime
     */
    Configuration(const QString& filename, OptionsPtr options);
    virtual ~Configuration() {}

    /**
     * @brief getTotalScreenCountX Get the total number of screens along the x axis
     * @return
     */
    int getTotalScreenCountX() const;

    /**
     * @brief getTotalScreenCountY Get the total number of screens along the y axis
     * @return
     */
    int getTotalScreenCountY() const;

    /**
     * @brief getScreenWidth Get the width of a screen.
     * @return width in pixel units
     * @note All the screens have the same size.
     */
    int getScreenWidth() const;

    /**
     * @brief getScreenWidth Get the height of a screen.
     * @return height in pixel units
     * @note All the screens have the same size.
     */
    int getScreenHeight() const;

    /**
     * @brief getMullionWidth Get the padding nedded to compensate for the physical displays' bezel
     * @return horizontal padding between two screens in pixel units
     */
    int getMullionWidth() const;

    /**
     * @brief getMullionHeight Get the padding nedded to compensate for the physical displays' bezel
     * @return vertical padding between two screens in pixel units
     */
    int getMullionHeight() const;

    /**
     * @brief getTotalWidth Get the total width of the DisplayWall, including the Mullion padding.
     * @return width in pixel units
     */
    int getTotalWidth() const;

    /**
     * @brief getTotalHeight Get the total height of the DisplayWall, including the Mullion padding.
     * @return height in pixel units
     */
    int getTotalHeight() const;

    /**
     * @brief getFullscreen Display the windows in fullscreen mode
     * @return
     */
    bool getFullscreen() const;

    /**
     * @brief getBackgroundUri Get the URI to the Content to be used as background
     * @return empty string if unspecified
     */
    const QString& getBackgroundUri() const;

    /**
     * @brief getBackgroundColor Get the uniform color to use for Background
     * @return defaults to black if unspecified
     */
    const QColor& getBackgroundColor() const;

    /**
     * @brief setBackgroundColor Set the background color
     * @param color
     */
    void setBackgroundColor(const QColor &color);

    /**
     * @brief setBackgroundUri Set the URI to the Content to be used as background
     * @param uri empty string to use no background content
     */
    void setBackgroundUri(const QString &uri);

    /**
     * @brief save Save the configuration to the current xml file.
     * @return true on succes, false on failure
     */
    bool save() const;

    /**
     * @brief save Save the configuration to the specified xml file.
     * @param filename destination file
     * @return true on succes, false on failure
     */
    bool save(const QString& filename) const;


protected:
    /**
     * @brief filename_ The path to the xml configuration file
     */
    QString filename_;

private:
    OptionsPtr options_;

    int totalScreenCountX_;
    int totalScreenCountY_;
    int screenWidth_;
    int screenHeight_;
    int mullionWidth_;
    int mullionHeight_;
    bool fullscreen_;

    QString backgroundUri_;
    QColor backgroundColor_;

    void load();
};

#endif
