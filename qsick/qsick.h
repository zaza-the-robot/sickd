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

#include <qcustomplot.h>
#include <QMainWindow>
#include <libsickd.h>
#include <vector>

class qsickMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	qsickMainWindow();
	virtual ~qsickMainWindow() {};
public slots:
	void poll_sensor();
private:
	void rescale_axes();
	void resizeEvent(QResizeEvent *event);
	double m_plot_data_rmax;
	uint32_t last_shmem_stamp;
	std::vector<double> sin_table;
	std::vector<double> cos_table;
	QCustomPlot *plotter;
};
