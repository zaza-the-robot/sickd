/*
 * qsick - Real-time plotting of sickd data
 *
 * Copyright (C) 2017 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * This file is part of qsick.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "qsick.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
	QApplication app (argc, argv);
	qsickMainWindow win;
	win.show();
	return app.exec();
}

void qsickMainWindow::rescale_axes()
{
	auto *view = plotter->axisRect();
	/* y axis only plots from 0, not -rmax, hence teh factor of 2 */
	double ratio = (double)view->height() * 2 / view->width();
	double rmax = m_plot_data_rmax;

	/* When there's more 'y' than 'x', shrink 'x' instead. */
	if (ratio < 1.0)
		rmax /= ratio;

	plotter->xAxis->setRange(-rmax, rmax);
	plotter->yAxis->setRange(0, rmax * ratio);
	plotter->replot();
}
void qsickMainWindow::poll_sensor(void)
{
	struct sick_packet pkt;
	double rmax;
	size_t yay, extra;

	yay = libsickd_shmem_poll(&pkt);

	if (yay == SICKD_SHMEM_STAMP_NO_DATA)
		printf("Shat\n");

	extra = yay - last_shmem_stamp;
	if (yay != last_shmem_stamp) {
		printf("Newsh\n");
		QVector<double> x(180), y(180);
		rmax = 0;

		for (int i = 0; i < 180; i++) {
			x[i] = cos_table[i] * pkt.distance[i];
			y[i] = sin_table[i] * pkt.distance[i];
			rmax = qMax(rmax, (double)pkt.distance[i]);
		}
		QCPCurve *polar = (QCPCurve *)plotter->plottable(0);
		polar->setData(x, y);
		m_plot_data_rmax = rmax;
		rescale_axes();
	}

	if (extra > 1)
		printf("Missed one\n");

	last_shmem_stamp = yay;
}

void qsickMainWindow::resizeEvent(QResizeEvent *event)
{
	rescale_axes();
}

qsickMainWindow::qsickMainWindow() :
	sin_table(180), cos_table(180)
{
	plotter = new QCustomPlot(this);
	setCentralWidget(plotter);
	last_shmem_stamp = ~0;
	libsickd_shmem_init();
	// create graph and assign data to it:
	QCPCurve *polar = new QCPCurve(plotter->xAxis, plotter->yAxis);
	plotter->addPlottable(polar);

	QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &qsickMainWindow::poll_sensor);
	timer->start(200);

	/* Used to convert radial data to cartesian coordinates. */
	for (int i = 0; i < 180; i++) {
		double theta = (double)i / 179.0 * M_PI;
		sin_table[i] = sin(theta);
		cos_table[i] = cos(theta);
	}
	m_plot_data_rmax = 1.0;
}
