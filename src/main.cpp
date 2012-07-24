//
// Configuration
//

// System includes
#include <iostream>
#include <string>
#include <complex>
#include <iomanip>
#include <ctime>

// OpenCV includes
#include <cv.h>
#include <highgui.h>

// Local includes
#include "auxiliary.h"
#include "traceiterator.h"
#include "tracetransform.h"
#include "orthocircusfunction.h"

// Debug flags
//#define DEBUG_IMAGES


//
// Iterator helpers
//

// Look for the median of the weighed indexes
//
// Conceptually this expands the list of indexes to a weighed one (in which each
// index is repeated as many times as the pixel value it represents), after
// which the median value of that array is located.
template <typename T>
Point iterator_weighedmedian(TraceIterator<T> &iterator)
{
	unsigned long sum = 0;
	while (iterator.hasNext()) {
		sum += iterator.value();
		iterator.next();
	}
	iterator.toFront();

	unsigned long integral = 0;
	Point median;
	while (iterator.hasNext()) {
		median = iterator.point();
		integral += iterator.value();
		iterator.next();

		if (2*integral >= sum)
			break;
	}
	return median;
}

// Look for the median of the weighed indexes, but take the square root of the
// pixel values as weight
template <typename T>
Point iterator_weighedmedian_sqrt(TraceIterator<T> &iterator)
{
	double sum = 0;
	while (iterator.hasNext()) {
		sum += std::sqrt(iterator.value());
		iterator.next();
	}
	iterator.toFront();

	double integral = 0;
	Point median;
	while (iterator.hasNext()) {
		median = iterator.point();
		integral += std::sqrt(iterator.value());
		iterator.next();

		if (2*integral >= sum)
			break;
	}
	return median;
}


//
// T functionals
//

// T-functional for the Radon transform.
//
// T(f(t)) = Int[0-inf] f(t)dt
template <typename T>
double tfunctional_radon(TraceIterator<T> &iterator)
{
	double integral = 0;
	while (iterator.hasNext()) {
		integral += iterator.value();
		iterator.next();
	}
	return integral;
}

// T(f(t)) = Int[0-inf] r*f(r)dr
template <typename T>
double tfunctional_1(TraceIterator<T> &iterator)
{
	// Transform the domain from t to r
	Point r = iterator_weighedmedian(iterator);
	TraceIterator<T> transformed = iterator.transformDomain(
		Segment{
			r,
			iterator.segment().end
		}
	);

	// Integrate
	double integral = 0;
	for (unsigned int t = 0; transformed.hasNext(); t++) {
		integral += transformed.value() * t;
		transformed.next();
	}
	return integral;
}

// T(f(t)) = Int[0-inf] r^2*f(r)dr
template <typename T>
double tfunctional_2(TraceIterator<T> &iterator)
{
	// Transform the domain from t to r
	Point r = iterator_weighedmedian(iterator);
	TraceIterator<T> transformed = iterator.transformDomain(
		Segment{
			r,
			iterator.segment().end
		}
	);

	// Integrate
	double integral = 0;
	for (unsigned int t = 0; transformed.hasNext(); t++) {
		integral += transformed.value() * t*t;
		transformed.next();
	}
	return integral;
}

// T(f(t)) = Int[0-inf] exp(5i*log(r1))*r1*f(r1)dr1
template <typename T>
double tfunctional_3(TraceIterator<T> &iterator)
{
	// Transform the domain from t to r1
	Point r1 = iterator_weighedmedian_sqrt(iterator);
	TraceIterator<T> transformed = iterator.transformDomain(
		Segment{
			r1,
			iterator.segment().end
		}
	);

	// Integrate
	std::complex<double> integral(0, 0);
	const std::complex<double> factor(0, 5);
	for (unsigned int t = 0; transformed.hasNext(); t++) {
		if (t > 0)	// since exp(i*log(0)) == 0
			integral += exp(factor*std::log(t))
				* (t*(double)transformed.value());
		transformed.next();
	}
	return std::abs(integral);
}

