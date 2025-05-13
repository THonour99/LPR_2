QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    loginwindow.cpp \
    platerecognitionwindow.cpp \
    parkingreservationwindow.cpp \
    dbmanager.cpp

HEADERS += \
    mainwindow.h \
    loginwindow.h \
    platerecognitionwindow.h \
    parkingreservationwindow.h \
    dbmanager.h

FORMS += \
    mainwindow.ui \
    loginwindow.ui \
    platerecognitionwindow.ui \
    parkingreservationwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# OpenCV配置
INCLUDEPATH += $$PWD/include
LIBS += -L$$PWD/lib \
    -lopencv_core460 \
    -lopencv_imgproc460 \
    -lopencv_highgui460 \
    -lopencv_imgcodecs460 \
    -lopencv_videoio460 \
    -lopencv_video460 \
    -lopencv_calib3d460 \
    -lopencv_photo460 \
    -lopencv_features2d460 \
    -lopencv_objdetect460 \
    -lopencv_face460
