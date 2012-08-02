//
// Configuration
//

// System includes
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <ctime>
#include <fstream>
#include <sys/stat.h>

// OpenCV includes
#include <cv.h>
#include <highgui.h>

// Local includes
#include "auxiliary.h"
#include "functionals.h"
#include "tracetransform.h"
#include "circusfunction.h"

// Algorithm parameters
#define ANGLE_INTERVAL		1
#define DISTANCE_INTERVAL	1


//
// Auxiliary
//

struct Profiler
{
	Profiler()
	{
		t1 = clock();
	}

	void stop()
	{
		t2 = clock();		
	}

	double elapsed()
	{
		return (double)(t2-t1)/CLOCKS_PER_SEC;
	}

	time_t t1, t2;
};


//
// Main
//

enum PFunctionalType {
       REGULAR,
       HERMITE
};

int main(int argc, char **argv)
{
	// Check and read the parameters
	if (argc < 3) {
		std::cerr << "Invalid usage: " << argv[0]
			<< " INPUT T-FUNCTIONALS [P-FUNCTIONALS]" << std::endl;
		return 1;
	}
	std::string fn_input = argv[1];

	// Get the chosen T-functionals
	std::vector<TFunctional<double,double>*> tfunctionals;
	std::vector<std::string> tfunctional_names;
	std::stringstream ss;
	ss << argv[2];
	while (!ss.eof()) {
		unsigned short i;
		ss >> i;
		if (ss.fail()) {
			std::cerr << "Error: unparseable T-functional identifier" << std::endl;
			return 1;
		}

		std::stringstream name;
		switch (i) {
		case 0:
			tfunctionals.push_back(new TFunctionalRadon<double>());
			break;
		case 1:
			tfunctionals.push_back(new TFunctional1<double>());
			break;
		case 2:
			tfunctionals.push_back(new TFunctional2<double>());
			break;
		case 3:
			tfunctionals.push_back(new TFunctional3<double>());
			break;
		case 4:
			tfunctionals.push_back(new TFunctional4<double>());
			break;
		case 5:
			tfunctionals.push_back(new TFunctional5<double>());
			break;
		default:
			std::cerr << "Error: invalid T-functional provided" << std::endl;
			return 1;
		}
		name << "T" << i;
		tfunctional_names.push_back(name.str());

		if (ss.peek() == ',')
			ss.ignore();
	}

	// Get the chosen P-functional
	std::vector<PFunctional<double,double>*> pfunctionals;
	std::vector<std::string> pfunctional_names;
	int pfunctional_regular = 0, pfunctional_hermite = 0;
	if (argc >= 4) {
		ss.clear();
		ss << argv[3];
		while (!ss.eof()) {
			PFunctionalType type = REGULAR;
			if (ss.peek() == 'H') {
				type = HERMITE;
				ss.ignore();
			}

			unsigned short i;
			ss >> i;
			if (ss.fail()) {
				std::cerr << "Error: unparseable P-functional identifier" << std::endl;
				return 1;
			}

			std::stringstream name;
			switch (type) {
			case REGULAR:
			{
				pfunctional_regular++;
				switch (i) {
				case 1:
					pfunctionals.push_back(new PFunctional1<double>());
					break;
				case 2:
					pfunctionals.push_back(new PFunctional2<double>());
					break;
				case 3:
					pfunctionals.push_back(new PFunctional3<double>());
					break;
				default:
					std::cerr << "Error: invalid P-functional provided" << std::endl;
					return 1;
				}
				name << "P" << i;
				break;
			}
			case HERMITE:
				pfunctional_hermite++;
				pfunctionals.push_back(new PFunctionalHermite<double>(i));
				name << "H" << i;
				break;	
			}
			pfunctional_names.push_back(name.str());

			if (ss.peek() == ',')
				ss.ignore();
		}
	}
	if (pfunctional_regular > 0 && pfunctional_hermite > 0) {
		std::cerr << "Error: cannot mix orthonormal and regular P-functionals" << std::endl;
		return 1;
	}

	// Read the image
	cv::Mat input = cv::imread(
		fn_input,	// filename
		0		// flags (0 = force grayscale)
	);
	if (input.empty()) {
		std::cerr << "Error: could not load image" << std::endl;
		return 1;
	}
	input = gray2mat(input);

	// Orthonormal P-functionals need a stretched image in order to ensure
	// a square sinogram
	if (pfunctional_hermite > 0) {
		int ndiag = (int) std::ceil(360.0/ANGLE_INTERVAL);
		int nrows = (int) std::ceil(ndiag/std::sqrt(2));
		cv::resize(
			input,
			input,
			cv::Size(nrows, nrows));
	}

	// Save profiling data
	std::vector<double> tfunctional_runtimes(tfunctionals.size());
	std::vector<double> pfunctional_runtimes(pfunctionals.size(), 0);

	// Process all T-functionals
	Profiler mainprofiler;
	cv::Mat data;
	int circus_decimals = 0;
	std::cerr << "Calculating";
	for (unsigned int t = 0; t < tfunctionals.size(); t++) {
		// Calculate the trace transform sinogram
		std::cerr << " " << tfunctional_names[t] << "..." << std::flush;
		Profiler tprofiler;
		cv::Mat sinogram = getTraceTransform(
			input,
			ANGLE_INTERVAL,		// angle resolution
			DISTANCE_INTERVAL,	// distance resolution
			tfunctionals[t]
		);
		tprofiler.stop();
		tfunctional_runtimes[t] = tprofiler.elapsed();

		// Save the sinogram image
		std::stringstream fn_trace_image;
		fn_trace_image << "trace_" << tfunctional_names[t] << ".pgm";
		cv::imwrite(fn_trace_image.str(), mat2gray<double>(sinogram));

		// Save the sinogram data
		std::stringstream fn_trace_data;
		fn_trace_data << "trace_" << tfunctional_names[t] << ".dat";
		int trace_decimals = 0;
		for (int i = 0; i < sinogram.rows; i++) {
			for (int j = 0; j < sinogram.cols; j++) {
				double pixel = sinogram.at<double>(i, j);
				trace_decimals = std::max(trace_decimals,
					(int)std::log10(pixel)
					+ 3);	// for comma and 2 decimals
			}
		}
		trace_decimals += 2;	// add spacing
		std::ofstream fd_trace(fn_trace_data.str());
		fd_trace << std::setiosflags(std::ios::fixed) << std::setprecision(2);
		for (int i = 0; i < sinogram.rows; i++) {
			for (int j = 0; j < sinogram.cols; j++) {
				double pixel = sinogram.at<double>(i, j);
				fd_trace << std::setw(trace_decimals) << pixel;
			}
			fd_trace << "\n";
		}
		fd_trace << std::flush;
		fd_trace.close();

		// Hermite functionals require the nearest orthonormal sinogram
		unsigned int sinogram_center;
		if (pfunctional_hermite > 0) {
			sinogram = nearest_orthonormal_sinogram(sinogram, sinogram_center);
		}

		// Process all P-functionals
		for (unsigned int p = 0; p < pfunctionals.size(); p++) {
			// Extra parameters to functional
			if (pfunctional_hermite > 0)
				((PFunctionalOrthonormal<double,double>*)pfunctionals[p])->setCenter(sinogram_center);

			// Calculate the circus function
			std::cerr << " " << pfunctional_names[p] << "..." << std::flush;
			Profiler pprofiler;
			cv::Mat circus = getCircusFunction(
				sinogram,
				pfunctionals[p]
			);
			pprofiler.stop();
			pfunctional_runtimes[p] += pprofiler.elapsed();

			// Normalize
			cv::Mat normalized = zscore<double>(circus);

			// Allocate the data
			if (data.empty()) {
				data = cv::Mat(
					cv::Size(
						 normalized.cols,
						 tfunctionals.size()*pfunctionals.size()
					),
					CV_64FC1
				);
			} else {
				assert(data.cols == normalized.cols);
			}

			// Copy the data
			for (int i = 0; i < normalized.cols; i++) {
				double pixel = normalized.at<double>(0, i);
				data.at<double>(
					t*pfunctionals.size()+p,	// row
					i				// column
				) = pixel;
				circus_decimals = std::max(circus_decimals,
					(int)std::log10(pixel)
					+ 3);	// for comma and 2 decimals
			}
		}
	}
	std::cerr << "\n";
	mainprofiler.stop();

	// Print runtime measurements	
	std::cerr << "t(total) = " << mainprofiler.elapsed()
		<< " s" << std::endl;
	for (int t = 0; t < tfunctionals.size(); t++) {
		std::cerr << "t(" << tfunctional_names[t] << ") = "
			<< tfunctional_runtimes[t] << " s\n";
	}
	for (int p = 0; p < pfunctionals.size(); p++) {
		std::cerr << "t(" << pfunctional_names[p] << ") = "
			<< pfunctional_runtimes[p] / tfunctionals.size() << " s\n";
	}

	// Save the output data
	if (pfunctionals.size() > 0) {
		std::ofstream fd_data("circus.dat");

		// Headers
		if (circus_decimals < 5)
			circus_decimals = 5;	// size of column header
		circus_decimals += 2;		// add spacing
		fd_data << "%  ";
		fd_data << std::setiosflags(std::ios::fixed) << std::setprecision(0);
		for (size_t tp = 0; tp < (unsigned)data.rows; tp++) {
			size_t t = tp / pfunctionals.size();
			size_t p = tp % pfunctionals.size();
			std::stringstream header;
			header << tfunctional_names[t] << "-"
				<< pfunctional_names[p];
			fd_data << std::setw(circus_decimals) << header.str();
		}
		fd_data << "\n";

		// Data
		fd_data << std::setiosflags(std::ios::fixed) << std::setprecision(2);
		for (int i = 0; i < data.cols; i++) {
			fd_data << "   ";
			for (int tp = 0; tp < data.rows; tp++) {
				fd_data << std::setw(circus_decimals)
					<< data.at<double>(tp, i);
			}
			fd_data << "\n";
		}

		fd_data << std::flush;
		fd_data.close();
	}

	// Generate a gnuplot script
	if (pfunctionals.size() > 0) {
		std::ofstream fd_gnuplot("circus.gp");

		fd_gnuplot << "#!/usr/bin/gnuplot -persist\n";
		fd_gnuplot << "set datafile commentschars '%'\n";

		fd_gnuplot << "plot";
		for (size_t tp = 0; tp < (unsigned)data.rows; tp++) {
			size_t t = tp / pfunctionals.size();
			size_t p = tp % pfunctionals.size();
			fd_gnuplot << "\t'circus.dat' using :" << tp+1
				<< " with lines title '" << tfunctional_names[t] << "-"
				<< pfunctional_names[p] << "'";
			if (tp+1 < (unsigned)data.rows)
				fd_gnuplot << ", \\";
			fd_gnuplot << "\n";
		}

		fd_gnuplot << std::endl;
		fd_gnuplot.close();
		chmod("circus.gp", 0755);
	}

	return 0;
}
