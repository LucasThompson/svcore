TEMPLATE = lib

SV_UNIT_PACKAGES = fftw3f sndfile mad oggz fishsound
load(../sv.prf)

CONFIG += sv staticlib qt thread warn_on stl rtti exceptions

TARGET = svdata

DEPENDPATH += fft fileio model ..
INCLUDEPATH += . fft model fileio ..
OBJECTS_DIR = tmp_obj
MOC_DIR = tmp_moc

# Input
HEADERS += fft/FFTDataServer.h \
           fft/FFTFileCache.h \
           fft/FFTFuzzyAdapter.h \
           fileio/AudioFileReader.h \
           fileio/AudioFileReaderFactory.h \
           fileio/BZipFileDevice.h \
           fileio/CodedAudioFileReader.h \
           fileio/CSVFileReader.h \
           fileio/CSVFileWriter.h \
           fileio/DataFileReader.h \
           fileio/DataFileReaderFactory.h \
           fileio/FileReadThread.h \
           fileio/MatrixFile.h \
           fileio/MIDIFileReader.h \
           fileio/MP3FileReader.h \
           fileio/OggVorbisFileReader.h \
           fileio/WavFileReader.h \
           fileio/WavFileWriter.h \
           model/DenseThreeDimensionalModel.h \
           model/DenseTimeValueModel.h \
           model/Model.h \
           model/NoteModel.h \
           model/PowerOfSqrtTwoZoomConstraint.h \
           model/PowerOfTwoZoomConstraint.h \
           model/RangeSummarisableTimeValueModel.h \
           model/SparseModel.h \
           model/SparseOneDimensionalModel.h \
           model/SparseTimeValueModel.h \
           model/SparseValueModel.h \
           model/TextModel.h \
           model/WaveFileModel.h
SOURCES += fft/FFTDataServer.cpp \
           fft/FFTFileCache.cpp \
           fft/FFTFuzzyAdapter.cpp \
           fileio/AudioFileReaderFactory.cpp \
           fileio/BZipFileDevice.cpp \
           fileio/CodedAudioFileReader.cpp \
           fileio/CSVFileReader.cpp \
           fileio/CSVFileWriter.cpp \
           fileio/DataFileReaderFactory.cpp \
           fileio/FileReadThread.cpp \
           fileio/MatrixFile.cpp \
           fileio/MIDIFileReader.cpp \
           fileio/MP3FileReader.cpp \
           fileio/OggVorbisFileReader.cpp \
           fileio/WavFileReader.cpp \
           fileio/WavFileWriter.cpp \
           model/DenseThreeDimensionalModel.cpp \
           model/DenseTimeValueModel.cpp \
           model/Model.cpp \
           model/NoteModel.cpp \
           model/PowerOfSqrtTwoZoomConstraint.cpp \
           model/PowerOfTwoZoomConstraint.cpp \
           model/RangeSummarisableTimeValueModel.cpp \
           model/WaveFileModel.cpp
