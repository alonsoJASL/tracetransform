//
// Configuration
//

// Header include
#include "auxiliary.hpp"

// Standard library
#include <fstream>
#include <iomanip>
#include <cassert>
#include <cstddef>
#include <cmath>
#include <stdexcept>

// Local
#include "logger.hpp"


//
// Routines
//

template <typename T> T readNetpbmValue(std::stringstream &stream) {
    // Skip whitespace and comment lines
    char next;
    do {
        next = stream.get();
        while (next == '#') {
            stream.ignore(INT_MAX, '\n');
            next = stream.get();
        }
    } while (next == ' ' || next == '\t' || next == '\n' || next == '\r');
    stream.unget();

    // Extract the value
    T value;
    stream >> value;
    if (stream.fail())
        throw std::runtime_error("Error processing file");

    return value;
}

std::vector<Eigen::MatrixXi> readnetpbm(std::string filename) {
    std::ifstream infile(filename);
    if (!infile.good())
        throw std::runtime_error("could not open input file");
    std::stringstream ss;
    ss << infile.rdbuf();

    // Magic string
    std::string magic;
    magic = readNetpbmValue<std::string>(ss);
    size_t channels;
    if (magic == "P2")
        channels = 1;
    else if (magic == "P3")
        channels = 3;
    else
        throw std::runtime_error("Invalid Netpbm magic");

    // Image size
    size_t numcols = readNetpbmValue<size_t>(ss);
    size_t numrows = readNetpbmValue<size_t>(ss);
    std::vector<Eigen::MatrixXi> data(channels);
    for (size_t i = 0; i < channels; i++)
        data[i] = Eigen::MatrixXi(numrows, numcols);

    // Maxval
    size_t maxval = readNetpbmValue<size_t>(ss);
    if (maxval != 255)
        clog(warning) << "Pixels not properly clipped to [0,255]" << std::endl;

    // Data
    unsigned int value;
    for (size_t row = 0; row < numrows; row++) {
        for (size_t col = 0; col < numcols; col++) {
            for (size_t i = 0; i < channels; i++) {
                value = readNetpbmValue<unsigned int>(ss);
                data[i](row, col) = value;
            }
        }
    }

    // Trailing data?
    char next = ss.peek();
    while (next == ' ' || next == '\t' || next == '\n' || next == '\r') {
        ss.get();
        next = ss.peek();
    }
    if (!ss.eof())
        clog(warning) << "Trailing data at end of image file" << std::endl;
    infile.close();

    return data;
}

void writepgm(std::string filename, const Eigen::MatrixXi &data) {
    std::ofstream outfile(filename);

    // First line: version
    outfile << "P2"
            << "\n";

    // Second line: size
    outfile << data.cols() << " " << data.rows() << "\n";

    // Third line: maxval
    outfile << 255 << "\n";

    // Data
    long pos = outfile.tellp();
    for (int row = 0; row < data.rows(); row++) {
        for (int col = 0; col < data.cols(); col++) {
            outfile << data(row, col);
            if (outfile.tellp() - pos > 66) {
                outfile << "\n";
                pos = outfile.tellp();
            } else {
                outfile << " ";
            }
        }
    }
    outfile.close();
}

void writecsv(std::string filename, const Eigen::MatrixXf &data) {
    // Open file
    std::ofstream fd_data(filename);

    // Print data
    for (int row = 0; row < data.rows(); row++) {
        for (int col = 0; col < data.cols(); col++) {
            fd_data << data(row, col);
            if (col < data.cols() - 1)
                fd_data << ", ";
        }
        fd_data << "\n";
    }

    fd_data << std::flush;
    fd_data.close();
}

Eigen::MatrixXf gray2mat(const Eigen::MatrixXi &input) {
    // Scale
    Eigen::MatrixXf output(input.rows(), input.cols());
    for (int col = 0; col < output.cols(); col++) {
        for (int row = 0; row < output.rows(); row++) {
            output(row, col) = input(row, col) / 255.0;
        }
    }
    return output;
}

