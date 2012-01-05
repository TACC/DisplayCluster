#ifndef OPTIONS_H
#define OPTIONS_H

#include <QtGui>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

class Options : public QObject {
    Q_OBJECT

    public:
        Options();

        bool getShowWindowBorders();

    public slots:
        void setShowWindowBorders(bool set);

    signals:
        void updated();

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int)
        {
            ar & showWindowBorders_;
        }

        bool showWindowBorders_;
};

#endif
