/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */
/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "TestFFTModel.h"

#include <QtTest>

#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    int good = 0, bad = 0;

    QCoreApplication app(argc, argv);
    app.setOrganizationName("sonic-visualiser");
    app.setApplicationName("test-model");

    {
	TestFFTModel t;
	if (QTest::qExec(&t, argc, argv) == 0) ++good;
	else ++bad;
    }

    if (bad > 0) {
	cerr << "\n********* " << bad << " test suite(s) failed!\n" << endl;
	return 1;
    } else {
	cerr << "All tests passed" << endl;
	return 0;
    }
}

