#ifndef SKELETON_THREAD_H
#define SKELETON_THREAD_H

#include <QThread>
#include <QtGui>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include "SkeletonState.h"

class SkeletonSensor;

// SkeletonThread: recieves updates from the OpenNI device context,
// and interprets the user skeletons detected within the device FOV
class SkeletonThread : public QThread {
    Q_OBJECT

    public:

        SkeletonThread();
        ~SkeletonThread();

        std::vector< boost::shared_ptr<SkeletonState> > getSkeletons();

    protected:

        void run();

    signals:

        void updateSkeletonsFinished();
        void skeletonsUpdated(std::vector< boost::shared_ptr<SkeletonState> > skeletons);

    public slots:

        void updateSkeletons();

    private:

        QTimer timer_;

        // the skeleton tracking sensor
        SkeletonSensor* sensor_;

        // the current state of each tracked user
        std::map<unsigned int, boost::shared_ptr<SkeletonState> > states_;

};

#endif