Eigen::MatrixXi mat2gray(const Eigen::MatrixXf &input) {
    // Detect maximum
    float maximum = 0;
    for (int col = 0; col < input.cols(); col++) {
        for (int row = 0; row < input.rows(); row++) {
            float pixel = input(row, col);
            if (pixel > maximum)
                maximum = pixel;
        }
    }

    // Scale
    Eigen::MatrixXi output(input.rows(), input.cols());
    for (int col = 0; col < output.cols(); col++) {
        for (int row = 0; row < output.rows(); row++) {
            output(row, col) = input(row, col) * 255.0 / maximum;
        }
    }
    return output;
}

float deg2rad(float degrees) { return (degrees * M_PI / 180); }

float interpolate(const Eigen::MatrixXf &source, const Point<float>::type &p) {
    assert(p.x() >= 0 && p.x() < source.cols() - 1);
    assert(p.y() >= 0 && p.y() < source.rows() - 1);

    // Get fractional and integral part of the coordinates
    float x_int, y_int;
    float x_fract = std::modf(p.x(), &x_int);
    float y_fract = std::modf(p.y(), &y_int);

    return source(y_int, x_int) * (1 - x_fract) * (1 - y_fract) +
           source(y_int, x_int + 1) * x_fract * (1 - y_fract) +
           source(y_int + 1, x_int) * (1 - x_fract) * y_fract +
           source(y_int + 1, x_int + 1) * x_fract * y_fract;
}

Eigen::MatrixXf resize(const Eigen::MatrixXf &input, const size_t rows,
                       const size_t cols) {
    // Calculate transform matrix
    Eigen::Matrix2f transform;
    transform << ((float)input.rows()) / rows, 0, 0,
        (((float)input.cols()) / cols);

    // Allocate output matrix
    Eigen::MatrixXf output = Eigen::MatrixXf::Zero(rows, cols);

    // Process all points
    // FIXME: borders are wrong (but this doesn't matter here since we
    //        only handle padded images)
    for (size_t col = 1; col < cols - 1; col++) {
        for (size_t row = 1; row < rows - 1; row++) {
            Point<float>::type p(col, row);
            p += Eigen::RowVector2f(0.5, 0.5);
            p *= transform;
            p -= Eigen::RowVector2f(0.5, 0.5);
            output(row, col) = interpolate(input, p);
        }
    }
    return output;
}

Eigen::MatrixXf pad(const Eigen::MatrixXf &image) {
    // Pad the images so we can freely rotate without losing information
    Point<float>::type origin(std::floor((image.cols() + 1) / 2.0) - 1,
                              std::floor((image.rows() + 1) / 2.0) - 1);
    int rLast = (int)std::ceil(std::hypot(image.cols() - 1 - origin.x() - 1,
                                          image.rows() - 1 - origin.y() - 1)) +
                1;
    int rFirst = -rLast;
    int nBins = (rLast - rFirst + 1);
    Eigen::MatrixXf image_padded = Eigen::MatrixXf::Zero(nBins, nBins);
    Point<float>::type origin_padded(
        std::floor((image_padded.cols() + 1) / 2.0) - 1,
        std::floor((image_padded.rows() + 1) / 2.0) - 1);
    Point<float>::type df = origin_padded - origin;
    image_padded.block((int)df.y(), (int)df.x(), image.rows(), image.cols()) =
        image;

    return image_padded;
}

std::string readable_si(double number, const std::string unit, double base) {
    const std::vector<std::string> dimensions = { "",  "K", "M", "G", "T",
                                                  "P", "E", "Z", "Y" };

    int i = 0;
    while (number >= base) {
        number /= base;
        i++;
    }

    std::stringstream ss;
    ss << std::setiosflags(std::ios::fixed) << std::setprecision(2) << number
       << " " << dimensions[i] << unit;
    return ss.str();
}

std::string readable_size(double size) { return readable_si(size, "iB", 1024); }

std::string readable_frequency(double frequency) {
    return readable_si(frequency, "Hz");
}