// T(f(t)) = Int[0-inf] exp(3i*log(r1))*f(r1)dr1
template <typename T>
double tfunctional_4(TraceIterator<T> &iterator)
{
	// Transform the domain from t to r1
	Point r1 = iterator_weighedmedian_sqrt(iterator);
	TraceIterator<T> transformed = iterator.transformDomain(
		Segment{
			r1,
			iterator.segment().end
		}
	);

	// Integrate
	std::complex<double> integral(0, 0);
	const std::complex<double> factor(0, 3);
	for (unsigned int t = 0; transformed.hasNext(); t++) {
		if (t > 0)	// since exp(i*log(0)) == 0
			integral += exp(factor*std::log(t))
				* (double)transformed.value();
		transformed.next();
	}
	return std::abs(integral);
}

// T(f(t)) = Int[0-inf] exp(4i*log(r1))*sqrt(r1)*f(r1)dr1
template <typename T>
double tfunctional_5(TraceIterator<T> &iterator)
{
	// Transform the domain from t to r1
	Point r1 = iterator_weighedmedian_sqrt(iterator);
	TraceIterator<T> transformed = iterator.transformDomain(
		Segment{
			r1,
			iterator.segment().end
		}
	);

	// Integrate
	std::complex<double> integral(0, 0);
	const std::complex<double> factor(0, 4);
	for (unsigned int t = 0; transformed.hasNext(); t++) {
		if (t > 0)	// since exp(i*log(0)) == 0
			integral += exp(factor*std::log(t))
				* (std::sqrt(t)*(double)transformed.value());
		transformed.next();
	}
	return std::abs(integral);
}


//
// P-functionals
//

// P(g(p)) = Sum(k) abs(g(p+1) -g(p))
template <typename T>
double pfunctional_1(TraceIterator<T> &iterator)
{
	unsigned long sum = 0;
	double previous;
	if (iterator.hasNext()) {
		previous = iterator.value();
		iterator.next();
	}
	while (iterator.hasNext()) {
		double current = iterator.value();
		sum += std::abs(previous -current);
		previous = current;
		iterator.next();
	}
	return (double)sum;
}

// P(g(p)) = median(g(p))
template <typename T>
double pfunctional_2(TraceIterator<T> &iterator)
{
	Point median = iterator_weighedmedian(iterator);
	return iterator.value(median);	// TODO: paper doesn't say g(median)?
}

// P(g(p)) = Int |Fourier(g(p))|^4
template <typename T>
double pfunctional_3(TraceIterator<T> &iterator)
{
	// Dump the trace in a vector
	// TODO: don't do this explicitly?
	std::vector<std::complex<double>> trace;
	while (iterator.hasNext()) {
		trace.push_back(iterator.value());
		iterator.next();
	}

	// Calculate and post-process the Fourier transform
	std::vector<std::complex<double>> fourier = dft(trace);
	std::vector<double> trace_processed(fourier.size());
	for (size_t i = 0; i < fourier.size(); i++)
		trace_processed[i] = std::pow(std::abs(fourier[i]), 4);

	// Integrate
	// FIXME: these values are huge (read: overflow) since we use [0,255]
	double sum = 0;
	for (size_t i = 0; i < trace_processed.size(); i++)
		sum += trace_processed[i];
	return sum;

}

template <typename T>
double pfunctional_4(TraceIterator<T> &iterator)
{
	return tfunctional_4(iterator);
}

/*
template <typename T>
double pfunctional_5(TraceIterator &iterator)
{
	return tfunctional_6(iterator);
}
*/

/*
template <typename T>
double pfunctional_6(TraceIterator &iterator)
{
	return tfunctional_7(iterator);
}
*/


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

// Available T-functionals
const std::vector<Functional<uchar,double>> TFUNCTIONALS{
	tfunctional_radon<uchar>,
	tfunctional_1<uchar>,
	tfunctional_2<uchar>,
	tfunctional_3<uchar>,
	tfunctional_4<uchar>,
	tfunctional_5<uchar>
};

// Available P-functionals
const std::vector<Functional<double,double>> PFUNCTIONALS{
	nullptr,
	pfunctional_1<double>,
	pfunctional_2<double>,
	pfunctional_3<double>
};

int main(int argc, char **argv)
{
	// Check and read the parameters
	if (argc < 4) {
		std::cerr << "Invalid usage: " << argv[0]
			<< " INPUT T-FUNCTIONALS P-FUNCTIONALS" << std::endl;
		return 1;
	}
	std::string fn_input = argv[1];

	// Get the chosen T-functionals
	std::stringstream ss;
	ss << argv[2];
	unsigned short i;
	std::vector<unsigned short> chosen_tfunctionals;
	while (ss >> i) {
		if (ss.fail() || i >= TFUNCTIONALS.size() || TFUNCTIONALS[i] == nullptr) {
			std::cerr << "Error: invalid T-functional provided" << std::endl;
			return 1;
		}
		chosen_tfunctionals.push_back(i);
		if (ss.peek() == ',')
			ss.ignore();
	}

	// Get the chosen P-functional
	ss.clear();
	ss << argv[3];
	std::vector<unsigned short> chosen_pfunctionals;
	while (ss >> i) {
		if (ss.fail() || i >= PFUNCTIONALS.size() || PFUNCTIONALS[i] == nullptr) {
			std::cerr << "Error: invalid P-functional provided" << std::endl;
			return 1;
		}
		chosen_pfunctionals.push_back(i);
		if (ss.peek() == ',')
			ss.ignore();
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

	// Save profiling data
	std::vector<double> runtimes(chosen_tfunctionals.size()*chosen_pfunctionals.size());

	// Process all T-functionals
	cv::Mat data;
	int decimals = 7;	// size of the column header
	std::cerr << "Calculating ";
	for (size_t t = 0; t < chosen_tfunctionals.size(); t++) {
		// Calculate the trace transform sinogram
		std::cerr << " T" << chosen_tfunctionals[t] << "..." << std::flush;
		Profiler tprofiler;
		cv::Mat sinogram = getTraceTransform(
			input,
			1,	// angle resolution
			1,	// distance resolution
			TFUNCTIONALS[chosen_tfunctionals[t]]
		);
		tprofiler.stop();

		// Show the trace transform sinogram
		std::stringstream sinogram_title;
		sinogram_title << "sinogram after functional T" << chosen_tfunctionals[t];
		#ifdef DEBUG_IMAGES
		cv::imshow(sinogram_title.str(), mat2gray(sinogram));
		#endif

		// Process all P-functionals
		for (size_t p = 0; p < chosen_pfunctionals.size(); p++) {
			// Calculate the circus function
			std::cerr << " P" << chosen_pfunctionals[p] << "..." << std::flush;
			Profiler pprofiler;
			cv::Mat circus = getOrthonormalCircusFunction(
				sinogram,
				PFUNCTIONALS[chosen_pfunctionals[p]]
			);
			pprofiler.stop();
			runtimes[t*chosen_pfunctionals.size()+p]
				= tprofiler.elapsed() + pprofiler.elapsed();

			// Allocate the data
			if (data.empty()) {
				data = cv::Mat(
					cv::Size(
						 circus.cols,
						 chosen_tfunctionals.size()*chosen_pfunctionals.size()
					),
					CV_64FC1
				);
			} else {
				assert(data.cols == circus.cols);
			}

			// Copy the data
			for (int i = 0; i < circus.cols; i++) {
				double pixel = circus.at<double>(0, i);
				data.at<double>(
					t*chosen_pfunctionals.size()+p,	// row
					i				// column
				) = pixel;
				decimals = std::max(decimals, (int)std::log10(pixel)+3);
			}
		}
	}
	std::cerr << std::endl;

	// Output the headers
	decimals += 2;
	std::cout << "#  ";
	std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(0);
	for (size_t tp = 0; tp < (unsigned)data.rows; tp++) {
		size_t t = tp / chosen_pfunctionals.size();
		size_t p = tp % chosen_pfunctionals.size();
		std::stringstream header;
		header << "T" << chosen_tfunctionals[t]
			<< "-P" << chosen_pfunctionals[p];
		std::cout << std::setw(decimals) << header.str();
	}
	std::cout << "\n";

	// Output the data
	std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(2);
	for (int i = 0; i < data.cols; i++) {
		std::cout << "   ";
		for (int tp = 0; tp < data.rows; tp++) {
			std::cout << std::setw(decimals)
				<< data.at<double>(tp, i);
		}
		std::cout << "\n";
	}

	// Output the footer
	std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(0);
	std::cout << "#  ";
	for (size_t tp = 0; tp < (unsigned)data.rows; tp++) {
		std::stringstream runtime;
		runtime << 1000.0*runtimes[tp] << "ms";
		std::cout << std::setw(decimals) << runtime.str();
	}
	std::cout << std::endl;

	// Give the user time to look at the images
	#ifdef DEBUG_IMAGES
	cv::waitKey();
	#endif

	return 0;
}
